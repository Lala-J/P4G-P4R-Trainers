#!/usr/bin/env python3
"""Read-only CRI CPK reader (UTF tables + CRILAYLA), for mining the player's own game files.

    python cpk.py list  [--cpk data.cpk] [--grep PATTERN]
    python cpk.py get   <path-in-cpk> [--cpk data.cpk] [--out DIR]

The game folder is found through Steam (libraryfolders.vdf), or set P4G_DIR / --game-dir.
Extracted files go to tools/datamine/cache/ (git-ignored). Nothing is ever written to the game.
"""
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
CACHE = os.path.join(HERE, "cache")


# ------------------------------------------------------------------ locating the game
def find_game_dir():
    if os.environ.get("P4G_DIR"):
        return os.environ["P4G_DIR"]
    if "--game-dir" in sys.argv:
        return sys.argv[sys.argv.index("--game-dir") + 1]
    import winreg
    k = winreg.OpenKey(winreg.HKEY_CURRENT_USER, r"Software\Valve\Steam")
    steam = winreg.QueryValueEx(k, "SteamPath")[0]
    libs = [steam]
    vdf = os.path.join(steam, "steamapps", "libraryfolders.vdf")
    if os.path.exists(vdf):
        text = open(vdf, encoding="utf-8", errors="replace").read()
        libs += [p.replace("\\\\", "\\") for p in re.findall(r'"path"\s+"([^"]+)"', text)]
    for lib in libs:
        manifest = os.path.join(lib, "steamapps", "appmanifest_1113000.acf")
        if os.path.exists(manifest):
            m = re.search(r'"installdir"\s+"([^"]+)"', open(manifest, encoding="utf-8").read())
            if m:
                return os.path.join(lib, "steamapps", "common", m.group(1))
    raise SystemExit("Persona 4 Golden not found; set P4G_DIR")


# ------------------------------------------------------------------ UTF tables
def read_utf(buf, off):
    """Parse an @UTF table at buf[off:]; returns (name, rows as list of dicts)."""
    if buf[off:off + 4] != b"@UTF":
        raise ValueError(f"no @UTF at 0x{off:X}")
    size = struct.unpack_from(">I", buf, off + 4)[0]
    t = buf[off + 8: off + 8 + size]
    rows_off = struct.unpack_from(">H", t, 2)[0]
    str_off, data_off, name_off = struct.unpack_from(">III", t, 4)
    ncols, row_len, nrows = struct.unpack_from(">HHI", t, 16)

    def cstr(o):
        e = t.index(b"\0", str_off + o)
        return t[str_off + o:e].decode("utf-8", "replace")

    fmt = {0: ">B", 1: ">b", 2: ">H", 3: ">h", 4: ">I", 5: ">i", 6: ">Q", 7: ">q", 8: ">f", 9: ">d"}

    def read_val(typ, p):
        if typ in fmt:
            n = struct.calcsize(fmt[typ])
            return struct.unpack_from(fmt[typ], t, p)[0], n
        if typ == 0xA:
            return cstr(struct.unpack_from(">I", t, p)[0]), 4
        if typ == 0xB:
            do, dl = struct.unpack_from(">II", t, p)
            return (data_off + do, dl), 8
        raise ValueError(f"column type {typ}")

    cols, p = [], 24
    for _ in range(ncols):
        flags = t[p]
        name = cstr(struct.unpack_from(">I", t, p + 1)[0])
        p += 5
        storage, typ = flags & 0xF0, flags & 0x0F
        const = None
        if storage in (0x30, 0x70):
            const, n = read_val(typ, p)
            p += n
        cols.append((name, storage, typ, const))

    rows = []
    for r in range(nrows):
        p = rows_off + r * row_len
        row = {}
        for name, storage, typ, const in cols:
            if storage == 0x50:
                row[name], n = read_val(typ, p)
                p += n
            elif storage in (0x30, 0x70):
                row[name] = const
            else:
                row[name] = None
        rows.append(row)
    return cstr(name_off), rows


# ------------------------------------------------------------------ CRILAYLA
def crilayla(data):
    if data[:8] != b"CRILAYLA":
        return data
    usize, hdr = struct.unpack_from("<II", data, 8)
    out = bytearray(usize + 0x100)
    out[:0x100] = data[0x10 + hdr: 0x10 + hdr + 0x100]
    src = len(data) - 0x100 - 1
    pool = left = 0
    dst_end = 0x100 + usize - 1
    done = 0

    def bits(n):
        nonlocal pool, left, src
        v = got = 0
        while got < n:
            if left == 0:
                pool, left, src = data[src], 8, src - 1
            take = min(left, n - got)
            v = (v << take) | ((pool >> (left - take)) & ((1 << take) - 1))
            left -= take
            got += take
        return v

    vle = (2, 3, 5, 8)
    while done < usize:
        if bits(1):
            ref = dst_end - done + bits(13) + 3
            length = 3
            for i, w in enumerate(vle):
                x = bits(w)
                length += x
                if x != (1 << w) - 1:
                    break
            else:
                while True:
                    x = bits(8)
                    length += x
                    if x != 255:
                        break
            for _ in range(length):
                out[dst_end - done] = out[ref]
                ref -= 1
                done += 1
        else:
            out[dst_end - done] = bits(8)
            done += 1
    return bytes(out)


# ------------------------------------------------------------------ archive
class Cpk:
    def __init__(self, path):
        self.path = path
        self.f = open(path, "rb")
        head = self.f.read(0x800)
        if head[:4] != b"CPK ":
            raise ValueError("not a CPK")
        _, rows = read_utf(head, 0x10)
        self.header = rows[0]
        toc = self.header["TocOffset"]
        self.f.seek(toc)
        size = struct.unpack_from("<Q", self.f.read(16), 8)[0]
        tbl = self.f.read(size)
        _, self.files = read_utf(tbl, 0)
        self.toc = toc
        self.by_path = {}
        for r in self.files:
            p = (r["DirName"] + "/" if r["DirName"] else "") + r["FileName"]
            self.by_path[p.lower()] = r

    def names(self):
        return [((r["DirName"] + "/") if r["DirName"] else "") + r["FileName"] for r in self.files]

    def read(self, path):
        r = self.by_path[path.lower().replace("\\", "/")]
        self.f.seek(self.toc + r["FileOffset"])
        data = self.f.read(r["FileSize"])
        if r["FileSize"] < r["ExtractSize"]:
            data = crilayla(data)
        return data


def open_cpk(name="data.cpk"):
    return Cpk(os.path.join(find_game_dir(), name))


def main():
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    a = sys.argv[1:]
    name = a[a.index("--cpk") + 1] if "--cpk" in a else "data.cpk"
    c = open_cpk(name)
    if not a or a[0] == "list":
        pat = re.compile(a[a.index("--grep") + 1], re.I) if "--grep" in a else None
        for n in c.names():
            if not pat or pat.search(n):
                print(n)
    elif a[0] == "get":
        out = a[a.index("--out") + 1] if "--out" in a else CACHE
        data = c.read(a[1])
        dst = os.path.join(out, a[1].replace("/", os.sep))
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        open(dst, "wb").write(data)
        print(f"{a[1]} -> {dst} ({len(data)} bytes)")


if __name__ == "__main__":
    main()
