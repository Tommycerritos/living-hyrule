"""Read-only native royal garden/return validation; usage: this_file.py PATH/oot.o2r.

Imports only the existing stdlib resource-reader helpers beside this test. No
assets are extracted or committed, and no game or renderer is launched.
"""
import argparse
import copy
import hashlib
import math
from pathlib import Path
import re
import zipfile

from MarketRestorationResourceTests import (
    Reader, collision, distance_to_triangle, fingerprint, native_group, rejects,
    require, triangle,
)

PREFIX = "scenes/shared/nakaniwa_scene/"
COLLISION = PREFIX + "nakaniwa_sceneCollisionHeader_001BC8"
EXPECTED_HASH = 0xFCCFFC007B957C78
GROUP_SHA256 = "7ef15cb5bc060d7473830a36cce5f8c673ec18f97c3b078ae7879188535788e3"
EMPTY_LABELS = {PREFIX + "nakaniwa_room_0ActorEntry_" + label
                for label in ("000070", "000164", "0001F4", "000284")}
ROOT = Path(__file__).resolve().parents[2]


def scene(data):
    reader, commands = Reader(data), {}
    count = reader.read("I")
    require(count <= 32, "Invalid scene command count")
    for _ in range(count):
        command = reader.read("I")
        require(command not in commands, "Duplicate native scene command")
        if command in (0, 1):
            value = reader.array("HhhhhhhH")
        elif command in (3, 23):
            value = reader.string()
        elif command == 4:
            value = [(reader.string(), reader.read("II")) for _ in range(reader.read("I"))]
        elif command in (6,):
            value = reader.array("BB")
        elif command == 7:
            value = reader.read("Bh")
        elif command in (8, 25):
            value = reader.read("BI")
        elif command in (11, 19):
            value = reader.array("H")
        elif command == 14:
            value = reader.array("4BhhhhHH")
        elif command == 15:
            value = reader.array("18Bhh")
        elif command in (16, 21):
            value = reader.read("3B")
        elif command == 17:
            value = reader.read("4B")
        elif command == 18:
            value = reader.read("2B")
        elif command == 22:
            value = reader.read("B")
        elif command == 24:
            value = [reader.string() for _ in range(reader.read("I"))]
        elif command == 10:
            unused, kind, pieces = reader.read("3B")
            require(kind == 0 and pieces == 1, "Unexpected garden mesh")
            value = [(reader.read("B"), reader.string(), reader.string()) for _ in range(pieces)]
        elif command == 20:
            value = None
        else:
            raise ValueError("Unsupported garden command " + str(command))
        commands[command] = value
    require(reader.offset == len(data), "Unexpected trailing scene data")
    return commands


def validate_header(commands):
    require(commands[24][:3] == ["", "", ""], "Ordinary adult header must use the base garden")
    require(14 not in commands and 23 not in commands, "Unexpected transition actors or automatic cutscene")


def floor(data, x, z, top):
    highest = None
    for index, poly in enumerate(data["polygons"]):
        if poly[5] < 26000:
            continue
        a, b, c = triangle(data, poly)
        denominator = (b[2] - c[2]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[2] - c[2])
        if not denominator:
            continue
        u = ((b[2] - c[2]) * (x - c[0]) + (c[0] - b[0]) * (z - c[2])) / denominator
        v = ((c[2] - a[2]) * (x - c[0]) + (a[0] - c[0]) * (z - c[2])) / denominator
        if min(u, v, 1 - u - v) < -1e-5:
            continue
        height = u * a[1] + v * b[1] + (1 - u - v) * c[1]
        if height <= top and (highest is None or height > highest[0]):
            highest = (height, index, data["surfaces"][poly[0]])
    require(highest is not None, "Missing supported floor")
    return highest


