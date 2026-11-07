#!/usr/bin/env python3
"""Keep a battle moving for tests: when nothing changes, press Confirm (dialogue) and re-arm
Rush (auto-battle). Stops when the battle ends or after --max seconds.

    python autofight.py [--max 300]
"""
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gamewin import bring_to_front, find_window, is_foreground  # noqa: E402
from keys import press  # noqa: E402
from title import classify, grab  # noqa: E402
from p4gproc import Proc  # noqa: E402

BATTLE_MGR = 0x140EC08F0
BLOCK = 0x1451BCD70


def enemy_hp(p):
    bm = p.u64(BATTLE_MGR)
    if not bm:
        return None
    out, n = [], p.u64(bm + 0x1D8)
    for _ in range(8):
        if not n:
            break
        u = p.try_read(n + 0xCF0, 8)
        if not u:
            break
        unit = int.from_bytes(u, "little")
        hp = p.try_read(unit + 8, 2)
        out.append(int.from_bytes(hp, "little") if hp else -1)
        nx = p.try_read(n + 0xD00, 8)
        n = int.from_bytes(nx, "little") if nx else 0
    return tuple(out)


def main():
    limit = float(sys.argv[sys.argv.index("--max") + 1]) if "--max" in sys.argv else 300
    p = Proc()
    h = find_window()
    bring_to_front(h)
    start, last, idle = time.time(), None, 0
    while time.time() - start < limit:
        hp = enemy_hp(p)
        if hp is None:
            print(f"battle over after {time.time() - start:.0f}s")
            return 0
        party = [p.u16(BLOCK + 0xC74 + c * 0x84 + 8) for c in range(3)]
        if hp != last:
            print(f"{time.time() - start:5.0f}s enemies {hp} party {party}")
            last, idle = hp, 0
        else:
            idle += 1
        if not is_foreground(h):
            bring_to_front(h)
        if idle >= 2 and classify(grab(h, 0.25)) == "saveload":
            print("stopped: a save/load screen is showing")
            return 1
        if idle >= 2:
            press("space", 150)          # advance dialogue / confirm a prompt
            if idle % 4 == 0:
                press("tab", 150)        # re-arm Rush at the command menu
        time.sleep(1.5)
    print("time limit reached")
    return 1


if __name__ == "__main__":
    sys.exit(main())
