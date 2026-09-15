"""Read-only Domain restoration checks against the owner's native oot.o2r.

Usage: python ZoraRestorationResourceTests.py PATH/oot.o2r
No extraction, game launch, asset output, or third-party modules.
"""
import argparse
import copy
import math
from pathlib import Path
import re
import struct
import sys
import zipfile

sys.dont_write_bytecode = True
from MarketRestorationResourceTests import (
    HASH_START, HASH_PRIME, MASK64, Reader, collision, fingerprint,
    require, rejects, native_group, triangle, segment_hits_triangle, distance_to_triangle,
)

PREFIX = "scenes/shared/spot07_scene/"
COLLISIONS = {
    PREFIX + "spot07_sceneCollisionHeader_003824": 0xFB86681709703C4D,
    "objects/object_spot07_object/object_spot07_object_Col_002590": 0x3A9C4A753979D621,
    "objects/object_spot07_object/object_spot07_object_Col_0038FC": 0x6BDDD95748B89EBF,
    "objects/object_spot06_objects/gLakeHyliaZoraShortcutIceblockCol": 0xF7850481F98BA469,
}
GROUPS = {
    PREFIX: (196, 0x3644526F41E93EDD),
    "objects/object_spot07_object/": (25, 0xA7D640EB3BD059E6),
    "objects/object_spot06_objects/": (16, 0x74486EF104396A16),
}
EMPTY_LABELS = {
    PREFIX + "spot07_room_0ActorEntry_000068", PREFIX + "spot07_room_0ActorEntry_000270",
    PREFIX + "spot07_room_0ActorEntry_000348", PREFIX + "spot07_room_1ActorEntry_000068",
    PREFIX + "spot07_room_1ActorEntry_0003B0", PREFIX + "spot07_room_1ActorEntry_000508",
}
DISPLAYS = {
    "objects/object_spot06_objects/gLakeHyliaZoraShortcutIceblockDL": 0xDFC1178A25D8EEE0,
    "objects/object_spot07_object/object_spot07_object_DL_000460": 0x0B1FAC08E451C764,
    "objects/object_spot07_object/object_spot07_object_DL_000BE0": 0x1F039ECFF6CB927F,
}


def words_hash(words):
    result = HASH_START
    for word in words:
        for byte in struct.pack("<I", word & 0xFFFFFFFF):
            result = ((result ^ byte) * HASH_PRIME) & MASK64
    return result


def scene(data):
    require(data[4:8] == b"MORO", "Expected native scene or room")
    reader, result = Reader(data), {}
    for _ in range(reader.read("I")):
        command = reader.read("I")
        require(command not in result, "Duplicate scene command")
        if command in (3, 23):
            value = reader.string()
        elif command in (0, 1):
            value = reader.array("HhhhhhhH")
        elif command == 4:
            value = [(reader.string(), reader.read("II")) for _ in range(reader.read("I"))]
        elif command == 6:
            value = reader.array("BB")
        elif command == 7:
            value = reader.read("Bh")
        elif command in (8, 25):
            value = reader.read("BI")
        elif command in (11, 19):
            value = reader.array("H")
        elif command in (13, 24):
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
            unused, kind, count = reader.read("BBB")
            require(kind in (0, 2), "Unsupported room mesh")
            value = (unused, kind, [(reader.read("B"), reader.read("4h") if kind == 2 else None,
                                     reader.string(), reader.string()) for _ in range(count)])
        elif command == 20:
            value = None
        else:
            raise ValueError("Unsupported scene command " + str(command))
        result[command] = value
    require(reader.offset == len(data), "Unexpected scene data")
    return result


def floor_at(data, x, y, z):
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
            if height <= y + 32:
                heights.append(height)
    require(heights, "Missing spawn floor")
    return max(heights)


def policy_geometry():
    # Read the actual exported constants, so changing the drawing transform or
    # rounded collision cannot silently leave a stale test-only cap in use.
    header = Path(__file__).resolve().parents[2] / "soh/soh/Enhancements/living-hyrule/ZoraRestorationPolicy.h"
    text = header.read_text(encoding="utf-8")
    def numbers(name):
        match = re.search(name + r"\s*\{(.*?)\};", text, re.S)
        require(match is not None, "Missing closure constants")
        return [float(value) for value in re.findall(r"-?\d+(?:\.\d+)?", match.group(1))]
    position, scale = numbers("kZoraClosurePosition"), numbers("kZoraClosureScale")
    values = numbers("kZoraClosureVertices")
    require(len(values) == 12, "Unexpected cap vertices")
    vertices = [tuple(values[offset:offset + 3]) for offset in range(0, 12, 3)]
    yaw = int(re.search(r"kZoraClosureYaw\s*=\s*(-?\d+)", text).group(1))
    return position, scale, yaw, vertices