def check_body(data, position, radius=25, footprint=44, heights=(32, 60, 85), allow_exit=False):
    x, y, z = position
    ground = floor(data, x, z, y + 24)[0]
    require(abs(ground - y) <= 3, "Unsupported center height")
    diagonal = round(footprint * 0.727)
    for dx, dz in ((0, 0), (footprint, 0), (-footprint, 0), (0, footprint), (0, -footprint),
                   (diagonal, diagonal), (-diagonal, diagonal), (diagonal, -diagonal), (-diagonal, -diagonal)):
        edge, index, surface = floor(data, x + dx, z + dz, ground + 24)
        require(abs(edge - ground) <= 3, "Unsafe footprint ledge")
        require(allow_exit or not (surface[0] >> 8 & 31), "Resident stands on a scene exit")
        require(not (surface[0] >> 13 & 31) and not (surface[1] >> 18 & 7), "Hazardous/conveyor floor")
        for wx, wy, wz, width, depth, properties in data["water"]:
            require(not (wx <= x + dx <= wx + width and wz <= z + dz <= wz + depth and wy > edge + 2),
                    "Spawn footprint is submerged")
    for height in heights:
        point = (x, ground + height, z)
        require(all(distance_to_triangle(point, triangle(data, poly)) >= radius - 0.01
                    for poly in data["polygons"]), "Actor body intersects native geometry")


def validate_entrances():
    source = (ROOT / "soh/include/tables/entrance_table.h").read_text()
    expected = {
        0x400: ("SCENE_CASTLE_COURTYARD_ZELDA", 0),
        0x402: ("SCENE_CASTLE_COURTYARD_ZELDA", 0),
        0x403: ("SCENE_CASTLE_COURTYARD_ZELDA", 0),
        0x5F2: ("SCENE_CASTLE_COURTYARD_ZELDA", 1),
        0x5F3: ("SCENE_CASTLE_COURTYARD_ZELDA", 1),
        0x13A: ("SCENE_OUTSIDE_GANONS_CASTLE", 0),
        0x13B: ("SCENE_OUTSIDE_GANONS_CASTLE", 0),
    }
    for entry, (scene_id, spawn) in expected.items():
        pattern = rf"/\*\s*0x{entry:03X}\s*\*/[^\n]*\b{scene_id}\s*,\s*{spawn}\s*,"
        require(re.search(pattern, source, re.I), "Native age-indexed entrance changed: " + hex(entry))


