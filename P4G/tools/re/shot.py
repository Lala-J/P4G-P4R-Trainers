#!/usr/bin/env python3
"""Screenshot the P4G window (for checking the overlay and in-game results).

    python shot.py <out.png> [--scale 0.5] [--front]

--front brings the game window to the foreground first (needed if something covers it).
Captures the window's client area from the screen, so the game must be visible.
"""
import ctypes
import ctypes.wintypes as wt
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from PIL import ImageGrab  # noqa: E402

from gamewin import bring_to_front, find_window, is_foreground, user32  # noqa: E402


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    scale = float(sys.argv[sys.argv.index("--scale") + 1]) if "--scale" in sys.argv else 1.0
    hwnd = find_window()
    if not hwnd:
        print("game window not found")
        return 1
    if "--front" in sys.argv and not bring_to_front(hwnd):
        print("could not bring the game to the foreground")
        return 1
    if not is_foreground(hwnd):
        print("warning: the game is not the foreground window; the capture may show something else")
    r = wt.RECT()
    user32.GetClientRect(hwnd, ctypes.byref(r))
    pt = wt.POINT(0, 0)
    user32.ClientToScreen(hwnd, ctypes.byref(pt))
    box = (pt.x, pt.y, pt.x + r.right, pt.y + r.bottom)
    img = ImageGrab.grab(bbox=box, all_screens=True)
    if scale != 1.0:
        img = img.resize((int(img.width * scale), int(img.height * scale)))
    img.save(out)
    print(f"saved {out} ({img.width}x{img.height})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