def verify_closure(base, vertices):
    caps = [[vertices[index] for index in indices] for indices in ((0, 1, 2), (0, 3, 1))]
    # The existing rock mouth is 79.12 units wide and 80 high. Test its complete
    # width/height including edges, in both ray directions, after vertex rounding.
    left, right = (-213, -192), (-137, -214)
    normal = (-22 / math.hypot(22, 76), -76 / math.hypot(22, 76))
    rays, exposed = 0, 0
    for column in range(41):
        along = column / 40
        x = left[0] + along * (right[0] - left[0])
        z = left[1] + along * (right[1] - left[1])
        for row in range(21):
            y = -220 + row * 4
            a = (x + normal[0] * 14, y, z + normal[1] * 14)
            b = (x - normal[0] * 14, y, z - normal[1] * 14)
            require(any(segment_hits_triangle(a, b, cap) for cap in caps), "Unclosed Lake shortcut gap")
            require(any(segment_hits_triangle(b, a, cap) for cap in caps), "Closure reverse ray gap")
            exposed += not any(segment_hits_triangle(a, b, triangle(base, poly)) for poly in base["polygons"])
            rays += 1
    require(exposed > 500, "The test no longer covers the open native tunnel")
    # Plane quantization used by the game must still agree with the vertices.
    for a, b, c in caps:
        ab = tuple(y - x for x, y in zip(a, b))
        ac = tuple(y - x for x, y in zip(a, c))
        cross = (ab[1]*ac[2]-ab[2]*ac[1], ab[2]*ac[0]-ab[0]*ac[2], ab[0]*ac[1]-ab[1]*ac[0])
        length = math.sqrt(sum(value * value for value in cross))
        normal3 = tuple(round(value * 32767 / length) / 32767 for value in cross)
        dist = round(-sum(value * point for value, point in zip(cross, a)) / length)
        require(length > 1000 and normal3[1] == 0 and normal3[2] < 0, "Invalid cap normal")
        for vertex in (a, b, c):
            require(abs(sum(n * v for n, v in zip(normal3, vertex)) + dist) < 0.6, "Cap plane rounding mismatch")
    return rays, caps


