"""Read-only native Market validation. Usage: python this_file.py PATH/oot.o2r.

No extraction, game launch, or external packages. Geometry is read from the
owner's local archive; no game asset data is stored in the repository.
"""
import argparse
import copy
import math
import struct
import zipfile


HASH_START = 14695981039346656037
HASH_PRIME = 1099511628211
MASK64 = (1 << 64) - 1
EXPECTED = {
    "day": ("002640", 0xD5DB47D7BE704D74),
    "night": ("0025F8", 0xE6C2154E570E39A5),
    "ruins": ("0015F8", 0xA742D89F91AECEDD),
}
GROUPS = {
    "scenes/shared/market_day_scene/": (44, 0xFF17C2A15E8C71A1),
    "scenes/shared/market_night_scene/": (47, 0x220DF8E517158FFE),
    "objects/gameplay_field_keep/": (99, 0x3F7D3018D3A7D7B6),
    "objects/gameplay_dangeon_keep/": (97, 0xC403CB7C046EDAA9),
}
EMPTY_LABELS = {
    "scenes/shared/market_day_scene/market_day_room_0ActorEntry_000060",
    "scenes/shared/market_night_scene/market_night_room_0ActorEntry_000050",
    "objects/gameplay_field_keep/gButterflySkelLimbs",
    "objects/gameplay_field_keep/gFieldUnusedFishSkelLimbs",
}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def rejects(operation, message):
    try:
        operation()
    except ValueError:
        return
    raise ValueError(message)


def native_group(names, prefix, count, expected):
    entries = sorted(name for name in names if name.startswith(prefix) and not name.endswith(".meta"))
    hashed = HASH_START
    for name in entries:
        for byte in name.encode() + b"\0":
            hashed = ((hashed ^ byte) * HASH_PRIME) & MASK64
    require(len(entries) == count and hashed == expected, "Incomplete native resource group")
    return entries


class Reader:
    def __init__(self, data):
        require(len(data) >= 64 and data[:4] == bytes(4), "Invalid native resource header")
        self.data, self.offset = data, 64

    def read(self, fmt):
        length = struct.calcsize("<" + fmt)
        require(self.offset + length <= len(self.data), "Truncated resource")
        value = struct.unpack_from("<" + fmt, self.data, self.offset)
        self.offset += length
        return value[0] if len(value) == 1 else value

    def array(self, fmt):
        count = self.read("I")
        require(count <= 10000, "Invalid resource count")
        return [self.read(fmt) for _ in range(count)]

    def string(self):
        count = self.read("I")
        require(count <= 1024 and self.offset + count <= len(self.data), "Invalid resource path")
        value = self.data[self.offset:self.offset + count].decode()
        self.offset += count
        return value


def collision(data):
    reader = Reader(data)
    result = {"bounds": reader.read("6h")}
    result["vertices"] = reader.array("3h")
    result["polygons"] = reader.array("4H4h")
    result["surfaces"] = [(second, first) for first, second in reader.array("II")]
    result["cameras"] = reader.array("HhI")
    result["poses"] = reader.array("3h")
    result["water"] = reader.array("5hI")
    require(reader.offset == len(data), "Unexpected trailing collision data")
    return result


def fingerprint(data):
    words = list(data["bounds"])
    for field in ("vertices", "polygons", "surfaces", "cameras", "poses", "water"):
        words.append(len(data[field]))
        for row in data[field]:
            words.extend(row)
    result = HASH_START
    for word in words:
        for byte in struct.pack("<I", word & 0xFFFFFFFF):
            result = ((result ^ byte) * HASH_PRIME) & MASK64
    return result


def scene(data):
    reader, commands = Reader(data), {}
    for _ in range(reader.read("I")):
        command = reader.read("I")
        require(command not in commands, "Duplicate native scene command")
        if command == 4:
            value = [(reader.string(), reader.read("II")) for _ in range(reader.read("I"))]
        elif command == 3:
            value = reader.string()
        elif command in (0, 1):
            value = reader.array("HhhhhhhH")
        elif command == 6:
            value = reader.array("BB")
        elif command == 7:
            value = reader.read("Bh")
        elif command in (8, 25):
            value = reader.read("BI")
        elif command in (11, 19):
            value = reader.array("H")
        elif command == 13:
            value = [reader.string() for _ in range(reader.read("I"))]
        elif command == 14:
            value = reader.array("4Bhhhhhh")
        elif command == 15:
            value = reader.array("18Bhh")
        elif command in (16, 21):
            value = reader.read("BBB")
        elif command == 17:
            value = reader.read("BBBB")
        elif command == 18:
            value = reader.read("BB")
        elif command == 22:
            value = reader.read("B")
        elif command == 10:
            unused, mesh_type, count = reader.read("BBB")
            require(mesh_type == 0 and count == 1, "Unexpected native room mesh")
            value = (unused, mesh_type, [(reader.read("B"), reader.string(), reader.string()) for _ in range(count)])
        elif command == 20:
            value = None
        else:
            raise ValueError("Unsupported scene command " + str(command))
        commands[command] = value
    require(reader.offset == len(data), "Unexpected trailing scene data")
    return commands


