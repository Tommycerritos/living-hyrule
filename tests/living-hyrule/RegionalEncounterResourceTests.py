"""Read-only encounter geometry/object checks against an owner's local oot.o2r.

Usage: python RegionalEncounterResourceTests.py PATH/oot.o2r
No extraction, external packages, downloaded assets or game launch.
"""
import argparse
import math
import re
import zipfile
from pathlib import Path

from MarketRestorationResourceTests import Reader, collision, distance_to_triangle, require, triangle


ROOT = Path(__file__).resolve().parents[2]
CASES = {
    "Trail": ("spot16", "003D10", 3, 0x16),
    "River": ("spot03", "006580", 1, 0x16),
    "Colossus": ("spot11", "004EE4", 2, 0x17),
}
FOOTPRINT = ((0, 0), (50, 0), (-50, 0), (0, 50), (0, -50), (36, 36), (-36, 36), (36, -36), (-36, -36))


def commands(data):
    """Decode ordinary outdoor scene headers using the native importer layout."""
    reader, result = Reader(data), {}
    for _ in range(reader.read("I")):
        command = reader.read("I")
        require(command not in result, "Duplicate native command")
        if command == 4:
            value = [(reader.string(), reader.read("II")) for _ in range(reader.read("I"))]
        elif command == 3:
            value = reader.string()
        elif command in (0, 1):
            value = reader.array("HhhhhhhH")
        elif command == 5:
            value = reader.read("4B")
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
            unused, mesh_type, count = reader.read("BBB")
            require(mesh_type in (0, 2), "Encounter room is not an ordinary outdoor mesh")
            value = []
            for _ in range(count):
                reader.read("B")
                if mesh_type == 2:
                    reader.read("4h")
                value.append((reader.string(), reader.string()))
        elif command == 20:
            value = None
        else:
            raise ValueError("Unsupported outdoor scene command " + str(command))
        result[command] = value
    require(reader.offset == len(data), "Unexpected trailing scene bytes")
    return result


def selected_room(archive, prefix, layer):
    base = commands(archive.read(prefix + "_room_0"))
    if layer == 0:
        return base
    path = base[24][layer - 1]
    if not path and layer == 3:
        path = base[24][1]
    return commands(archive.read(path)) if path else base


def floor_at(data, x, z, ceiling):
    hits = []
    for poly in data["polygons"]:
        if poly[5] < 30000:
            continue
        a, b, c = triangle(data, poly)
        denominator = (b[2] - c[2]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[2] - c[2])
        if denominator == 0:
            continue
        first = ((b[2] - c[2]) * (x - c[0]) + (c[0] - b[0]) * (z - c[2])) / denominator
        second = ((c[2] - a[2]) * (x - c[0]) + (a[0] - c[0]) * (z - c[2])) / denominator
        third = 1 - first - second
        if min(first, second, third) < -1e-6:
            continue
        height = first * a[1] + second * b[1] + third * c[1]
        if height <= ceiling:
            hits.append((height, poly))
    require(hits, "Missing static floor")
    return max(hits, key=lambda item: item[0])


def native_category(actor_id):
    table = (ROOT / "soh/include/tables/actor_table.h").read_text(encoding="utf-8")
    match = re.search(r"/\* 0x%04X \*/ DEFINE_ACTOR(?:_INTERNAL)?\((\w+)," % actor_id, table, re.IGNORECASE)
    require(match, "Unknown native actor identity " + hex(actor_id))
    name = match[1]
    path = ROOT / "soh/src/overlays/actors" / ("ovl_" + name) / ("z_" + name.lower() + ".c")
    if not path.exists():
        path = ROOT / "soh/src/code" / ("z_" + name.lower() + ".c")
    if name == "En_A_Obj":
        path = ROOT / "soh/src/code/z_en_a_keep.c"
    source = path.read_text(encoding="utf-8")
    category = re.search(r"const ActorInit \w+\s*=\s*\{\s*ACTOR_\w+,\s*(ACTORCAT_\w+)", source)
    require(category, "Missing native actor profile " + name)
    return category[1]


