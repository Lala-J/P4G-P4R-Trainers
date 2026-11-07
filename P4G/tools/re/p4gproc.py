#!/usr/bin/env python3
"""Read-only access to a running P4G.exe for reverse engineering.

Opens the process with PROCESS_VM_READ | PROCESS_QUERY_INFORMATION only. Nothing here
can write to the game.

    python p4gproc.py info                       # module base, PE timestamp, sections
    python p4gproc.py read  <addr> [len]         # hex dump
    python p4gproc.py ptr   <addr> [depth]       # follow a pointer chain
    python p4gproc.py dis   <addr> [len]         # disassemble (needs capstone)
    python p4gproc.py scan  "<AOB pattern>" [section]   # e.g. "48 8B ?? ?? 90", section .shared
    python p4gproc.py refs  <target> [section]   # rel32 call/jmp/lea/mov references to target

Addresses accept 0x-hex or 'P4G.exe+0x1234'.
"""
import ctypes
import ctypes.wintypes as wt
import re
import struct
import sys

PROCESS_VM_READ = 0x0010
PROCESS_QUERY_INFORMATION = 0x0400
TH32CS_SNAPPROCESS = 0x2
TH32CS_SNAPMODULE = 0x8 | 0x10

k32 = ctypes.WinDLL("kernel32", use_last_error=True)


class PROCESSENTRY32W(ctypes.Structure):
    _fields_ = [("dwSize", wt.DWORD), ("cntUsage", wt.DWORD), ("th32ProcessID", wt.DWORD),
                ("th32DefaultHeapID", ctypes.c_void_p), ("th32ModuleID", wt.DWORD),
                ("cntThreads", wt.DWORD), ("th32ParentProcessID", wt.DWORD),
                ("pcPriClassBase", ctypes.c_long), ("dwFlags", wt.DWORD),
                ("szExeFile", ctypes.c_wchar * 260)]


class MODULEENTRY32W(ctypes.Structure):
    _fields_ = [("dwSize", wt.DWORD), ("th32ModuleID", wt.DWORD), ("th32ProcessID", wt.DWORD),
                ("GlblcntUsage", wt.DWORD), ("ProccntUsage", wt.DWORD),
                ("modBaseAddr", ctypes.c_void_p), ("modBaseSize", wt.DWORD),
                ("hModule", wt.HMODULE), ("szModule", ctypes.c_wchar * 256),
                ("szExePath", ctypes.c_wchar * 260)]


k32.CreateToolhelp32Snapshot.restype = wt.HANDLE
k32.OpenProcess.restype = wt.HANDLE
k32.ReadProcessMemory.argtypes = [wt.HANDLE, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t,
                                  ctypes.POINTER(ctypes.c_size_t)]


def find_pid(name="P4G.exe"):
    snap = k32.CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)
    e = PROCESSENTRY32W(); e.dwSize = ctypes.sizeof(e)
    ok = k32.Process32FirstW(snap, ctypes.byref(e))
    pid = None
    while ok:
        if e.szExeFile.lower() == name.lower():
            pid = e.th32ProcessID
            break
        ok = k32.Process32NextW(snap, ctypes.byref(e))
    k32.CloseHandle(snap)
    return pid