def triangle(data, polygon):
    return [data["vertices"][value & 8191] for value in polygon[1:4]]


def floor_at(data, x, z):
    heights = []
    for poly in data["polygons"]:
        if poly[5] <= 0:
            continue
        a, b, c = triangle(data, poly)
        denominator = (b[2] - c[2]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[2] - c[2])
        if denominator == 0:
            continue
        first = ((b[2] - c[2]) * (x - c[0]) + (c[0] - b[0]) * (z - c[2])) / denominator
        second = ((c[2] - a[2]) * (x - c[0]) + (a[0] - c[0]) * (z - c[2])) / denominator
        third = 1 - first - second
        if min(first, second, third) >= -1e-6:
            height = first * a[1] + second * b[1] + third * c[1]
            if height <= 40:
                heights.append(height)
    require(heights, "Missing walkable floor")
    return max(heights)


def segment_hits_triangle(start, end, vertices):
    def sub(a, b): return tuple(x - y for x, y in zip(a, b))
    def dot(a, b): return sum(x * y for x, y in zip(a, b))
    def cross(a, b): return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
    origin, second, third = vertices
    direction, edge1, edge2 = sub(end, start), sub(second, origin), sub(third, origin)
    p = cross(direction, edge2)
    determinant = dot(edge1, p)
    if abs(determinant) < 1e-8:
        return False
    offset = sub(start, origin)
    u = dot(offset, p) / determinant
    q = cross(offset, edge1)
    v, distance = dot(direction, q) / determinant, dot(edge2, q) / determinant
    return u >= -1e-6 and v >= -1e-6 and u + v <= 1.000001 and 0 <= distance <= 1


def distance_to_triangle(point, vertices):
    def sub(a, b): return tuple(x - y for x, y in zip(a, b))
    def dot(a, b): return sum(x * y for x, y in zip(a, b))
    a, b, c = vertices
    ab, ac, ap = sub(b, a), sub(c, a), sub(point, a)
    aa, bb, cc = dot(ab, ab), dot(ab, ac), dot(ac, ac)
    determinant = aa * cc - bb * bb
    candidates = []
    if determinant > 0:
        first = (cc * dot(ap, ab) - bb * dot(ap, ac)) / determinant
        second = (aa * dot(ap, ac) - bb * dot(ap, ab)) / determinant
        if first >= 0 and second >= 0 and first + second <= 1:
            projected = tuple(x + first * y + second * z for x, y, z in zip(a, ab, ac))
            candidates.append(math.dist(point, projected))
    for start, end in ((a, b), (b, c), (c, a)):
        edge = sub(end, start)
        length_squared = dot(edge, edge)
        along = min(1, max(0, dot(sub(point, start), edge) / length_squared)) if length_squared else 0
        candidates.append(math.dist(point, tuple(x + along * y for x, y in zip(start, edge))))
    return min(candidates)


