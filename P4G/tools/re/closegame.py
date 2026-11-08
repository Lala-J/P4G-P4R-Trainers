#!/usr/bin/env python3
"""Close P4G gracefully: ask its window to close, then confirm the game's own "End game?" box.

    python closegame.py [--wait 20]

Nothing is saved; the game exits exactly as if the player closed the window and chose OK.
The process is never killed.
"""
import ctypes
import ctypes.wintypes as wt
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gamewin import find_window, user32  # noqa: E402

WM_CLOSE, WM_COMMAND, IDOK = 0x0010, 0x0111, 1


def game_pid(hwnd):
    pid = wt.DWORD()
    user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
    return pid.value


def find_dialog(pid):
    """A standard dialog box (#32770) owned by the game process."""
    found = []

    @ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    def cb(h, _):
        cls = ctypes.create_unicode_buffer(64)
        user32.GetClassNameW(h, cls, 64)
        if cls.value == "#32770" and user32.IsWindowVisible(h) and game_pid(h) == pid:
            found.append(h)
        return True

    user32.EnumWindows(cb, 0)
    return found[0] if found else None


def main():
    wait = 20
    if "--wait" in sys.argv:
        wait = int(sys.argv[sys.argv.index("--wait") + 1])
    h = find_window()
    if not h:
        print("game window not found")
        return 1
    pid = game_pid(h)
    user32.PostMessageW(h, WM_CLOSE, 0, 0)
    until = time.time() + wait
    while time.time() < until:
        d = find_dialog(pid)
        if d:
            user32.PostMessageW(d, WM_COMMAND, IDOK, 0)
            print("confirmed the game's exit dialog")
            break
        time.sleep(0.3)
    while time.time() < until:
        if not find_window():
            print("game closed")
            return 0
        time.sleep(0.5)
    print("the game is still running")
    return 1


if __name__ == "__main__":
    sys.exit(main())
