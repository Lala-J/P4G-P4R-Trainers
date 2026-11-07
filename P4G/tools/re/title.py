#!/usr/bin/env python3
"""Drive P4G from the attract loop to the title menu (for automated tests).

    python title.py            # until the title menu is showing
    python title.py --load N   # then Load Game -> slot N (1-based), confirm

Screens are told apart by colour: the title menu is deep blue, the "press any button"
splash is yellow, anything else is treated as the opening movie (skippable with Enter).
"""
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from PIL import ImageGrab  # noqa: E402

import ctypes  # noqa: E402
import ctypes.wintypes as wt  # noqa: E402
from gamewin import bring_to_front, find_window, is_foreground, user32  # noqa: E402
from keys import press  # noqa: E402


def grab(hwnd, scale=0.1):
    r = wt.RECT()
    user32.GetClientRect(hwnd, ctypes.byref(r))
    pt = wt.POINT(0, 0)
    user32.ClientToScreen(hwnd, ctypes.byref(pt))
    img = ImageGrab.grab(bbox=(pt.x, pt.y, pt.x + r.right, pt.y + r.bottom), all_screens=True)
    return img.resize((max(1, int(img.width * scale)), max(1, int(img.height * scale))))


def _strip_yellow(img, y0, y1):
    w, h = img.size
    c = img.convert("RGB").crop((0, int(h * y0), w, max(int(h * y0) + 1, int(h * y1)))).resize((96, 12))
    raw = c.tobytes()
    px = [tuple(raw[i:i + 3]) for i in range(0, len(raw), 3)]
    return sum(1 for r, g, b in px if r > 200 and g > 150 and b < 120) / len(px)


def is_save_or_load(img):
    """The save / load screens have solid yellow strips at the top and bottom."""
    return _strip_yellow(img, 0.0, 0.12) > 0.8 and _strip_yellow(img, 0.88, 1.0) > 0.7


def classify(img):
    if is_save_or_load(img):
        return "saveload"
    raw = img.convert("RGB").tobytes()
    px = [tuple(raw[i:i + 3]) for i in range(0, len(raw), 3)]
    n = len(px)
    blue = sum(1 for r, g, b in px if b > 120 and b > r + 50 and b > g + 50) / n
    yellow = sum(1 for r, g, b in px if r > 200 and g > 190 and b < 110) / n
    if blue > 0.55:
        return "menu"
    if yellow > 0.55:
        return "splash"
    return "other"


def to_menu(hwnd, timeout=90):
    end = time.time() + timeout
    last_press = 0
    stable = 0
    while time.time() < end:
        if not is_foreground(hwnd):
            bring_to_front(hwnd)
        state = classify(grab(hwnd))
        if state == "menu":
            stable += 1
            if stable >= 3:          # the menu has finished animating in
                return True
        else:
            stable = 0
            # The splash ignores input while its banner sweeps in; keep pressing every ~1.2 s.
            if time.time() - last_press > 1.2:
                press("enter", 300)
                last_press = time.time()
        time.sleep(0.3)
    return False


def main():
    hwnd = find_window()
    if not hwnd or not bring_to_front(hwnd):
        print("game window not available")
        return 1
    if not to_menu(hwnd):
        print("did not reach the title menu")
        return 1
    print("title menu")
    if "--load" in sys.argv:
        slot = int(sys.argv[sys.argv.index("--load") + 1])
        press("enter", 250)                      # LOAD GAME is the default selection
        time.sleep(2.0)
        for _ in range(slot - 1):
            press("down", 150)
            time.sleep(0.25)
        press("enter", 250)
        time.sleep(1.0)
        print(f"selected slot {slot} (a confirmation prompt may follow)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