def run(archive):
    with zipfile.ZipFile(archive) as package:
        names = set(package.namelist())
        for prefix, (count, expected) in GROUPS.items():
            entries = native_group(names, prefix, count, expected)
            # A missing dependency, renamed file, or unexpected geometry layout
            # must fail the same whole-group preflight used before payment.
            required_name = next(name for name in entries if name not in EMPTY_LABELS)
            rejects(lambda: native_group(names - {required_name}, prefix, count, expected),
                    "Missing native dependency accepted")
            rejects(lambda: native_group((names - {required_name}) | {required_name + "_changed"},
                                         prefix, count, expected), "Renamed native dependency accepted")
            for name in entries:
                if name in EMPTY_LABELS:
                    require(not package.read(name), "Unexpected native address label data")
                else:
                    Reader(package.read(name))
        geometry = {}
        for variant, (suffix, expected) in EXPECTED.items():
            prefix = f"scenes/shared/market_{variant}_scene/market_{variant}"
            data = package.read(prefix + "_sceneCollisionHeader_" + suffix)
            geometry[variant] = collision(data)
            require(fingerprint(geometry[variant]) == expected, "Unexpected collision fingerprint")
            mutated = copy.deepcopy(geometry[variant])
            mutated["vertices"][0] = (0, 0, 0)
            require(fingerprint(mutated) != expected, "Mutation did not invalidate fingerprint")
            rejects(lambda: collision(data[:-1]), "Truncation was accepted")
            rejects(lambda: collision(bytes(63)), "Missing collision was accepted")
        ruins = geometry["ruins"]
        original_scene = scene(package.read("scenes/shared/market_ruins_scene/market_ruins_scene"))
        require(original_scene[19] == [0x33, 0x138, 0x171, 0xAD, 0x29A, 0], "Adult exits changed")
        require(len(original_scene[6]) == 12 and original_scene[6][:3] == [(0, 0), (1, 0), (2, 0)],
                "Adult entrance mapping changed")
        require(len(original_scene[0]) == 11 and [row[1:4] for row in original_scene[0][:3]] ==
                [(-4, 0, 768), (-2, 0, -762), (477, 0, -367)], "Adult spawn positions changed")
        require(14 not in original_scene, "Unexpected adult transition actors")
        caps = [triangle(ruins, polygon) for polygon in ruins["polygons"][197:213]]
        # Rays from the square into each of the six shops and two alleys must
        # hit retained native walls throughout normal standing body height.
        blocked_routes = [((-480, 580), (-480, 630)), ((-450, 480), (-550, 550)),
                          ((-450, -480), (-550, -550)), ((-450, 0), (-495, 0)),
                          ((510, -120), (550, -120)), ((510, 220), (550, 220)),
                          ((320, -550), (320, -600)), ((-240, -585), (-240, -640))]
        for start, end in blocked_routes:
            for height in (10, 40, 80):
                require(any(segment_hits_triangle((start[0], height, start[1]), (end[0], height, end[1]), cap)
                            for cap in caps), "Native closed approach is open")
        for variant in ("day", "night"):
            original = copy.deepcopy(geometry[variant])
            prefix = f"scenes/shared/market_{variant}_scene/market_{variant}"
            room = scene(package.read(prefix + "_room_0"))
            native_scene = scene(package.read(prefix + "_scene"))
            require(len(native_scene[14]) == 6, "Expected six native storefront doors")
            require(native_scene[17][1] == (9 if variant == "day" else 10), "Unexpected panorama selection")
            suffix = "0057D8" if variant == "day" else "005708"
            require(room[10][2] == [(0, prefix + "_room_0DL_" + suffix, "")], "Native mesh mismatch")
            for camera, exit_id in ((word & 255, (word >> 8) & 31) for word, _ in original["surfaces"]):
                require(camera < len(original["cameras"]) and exit_id <= 11, "Invalid source surface")
            for spawn in original_scene[0][:3]:
                x, y, z = spawn[1:4]
                for dx, dz in ((0, 0), (20, 0), (-20, 0), (0, 20), (0, -20),
                               (14, 14), (-14, 14), (14, -14), (-14, -14)):
                    require(abs(floor_at(original, x + dx, z + dz) - y) < 0.01, "Unsafe adult spawn footprint")
                for target in ((x, 75, z), (x+20, 40, z), (x-20, 40, z), (x, 40, z+20), (x, 40, z-20)):
                    require(not any(segment_hits_triangle((x, 25, z), target, cap) for cap in caps), "Spawn intersects closure")
                for height in (25, 50, 75):
                    all_triangles = [triangle(original, polygon) for polygon in original["polygons"]] + caps
                    require(min(distance_to_triangle((x, height, z), tri) for tri in all_triangles) >= 20,
                            "Adult spawn body is obstructed")
            # The genuine adult corridor exits retain identical world triangles.
            for exit_id in (1, 2, 3):
                def exit_triangles(data):
                    return sorted(sorted(triangle(data, p)) for p in data["polygons"]
                                  if (data["surfaces"][p[0]][0] >> 8) & 31 == exit_id)
                require(exit_triangles(original) == exit_triangles(ruins), "Adult route boundary changed")
            require(geometry[variant] == original, "Source resource mutated")
            for side in (-1, 1):
                for stack in range(7):
                    along = (stack + 0.5) / 7
                    x, z = -460 - 100*along + 12, side*(440 + 160*along - 8)
                    for dx, dz in ((0, 0), (-14, -14), (14, -14), (-14, 14), (14, 14)):
                        require(abs(floor_at(original, x + dx, z + dz) - 2) <= 2.01, "Barricade lacks ground")
            title, folder = ("Day", "MDVR") if variant == "day" else ("Night", "MNVR")
            for face in range(1, 5):
                require(f"textures/vr_{folder}_static/gMarket{title}{'' if face == 1 else face}BgTex" in names,
                        "Missing panorama face")
                require(f"textures/vr_{folder}_pal_static/gMarket{title}Bg{'' if face == 1 else face}TLUT" in names,
                        "Missing panorama palette")
        print("Market resources: fingerprints, dependencies, closed approaches, adult spawn footprints, original exits, and panoramas passed.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive")
    run(parser.parse_args().archive)