class Proc:
    def __init__(self, name="P4G.exe"):
        self.pid = find_pid(name)
        if not self.pid:
            raise SystemExit(f"{name} is not running")
        self.h = k32.OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, False, self.pid)
        if not self.h:
            raise SystemExit(f"OpenProcess failed ({ctypes.get_last_error()})")
        self.modules = self._modules()
        self.base = self.modules[name.lower()][0]
        self.sections = self._sections()

    def _modules(self):
        snap = k32.CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, self.pid)
        m = MODULEENTRY32W(); m.dwSize = ctypes.sizeof(m)
        out = {}
        ok = k32.Module32FirstW(snap, ctypes.byref(m))
        while ok:
            out[m.szModule.lower()] = (m.modBaseAddr, m.modBaseSize)
            ok = k32.Module32NextW(snap, ctypes.byref(m))
        k32.CloseHandle(snap)
        return out

    def read(self, addr, n):
        buf = ctypes.create_string_buffer(n)
        got = ctypes.c_size_t(0)
        if not k32.ReadProcessMemory(self.h, ctypes.c_void_p(addr), buf, n, ctypes.byref(got)):
            raise OSError(f"read 0x{addr:X}+{n} failed ({ctypes.get_last_error()})")
        return buf.raw[:got.value]

    def try_read(self, addr, n):
        try:
            return self.read(addr, n)
        except OSError:
            return None

    def u8(self, a): return self.read(a, 1)[0]
    def u16(self, a): return struct.unpack("<H", self.read(a, 2))[0]
    def u32(self, a): return struct.unpack("<I", self.read(a, 4))[0]
    def u64(self, a): return struct.unpack("<Q", self.read(a, 8))[0]

    def _sections(self):
        hdr = self.read(self.base, 0x1000)
        e_lfanew = struct.unpack_from("<I", hdr, 0x3C)[0]
        nsec = struct.unpack_from("<H", hdr, e_lfanew + 6)[0]
        opt = struct.unpack_from("<H", hdr, e_lfanew + 20)[0]
        self.timestamp = struct.unpack_from("<I", hdr, e_lfanew + 8)[0]
        self.size_of_image = struct.unpack_from("<I", hdr, e_lfanew + 24 + 56)[0]
        off = e_lfanew + 24 + opt
        secs = {}
        for i in range(nsec):
            s = hdr[off + i * 40: off + i * 40 + 40]
            name = s[:8].rstrip(b"\0").decode("latin-1")
            vsize, va = struct.unpack_from("<II", s, 8)
            secs[name] = (self.base + va, vsize)
        return secs

    def addr(self, text):
        text = text.strip()
        m = re.match(r"(?i)p4g\.exe\+(0x)?([0-9a-f]+)$", text)
        if m:
            return self.base + int(m.group(2), 16)
        return int(text, 16) if text.lower().startswith("0x") else int(text, 16)

    def scan(self, pattern, section=".shared", limit=50):
        toks = pattern.split()
        rx = b"".join(b"." if t in ("?", "??") else re.escape(bytes([int(t, 16)])) for t in toks)
        rx = re.compile(rx, re.DOTALL)
        start, size = self.sections[section]
        hits = []
        chunk = 0x100000
        for off in range(0, size, chunk):
            data = self.try_read(start + off, min(chunk + len(toks), size - off))
            if not data:
                continue
            for m in rx.finditer(data):
                if m.start() < chunk:
                    hits.append(start + off + m.start())
                    if len(hits) >= limit:
                        return hits
        return hits


def hexdump(base, data):
    for i in range(0, len(data), 16):
        row = data[i:i + 16]
        print(f"{base + i:016X}  {row.hex(' '):<47}  " +
              "".join(chr(c) if 32 <= c < 127 else "." for c in row))


def disasm(p, addr, n):
    try:
        import capstone
    except ImportError:
        raise SystemExit("pip install capstone")
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
    for ins in md.disasm(p.read(addr, n), addr):
        print(f"{ins.address:016X}  {ins.bytes.hex(' '):<30} {ins.mnemonic} {ins.op_str}")


def refs(p, target, section):
    """Find rel32 references (E8/E9 call/jmp, and RIP-relative disp32) to target."""
    start, size = p.sections[section]
    out = []
    chunk = 0x100000
    for off in range(0, size, chunk):
        data = p.try_read(start + off, min(chunk + 16, size - off))
        if not data:
            continue
        for i in range(0, min(chunk, len(data) - 4)):
            disp = struct.unpack_from("<i", data, i)[0]
            # disp32 ends at i+4 for call/jmp; for RIP-relative operands the instruction may
            # continue (imm), so accept end positions i+4..i+8 and let the caller verify.
            for tail in (0, 1, 4):
                if start + off + i + 4 + tail + disp == target:
                    out.append(start + off + i - 1)
    return out


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return
    p = Proc()
    cmd = argv[1]
    if cmd == "info":
        print(f"pid {p.pid}  base 0x{p.base:X}  timestamp 0x{p.timestamp:08X}  "
              f"SizeOfImage 0x{p.size_of_image:X}")
        for n, (a, s) in p.sections.items():
            print(f"  {n:<10} 0x{a:X}  size 0x{s:X}")
    elif cmd == "read":
        a = p.addr(argv[2]); n = int(argv[3], 0) if len(argv) > 3 else 0x80
        hexdump(a, p.read(a, n))
    elif cmd == "ptr":
        a = p.addr(argv[2]); depth = int(argv[3]) if len(argv) > 3 else 1
        for _ in range(depth):
            v = p.u64(a)
            print(f"[0x{a:X}] = 0x{v:X}")
            a = v
    elif cmd == "dis":
        disasm(p, p.addr(argv[2]), int(argv[3], 0) if len(argv) > 3 else 0x60)
    elif cmd == "scan":
        sec = argv[3] if len(argv) > 3 else ".shared"
        for h in p.scan(argv[2], sec):
            print(f"0x{h:X}  (P4G.exe+0x{h - p.base:X})")
    elif cmd == "refs":
        sec = argv[3] if len(argv) > 3 else ".shared"
        for h in refs(p, p.addr(argv[2]), sec):
            print(f"0x{h:X}")
    else:
        print(__doc__)


if __name__ == "__main__":
    main(sys.argv)
