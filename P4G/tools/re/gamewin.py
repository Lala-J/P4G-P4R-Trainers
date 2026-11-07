"""Find the P4G window and bring it to the foreground reliably."""
import ctypes
import ctypes.wintypes as wt
import time

user32 = ctypes.WinDLL("user32", use_last_error=True)
kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
user32.SetProcessDPIAware()

TITLE_PREFIX = "Persona4 GOLDEN"


def find_window():
    found = []

    @ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    def cb(hwnd, _):
        n = user32.GetWindowTextLengthW(hwnd)
        if n and user32.IsWindowVisible(hwnd):
            buf = ctypes.create_unicode_buffer(n + 1)
            user32.GetWindowTextW(hwnd, buf, n + 1)
            if buf.value.startswith(TITLE_PREFIX):
                found.append(hwnd)
        return True

    user32.EnumWindows(cb, 0)
    return found[0] if found else None


def is_foreground(hwnd):
    return hwnd is not None and user32.GetForegroundWindow() == hwnd


def bring_to_front(hwnd, tries=5):
    """Defeat the foreground lock: a synthetic Alt press plus AttachThreadInput."""
    if not hwnd:
        return False
    for _ in range(tries):
        if is_foreground(hwnd):
            return True
        fg = user32.GetForegroundWindow()
        fg_tid = user32.GetWindowThreadProcessId(fg, None)
        me = kernel32.GetCurrentThreadId()
        attached = fg_tid and fg_tid != me and user32.AttachThreadInput(me, fg_tid, True)
        user32.keybd_event(0x12, 0, 0, 0)          # Alt down
        if user32.IsIconic(hwnd):
            user32.ShowWindow(hwnd, 9)              # SW_RESTORE
        user32.BringWindowToTop(hwnd)
        user32.SetForegroundWindow(hwnd)
        user32.keybd_event(0x12, 0, 2, 0)          # Alt up
        if attached:
            user32.AttachThreadInput(me, fg_tid, False)
        time.sleep(0.3)
    return is_foreground(hwnd)