def run(archive):
    with zipfile.ZipFile(archive) as package:
        names = set(package.namelist())
        for prefix, (count, hashed) in GROUPS.items():
            entries = native_group(names, prefix, count, hashed)
            for name in entries:
                data = package.read(name)
                require((name in EMPTY_LABELS and len(data) == 0) or len(data) >= 64, "Missing native dependency")
            missing = names - {entries[-1]}
            rejects(lambda: native_group(missing, prefix, count, hashed), "Missing assets accepted")

        collisions = []
        for path, expected in COLLISIONS.items():
            raw = package.read(path)
            require(raw[4:8] == b"LOCO", "Expected native collision")
            decoded = collision(raw)
            require(fingerprint(decoded) == expected, "Unrecognized native collision layout")
            rejects(lambda: collision(raw[:-1]), "Truncated collision accepted")
            mutated = copy.deepcopy(decoded)
            mutated["surfaces"][0] = (mutated["surfaces"][0][0] ^ 256, mutated["surfaces"][0][1])
            require(fingerprint(mutated) != expected, "Exit mutation escaped fingerprint")
            collisions.append(decoded)
        base, pool_ice, upper_ice, native_cap = collisions
        require([len(base[key]) for key in ("vertices", "polygons", "surfaces", "cameras", "poses", "water")]
                == [384, 695, 25, 9, 3, 4], "Unexpected Domain geometry size")
        require(base["water"] == [(-157, 1008, -2826, 1000, 1080, 260), (-348, 877, -1746, 553, 780, 260),
                                  (205, 877, -1746, 847, 520, 260), (-1262, 0, -1068, 2480, 1418, 8452)],
                "Native water volumes changed")
        require(base["surfaces"][24] == (1029, 24514), "Unexpected underwater exit material")
        require(base["cameras"] == [(18, 3, 0), (0, 0, 0), (3, 0, 0), (58, 0, 0), (5, 0, 0),
                                     (4, 0, 0), (1, 0, 0), (2, 0, 0), (62, 0, 0)],
                "Native camera indices or retained pose pointers changed")
        exits = [index for index, poly in enumerate(base["polygons"])
                 if ((base["surfaces"][poly[0]][0] >> 8) & 31) == 4]
        require(exits == [693, 694], "Exit4 no longer confined to the tunnel")
        for index in exits:
            require(all(vertex[1] == -220 for vertex in triangle(base, base["polygons"][index])), "Portal floor moved")

        child = scene(package.read(PREFIX + "spot07_scene"))
        adult = scene(package.read(PREFIX + "spot07_sceneSet_003A40"))
        require(child[24] == ["", PREFIX + "spot07_sceneSet_003A40", "", PREFIX + "spot07_sceneSet_003C60"],
                "Adult alternate scene routing changed")
        require(adult[19] == [0x19D, 0x225, 0x380, 0x560], "Original exits changed")
        for command in (3, 14, 15, 17, 19):
            require(child[command] == adult[command], "Static child/adult scene command mismatch")
        room_actors = []
        for index, suffix, hashed in ((0, "000220", 0xA7551307B3117FC5), (1, "000360", 0x8959061EAC5892D9)):
            child_room = scene(package.read(PREFIX + "spot07_room_" + str(index)))
            adult_path = PREFIX + "spot07_room_" + str(index) + "Set_" + suffix
            adult_room = scene(package.read(adult_path))
            require(child_room[24][1] == adult_path and child_room[24][2] == "", "Adult room layer routing changed")
            require(child_room[10] == adult_room[10] and child_room[10][1] == 2, "Permanent rooms are not identical")
            require(len(child_room[10][2]) == (12 if index == 0 else 20), "Unexpected mesh density")
            actors = adult_room[1]
            require(words_hash([len(actors)] + [value for actor in actors for value in actor]) == hashed,
                    "Adult actors changed")
            room_actors.append(actors)
        require((0x164, 628, 996, -1780, 0, 0, 0, 0) in room_actors[0], "King Zora was not retained")
        require((0xEF, 483, 53, 214, 0, 8192, 0, 0x304) in room_actors[1], "Shop red ice was not retained")
        require((0x95, -10, 897, -909, 16384, 0, 0, 0xB240) in room_actors[1], "Adult Skulltula changed")

        for path, expected in DISPLAYS.items():
            data = package.read(path)
            require(data[4:8] == b"TLDO" and data[64] == 4, "Unexpected native drawing format")
            require(words_hash(struct.unpack("<" + "I" * ((len(data) - 72) // 4), data[72:])) == expected,
                    "Native drawing dependencies changed")
        raw_vertices = package.read("objects/object_spot06_objects/object_spot06_objectsVtx_001120")
        require(raw_vertices[4:8] == b"RRAO" and struct.unpack_from("<II", raw_vertices, 64) == (25, 4),
                "Unexpected ice plug vertex resource")
        model = [struct.unpack_from("<3h", raw_vertices, 72 + index * 16) for index in range(4)]
        require(model == native_cap["vertices"], "Native cap visual/collision disagree")
        position, scale, yaw, vertices = policy_geometry()
        angle = yaw * math.pi / 32768
        for local, rounded in zip(model, vertices):
            x, y, z = [value * size for value, size in zip(local, scale)]
            rendered = (position[0] + math.cos(angle)*x + math.sin(angle)*z,
                        position[1] + y, position[2] - math.sin(angle)*x + math.cos(angle)*z)
            require(math.dist(rendered, rounded) < 0.75, "Drawing/collision closure mismatch")
        rays, caps = verify_closure(base, vertices)

        # All four supported adult arrivals keep their native floor/body clearance.
        footprints = [(0, 0), (20, 0), (-20, 0), (0, 20), (0, -20), (14, 14), (14, -14), (-14, 14), (-14, -14)]
        for index, expected_floor in enumerate((210, 996, 54, 947)):
            x, y, z = adult[0][index][1:4]
            require(adult[6][index] == (index, 1 if index in (0, 2) else 0), "Spawn-room mismatch")
            for dx, dz in footprints:
                require(abs(floor_at(base, x + dx, y, z + dz) - expected_floor) < 0.01, "Unsafe adult arrival footprint")
            for height in (25, 50, 75):
                point = (x, y + height, z)
                nearest = min(distance_to_triangle(point, triangle(base, poly)) for poly in base["polygons"])
                require(nearest >= 22.9, "Adult arrival body obstructed")
                require(min(distance_to_triangle(point, cap) for cap in caps) > 500, "Closure interferes with arrival")

        # Reproduce EnSw's initial wall-search probes, not just a nearby-triangle
        # distance. Its fourth probe attaches to permanent static polygon333.
        probes = [((-10, 915, -909), (-10, 879, -909)), ((-10, 879, -909), (-10, 879, -933)),
                  ((-10, 879, -909), (14, 879, -909)), ((-10, 879, -909), (-34, 879, -909))]
        for index, (start, end) in enumerate(probes):
            hits = [i for i, poly in enumerate(base["polygons"]) if segment_hits_triangle(start, end, triangle(base, poly))]
            require(hits == ([] if index < 3 else [333]), "Adult Skulltula lost its static support")
        skull = (-10, 897, -909)
        for ice, offset in ((pool_ice, (0, 0, 0)), (upper_ice, (445, 1008, -1742))):
            scaled = [[tuple(value * 0.1 + shift for value, shift in zip(vertex, offset))
                       for vertex in triangle(ice, poly)] for poly in ice["polygons"]]
            require(min(distance_to_triangle(skull, poly) for poly in scaled) > 110, "Skulltula depends on removed ice")
    print(f"Domain native resources passed: 4 collision fingerprints, 3 drawing manifests, "
          f"adult rooms/quests, 4 arrivals, Skulltula support, {rays} shortcut coverage rays in both directions.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    run(parser.parse_args().archive)
