"""Validate returning Domain residents against the owner's native oot.o2r.

Read-only: no asset extraction, game launch or third-party modules.
"""
import argparse
import math
from pathlib import Path
import re
import sys
import zipfile

sys.dont_write_bytecode = True
from ZoraRestorationResourceTests import PREFIX, collision, fingerprint, scene, floor_at
from MarketRestorationResourceTests import require, triangle, distance_to_triangle

ROOT = Path(__file__).resolve().parents[2]
FOOTPRINT = [(0, 0), (32, 0), (-32, 0), (0, 32), (0, -32), (22, 22), (22, -22), (-22, 22), (-22, -22)]


def dry(data, x, y, z):
    for low_x, height, low_z, width, depth, properties in data["water"]:
        room = (properties >> 13) & 63
        if room not in (1, 63) or properties & 0x80000:
            continue
        require(not (low_x < x < low_x + width and low_z < z < low_z + depth and height > y + 2),
                "Resident or conversation footprint is submerged")
    # z_bgcheck.c has an additional height-sensitive waterfall volume. Preserve
    # its native exclusion too, rather than relying solely on room water boxes.
    require(not (-348 < x < 205 and 777 < y < 977 and -1746 < z < -967 and 877 > y + 2),
            "Footprint enters the special upper waterfall volume")


def floor_and_body(data, position, footprint, radius):
    x, authored_y, z = position
    y = floor_at(data, x, authored_y, z)
    require(abs(y - authored_y) <= 1, "Authored floor moved")
    for dx, dz in footprint:
        height = floor_at(data, x + dx, y, z + dz)
        require(abs(height - y) <= 8, "Footprint crosses a steep edge or drop")
        dry(data, x + dx, height, z + dz)
    minimum = min(distance_to_triangle((x, y + height, z), triangle(data, poly))
                  for height in (24, 60, 85) for poly in data["polygons"])
    require(minimum >= radius, "Body intersects permanent geometry")
    return y, minimum


def run(archive):
    source = (ROOT / "soh/soh/Enhancements/living-hyrule/WaterDesertPolicy.h").read_text(encoding="utf-8")
    block = re.search(r"kDomainResidentPlacements\s*\{\s*\{(.*?)\}\s*\};", source, re.S)
    require(block is not None, "Missing Domain placements")
    matches = re.findall(r"\{\s*WaterDesertResidentId::(\w+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+)\s*\}",
                         block.group(1))
    require([row[0] for row in matches] == ["Lethra", "Neris"], "Unexpected or duplicate returning identities")
    with zipfile.ZipFile(archive) as package:
        data = collision(package.read(PREFIX + "spot07_sceneCollisionHeader_003824"))
        require(fingerprint(data) == 0xFB86681709703C4D, "Domain native collision changed")
        adult = scene(package.read(PREFIX + "spot07_sceneSet_003A40"))
        room = scene(package.read(PREFIX + "spot07_room_1Set_000360"))
        require((0xEF, 483, 53, 214, 0, 8192, 0, 0x304) in room[1], "Expected original shop ice missing")
        require((0x95, -10, 897, -909, 16384, 0, 0, 0xB240) in room[1], "Expected original Skulltula missing")
        results = []
        for name, sx, sy, sz, syaw in matches:
            x, y, z, yaw = map(int, (sx, sy, sz, syaw))
            floor, clearance = floor_and_body(data, (x, y, z), FOOTPRINT, 22)
            # Every native actor is conservatively kept away at the resident's
            # level, even address-only environmental actors with no solid body.
            for actor in room[1]:
                if abs(actor[2] - floor) < 150:
                    require(math.hypot(actor[1] - x, actor[3] - z) >= 180,
                            "Resident crowds a native actor or quest object")
            for start, entry in zip(adult[0], adult[6]):
                if entry[1] == 1 and abs(start[2] - floor) < 150:
                    require(math.hypot(start[1] - x, start[3] - z) >= 200,
                            "Resident crowds an adult arrival")
            for transition in adult[14]:
                # Native transition actors have position in fields5..7.
                require(math.dist((x, floor, z), transition[5:8]) >= 300, "Resident crowds a room transition")
            for quest_position in ((628, 996, -1780), (483, 53, 214), (-10, 897, -909)):
                require(math.dist((x, floor, z), quest_position) >= 300, "Resident approaches King/red ice/Skulltula")
            # Link can stand 80 units in front, within native talk range105.
            # Probe the complete approach corridor, not only the NPC centre.
            angle = yaw * math.pi / 32768
            player_footprint = [(0, 0), (20, 0), (-20, 0), (0, 20), (0, -20),
                                (14, 14), (14, -14), (-14, 14), (-14, -14)]
            for along in (55, 65, 80, 95):
                px, pz = x + math.sin(angle) * along, z + math.cos(angle) * along
                py = floor_at(data, px, floor, pz)
                require(abs(py - floor) <= 8, "Talking approach crosses a stair/drop")
                floor_and_body(data, (px, py, pz), player_footprint, 20)
            results.append((name, floor, clearance))
        require(math.dist(tuple(map(float, matches[0][1:4])), tuple(map(float, matches[1][1:4]))) >= 300,
                "Returning residents crowd each other")
    for name, floor, clearance in results:
        print(f"{name}: dry floor {floor:.3f}, body clearance {clearance:.3f}; eight-point footprint and talking corridor pass.")
    print("Domain resident native checks passed: two existing identities; adult arrivals, room transitions, King, red ice and Skulltula clear.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    run(parser.parse_args().archive)
