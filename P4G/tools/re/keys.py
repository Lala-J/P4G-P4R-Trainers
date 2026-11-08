#!/usr/bin/env python3
"""Send keystrokes to the game (for driving menus during tests).

    python keys.py <key>[:hold_ms] [<key> ...] [--gap ms] [--front]

Keys: enter esc space up down left right insert pgup pgdn tab bksp f1..f12, a-z, 0-9.
P4G reads the keyboard through DirectInput, so keys are sent as hardware scan codes.
"""
import ctypes
import ctypes.wintypes as wt
import sys
import time

user32 = ctypes.WinDLL("user32", use_last_error=True)

SCAN = {
    "esc": 0x01, "enter": 0x1C, "space": 0x39, "tab": 0x0F, "bksp": 0x0E,
    "up": (0x48, True), "down": (0x50, True), "left": (0x4B, True), "right": (0x4D, True),
    "insert": (0x52, True), "pgup": (0x49, True), "pgdn": (0x51, True), "home": (0x47, True),
    "lshift": 0x2A, "lctrl": 0x1D,
}
for i, c in enumerate("1234567890"):
    SCAN[c] = 0x02 + i
for c, sc in zip("qwertyuiop", range(0x10, 0x1A)):
    SCAN[c] = sc
for c, sc in zip("asdfghjkl", range(0x1E, 0x27)):
    SCAN[c] = sc
for c, sc in zip("zxcvbnm", range(0x2C, 0x33)):
    SCAN[c] = sc
for i in range(1, 11):
    SCAN[f"f{i}"] = 0x3A + i
SCAN["f11"], SCAN["f12"] = 0x57, 0x58

KEYEVENTF_EXTENDEDKEY, KEYEVENTF_KEYUP, KEYEVENTF_SCANCODE = 0x1, 0x2, 0x8


class KEYBDINPUT(ctypes.Structure):
    _fields_ = [("wVk", wt.WORD), ("wScan", wt.WORD), ("dwFlags", wt.DWORD), ("time", wt.DWORD),
                ("dwExtraInfo", ctypes.c_size_t)]


class INPUT(ctypes.Structure):
    class _U(ctypes.Union):
        _fields_ = [("ki", KEYBDINPUT), ("pad", ctypes.c_byte * 32)]
    _anonymous_ = ("u",)
    _fields_ = [("type", wt.DWORD), ("u", _U)]


def send(scan, ext, up):
    flags = KEYEVENTF_SCANCODE | (KEYEVENTF_EXTENDEDKEY if ext else 0) | (KEYEVENTF_KEYUP if up else 0)
    inp = INPUT(type=1)
    inp.ki = KEYBDINPUT(0, scan, flags, 0, 0)
    user32.SendInput(1, ctypes.byref(inp), ctypes.sizeof(INPUT))


class NotInFront(RuntimeError):
    pass


def _ensure_game_front():
    """Every key goes to the game or nowhere: bring the game to the front if needed, and refuse
    if it can't be (so keystrokes never land in another application)."""
    import os
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    from gamewin import bring_to_front, find_window, is_foreground
    h = find_window()
    if is_foreground(h):
        return
    if not (h and bring_to_front(h) and is_foreground(h)):
        raise NotInFront("the game is not the foreground window; no key sent")


def press(name, hold_ms=200):
    v = SCAN[name.lower()]
    scan, ext = (v if isinstance(v, tuple) else (v, False))
    _ensure_game_front()
    send(scan, ext, False)
    time.sleep(hold_ms / 1000)
    send(scan, ext, True)


def main():
    args = sys.argv[1:]
    gap = 250
    if "--gap" in args:
        i = args.index("--gap")
        gap = int(args[i + 1])
        del args[i:i + 2]
    import os
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    from gamewin import bring_to_front, find_window, is_foreground
    h = find_window()
    if "--front" in args:
        args.remove("--front")
        bring_to_front(h)
    for a in args:
        # Safety: never type into another application.
        if not is_foreground(h):
            print("aborted: the game is not the foreground window (use --front)")
            return 1
        name, _, hold = a.partition(":")
        press(name, int(hold) if hold else 200)
        time.sleep(gap / 1000)
    return 0


if __name__ == "__main__":
    sys.exit(main())