def run(archive):
    with zipfile.ZipFile(archive) as package:
        names = native_group(package.namelist(), PREFIX, 105, 0x1CE2F8B9E056309A)
        whole_group = hashlib.sha256()
        for name in names:
            raw = package.read(name)
            whole_group.update(name.encode() + b"\0")
            whole_group.update(raw)
            require(not raw if name in EMPTY_LABELS else len(raw) > 64, "Missing/truncated garden dependency")
        require(whole_group.hexdigest() == GROUP_SHA256, "Native garden dependency payload changed")
        raw = package.read(COLLISION)
        data = collision(raw)
        require(fingerprint(data) == EXPECTED_HASH, "Native garden collision changed")
        for field, length in (("vertices", 236), ("polygons", 270), ("surfaces", 9), ("cameras", 1), ("water", 1)):
            require(len(data[field]) == length, "Native garden topology changed")
        require(data["cameras"] == [(0, 0, 0)], "Garden unexpectedly contains a forced camera")
        base = scene(package.read(PREFIX + "nakaniwa_scene"))
        room = scene(package.read(PREFIX + "nakaniwa_room_0"))
        validate_header(base)
        validate_header(room)
        require(base[3] == COLLISION and base[19] == [0x296, 0], "Native garden collision/exit route changed")
        require(base[6] == [(0, 0), (1, 0)] and len(base[4]) == 1 and base[4][0][0] == PREFIX + "nakaniwa_room_0",
                "Garden room/entrance mappings changed")
        require(room[10] == [(0, PREFIX + "nakaniwa_room_0DL_007178", PREFIX + "nakaniwa_room_0DL_014E98")],
                "Garden must use its normal opaque/translucent 3D mesh")
        require(room[16] == (255, 255, 0), "Visit must preserve the native frozen time")
        require(len(room[1]) == 9, "Original room actor inventory changed")
        # The whole native room actor list is removed before Init, including the
        # original Zelda, Impa, guards, ending viewer and wonder-item scripts.
        actor_table = (ROOT / "soh/include/tables/actor_table.h").read_text()
        actor_names = {int(number, 16): name for number, name in re.findall(
            r"/\*\s*0x([0-9A-F]+)\s*\*/\s*DEFINE_ACTOR\([^,]+,\s*([^,]+)", actor_table, re.I)}
        require([actor_names[row[0]] for row in room[1]] == [
            "ACTOR_EN_WONDER_ITEM", "ACTOR_EN_HEISHI2", "ACTOR_EN_HEISHI2", "ACTOR_DEMO_KANKYO",
            "ACTOR_DEMO_EFFECT", "ACTOR_EN_WONDER_ITEM", "ACTOR_EN_ZL4", "ACTOR_DEMO_IM", "ACTOR_EN_VIEWER"],
            "Native story actor inventory changed")
        require(base[0] == [(0, 604, 44, 8, 0, -16384, 0, 0xFFF), (0, -429, 84, 0, 0, -16384, 0, 0xDFF)],
                "Native garden spawns changed")
        for start in base[0]:
            check_body(data, start[1:4], radius=20, footprint=20, heights=(24, 50, 75))
        policy = (ROOT / "soh/soh/Enhancements/living-hyrule/RoyalEstatePolicy.h").read_text()
        positions = re.findall(r"\{\s*(-?\d+),\s*(-?\d+),\s*(-?\d+),\s*(-?\d+)\s*\}", policy)
        require(len(positions) == 3, "Missing canonical household positions")
        for x, y, z, yaw in positions:
            check_body(data, tuple(map(int, (x, y, z))))
        exits = [(index, poly) for index, poly in enumerate(data["polygons"])
                 if data["surfaces"][poly[0]][0] >> 8 & 31]
        require([index for index, _ in exits] == [28, 29], "Unexpected exit surfaces")
        corners = {vertex for _, poly in exits for vertex in triangle(data, poly)}
        require(corners == {(678, 44, -80), (678, 44, 80), (1037, 44, -80), (1037, 44, 80)},
                "The native east doorway no longer has the verified return floor")
        # Adult Link's body can walk from the initial spawn into the exit strip.
        for x in range(604, 701, 4):
            check_body(data, (x, 44, 8), radius=20, footprint=20, heights=(24, 50, 75), allow_exit=True)
        approach_path = "scenes/shared/ganon_tou_scene/ganon_tou_sceneCollisionHeader_002610"
        approach = collision(package.read(approach_path))
        check_body(approach, (-453, 1085, 3400), radius=20, footprint=20, heights=(24, 50, 75))
        # Meaningful negative fixtures: fail rather than accepting partial or
        # repointed resources that could restore native quest actors or exits.
        for length in (0, 63, 79, len(raw) - 1):
            rejects(lambda length=length: collision(raw[:length]), "Accepted truncated collision")
        changed = copy.deepcopy(data)
        changed["surfaces"][2] = (0, changed["surfaces"][2][1])
        require(fingerprint(changed) != EXPECTED_HASH, "Exit removal did not alter fingerprint")
        changed = copy.deepcopy(base)
        changed[24][1] = "unexpected adult story header"
        rejects(lambda: validate_header(changed), "Accepted an adult alternate story header")
        changed = copy.deepcopy(room)
        changed[14] = []
        rejects(lambda: validate_header(changed), "Accepted a pre-hook transition actor command")
        changed = copy.deepcopy(room)
        changed[23] = "story cutscene"
        rejects(lambda: validate_header(changed), "Accepted an automatic story cutscene")
        rejects(lambda: native_group(names[:-1], PREFIX, 105, 0x1CE2F8B9E056309A), "Accepted missing dependency")
    validate_entrances()
    print("Royal estate native resources: garden, adult spawns, household, exit walk, return and negative fixtures passed.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    run(parser.parse_args().archive)
