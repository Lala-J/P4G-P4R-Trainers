#!/usr/bin/env python3
"""Print the current dungeon floor's layout (from the game's minimap grid) with the party's cell.

    python mapview.py

Rows are the minor axis (world x / 1200), columns the major axis (world z / 1200).
'#' wall, '.' walkable, '@' the party, ' ' outside the floor. Read-only.
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from p4gproc import Proc  # noqa: E402

GRID, ROOT = 0x1411AB39C, 0x140EC0FF0


def main():
    p = Proc()
    d = p.read(GRID, 24 * 16 * 16)
    q = lambda a: struct.unpack("<Q", p.read(a, 8))[0]
    try:
        pl = q(q(q(q(ROOT) + 0x250) + 0x48) + 0x18)
        x, z = struct.unpack("<f", p.read(pl + 0x360, 4))[0], struct.unpack("<f", p.read(pl + 0x368, 4))[0]
        me = (int((z + 600) / 1200), int((x + 600) / 1200))
    except Exception:
        me = None
    for mi in range(16):
        row = ""
        for ma in range(24):
            f = d[(ma * 16 + mi) * 16]
            row += "@" if me == (ma, mi) else {0: " ", 1: ".", 2: "#"}.get(f, "?")
        print(row)
    print("party cell (major, minor):", me)


if __name__ == "__main__":
    main()