def validate_site(archive, name, values):
    x, y, z, floor_y, leash, lifetime = values
    scene, collision_suffix, layer, object_id = CASES[name]
    prefix = "scenes/shared/" + scene + "_scene/" + scene
    room = selected_room(archive, prefix, layer)
    require(object_id in room[11], name + " lacks its actual native object bank")
    if name == "River":
        require(object_id not in selected_room(archive, prefix, 2)[11], "Recheck adult River object limitation")
    data = collision(archive.read(prefix + "_sceneCollisionHeader_" + collision_suffix))
    height, _ = floor_at(data, x, z, floor_y + 40)
    require(abs(height - floor_y) <= 1, name + " authored floor height changed")
    samples = []
    for dx, dz in FOOTPRINT:
        edge, poly = floor_at(data, x + dx, z + dz, floor_y + 40)
        surface = data["surfaces"][poly[0]][0]
        require(abs(edge - height) <= 8, name + " footprint crosses a ledge or steep slope")
        require((surface >> 8 & 31) == 0 and (surface >> 18 & 7) == 0 and not (poly[2] & 0x2000),
                name + " footprint crosses an exit, special floor or conveyor")
        floor_type = surface >> 13 & 31
        require(floor_type in (4, 7) if name == "Colossus" else floor_type == 0, "Wrong native floor type")
        water = [row for row in data["water"] if row[0] < x + dx < row[0] + row[3]
                 and row[2] < z + dz < row[2] + row[4]]
        if name == "River":
            require(any(abs(row[1] - y) <= 1 and 10 <= row[1] - edge <= 80 for row in water),
                    "Blue Tektite footprint lacks its bounded native water surface")
        else:
            require(all(row[1] < edge - 2 for row in water), name + " dry pocket is submerged")
        samples.append(edge)
    base = y if name == "River" else height
    clearance = min(distance_to_triangle((x, base + dy, z), triangle(data, poly))
                    for poly in data["polygons"] for dy in (35, 75, 110))
    require(clearance > 30, name + " body intersects native collision")
    for actor in room[1]:
        actor_id, ax, ay, az = actor[:4]
        if actor_id in (0x112, 0x26, 0xa7):  # Non-solid rupee/wind triggers; separately guarded native spawner.
            continue
        category = native_category(actor_id)
        hostile = category in ("ACTORCAT_ENEMY", "ACTORCAT_BOSS") or actor_id == 0x1c
        important = category in ("ACTORCAT_NPC", "ACTORCAT_DOOR")
        if abs(ay - y) > (250 if hostile or important else 160):
            continue
        minimum = 600 if hostile else 400 if important else 250 if category == "ACTORCAT_EXPLOSIVE" else 100
        require(math.hypot(ax - x, az - z) >= minimum, name + " offset conflicts with native actor " + hex(actor_id))
    scene_header = commands(archive.read(prefix + "_scene"))
    for start in scene_header[0]:
        require(abs(start[2] - y) > 200 or math.hypot(start[1] - x, start[3] - z) > 400,
                name + " pocket obstructs a native arrival point")
    if name == "Trail":
        lower, _ = floor_at(data, -2000, z, 500)
        require(y - lower > 600, "Trail overlook is no longer separate from the lower required ascent")
    if name == "Colossus":
        require(z - leash >= 350 and lifetime + 20 < 200, "Desert route/cooldown margins changed")
        spawners = [actor for actor in room[1] if actor[0] == 0xa7]
        require(len(spawners) == 1 and spawners[0][-1] >> 11 == 0, "Unexpected native desert spawner")
    print(name + ": loaded object, nine floor/water probes, body, native actors and arrival clearance passed")


def validate_resources(archive):
    for family, skeleton, joints in (("object_tite", "object_tite_Skel_003A20", 25),
                                      ("object_reeba", "object_reeba_Skel_001EE8", 18)):
        header = (ROOT / "soh/assets/objects" / family / (family + ".h")).read_text(encoding="utf-8")
        paths = re.findall(r'"__OTR__([^\"]+)"', header)
        require(paths, "Missing native resource declarations")
        for path in paths:
            require(path in archive.namelist() and len(archive.read(path)) > 64, "Missing native enemy asset " + path)
        reader = Reader(archive.read("objects/" + family + "/" + skeleton))
        reader.read("BB")
        limb_count = reader.read("I")
        reader.read("IB")
        count = reader.read("I")
        require(limb_count + 1 == joints and count == limb_count, "Native skeleton no longer fits embedded pose arrays")
        for _ in range(count):
            path = reader.string()
            require(path in archive.namelist() and len(archive.read(path)) > 64, "Missing native skeleton limb")
        require(reader.offset == len(reader.data), "Unexpected skeleton data")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("archive", type=Path)
    args = parser.parse_args()
    source = (ROOT / "soh/soh/Enhancements/living-hyrule/RegionalEncounterPolicy.h").read_text(encoding="utf-8")
    rows = re.findall(r"\{\s*EncounterPlace::(\w+),\s*RegionalEnemyKind::\w+,\s*"
                      r"(-?\d+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+),\s*(\d+),\s*(\d+)\s*\}", source)
    require({row[0] for row in rows} == set(CASES) and len(rows) == 3, "Recheck deployed encounter definitions")
    with zipfile.ZipFile(args.archive) as archive:
        validate_resources(archive)
        for row in rows:
            validate_site(archive, row[0], tuple(float(value) for value in row[1:]))
    print("Native encounter resource and placement checks passed; no game was started")


if __name__ == "__main__":
    main()
