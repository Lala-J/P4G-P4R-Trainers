#!/usr/bin/env python3
"""Click inside the game window at client coordinates (for driving the trainer UI in tests).

    python click.py <x> <y> [--right] [--double]
    python click.py <x> <y> --wheel N     # scroll N notches there (negative = down)

Coordinates are in game-window client pixels (1920x1080 at native resolution).
Refuses to click unless the game is the foreground window.
"""
import ctypes
import ctypes.wintypes as wt
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gamewin import bring_to_front, find_window, is_foreground, user32  # noqa: E402

MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP = 0x2, 0x4
MOUSEEVENTF_RIGHTDOWN, MOUSEEVENTF_RIGHTUP = 0x8, 0x10


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    x, y = int(sys.argv[1]), int(sys.argv[2])
    h = find_window()
    if not h or not bring_to_front(h) or not is_foreground(h):
        print("aborted: the game is not the foreground window")
        return 1
    pt = wt.POINT(x, y)
    user32.ClientToScreen(h, ctypes.byref(pt))
    user32.SetCursorPos(pt.x, pt.y)
    time.sleep(0.25)  # let the UI see the hover before the press
    if "--wheel" in sys.argv:
        n = int(sys.argv[sys.argv.index("--wheel") + 1])
        for _ in range(abs(n)):
            user32.mouse_event(0x0800, 0, 0, 120 if n > 0 else -120, 0)  # MOUSEEVENTF_WHEEL
            time.sleep(0.05)
        return 0
    down, up = ((MOUSEEVENTF_RIGHTDOWN, MOUSEEVENTF_RIGHTUP) if "--right" in sys.argv
                else (MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP))
    for _ in range(2 if "--double" in sys.argv else 1):
        user32.mouse_event(down, 0, 0, 0, 0)
        time.sleep(0.1)
        user32.mouse_event(up, 0, 0, 0, 0)
        time.sleep(0.08)
    return 0


if __name__ == "__main__":
    sys.exit(main())
