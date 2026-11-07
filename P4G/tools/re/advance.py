#!/usr/bin/env python3
"""Press Confirm repeatedly to advance dialogue until a condition holds (for test runs).

    python advance.py --until battle|field [--max N] [--gap seconds] [--key space]

battle: stops once the battle manager exists; field: once no battle is running.
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


def arg(name, default):
    return sys.argv[sys.argv.index(name) + 1] if name in sys.argv else default


def main():
    until = arg("--until", "battle")
    limit = int(arg("--max", "60"))
    gap = float(arg("--gap", "1.2"))
    key = arg("--key", "space")
    p = Proc()
    h = find_window()
    bring_to_front(h)
    for i in range(limit):
        in_battle = p.u64(BATTLE_MGR) != 0
        if (until == "battle" and in_battle) or (until == "field" and not in_battle and i > 0):
            print(f"condition '{until}' reached after {i} presses")
            return 0
        if not is_foreground(h):
            print("aborted: game lost focus")
            return 1
        # Never confirm on a save / load screen (the end-of-day save prompt overwrites a slot).
        if classify(grab(h, 0.25)) == "saveload":
            print("stopped: a save/load screen is showing; handle it by hand")
            return 1
        press(key, 150)
        time.sleep(gap)
    print("gave up")
    return 1


if __name__ == "__main__":
    sys.exit(main())
