#!/usr/bin/env python3
"""Atlus flowscript (.bf, FLW0) reader for the PC build: procedures, instructions, and every
script-function call with its constant arguments.

    python bf.py dis <file.bf | cpk:path>        # readable listing
    python bf.py calls <file.bf | cpk:path>      # one line per call: proc  FUNC(args)

Used by the miners; reads only. Function names/argument counts come from Atlus-Script-Tools'
P4G library (downloaded to cache/lib by fetch_lib()).
"""
import json
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
LIB = os.path.join(HERE, "cache", "lib")
LIB_URL = ("https://raw.githubusercontent.com/tge-was-taken/Atlus-Script-Tools/master/Source/"
           "AtlusScriptLibrary/Libraries/Persona4Golden")

OPS = ["PUSHI", "PUSHF", "PUSHIX", "PUSHIF", "PUSHREG", "POPIX", "POPFX", "PROC", "COMM", "END",
       "JUMP", "CALL", "RUN", "GOTO", "ADD", "SUB", "MUL", "DIV", "MINUS", "NOT", "OR", "AND", "EQ",
       "NEQ", "S", "L", "SE", "LE", "IF", "PUSHIS", "PUSHLIX", "PUSHLFX", "POPLIX", "POPLFX", "PUSHSTR"]


def _load_json(path):
    text = open(path, encoding="utf-8-sig").read()
    return json.loads(re.sub(r",(\s*[\]}])", r"\1", text))  # the library has trailing commas


def fetch_lib():
    os.makedirs(LIB, exist_ok=True)
    import urllib.request
    for m in ("Common", "Field", "AI", "Event", "Facility", "Net"):
        dst = os.path.join(LIB, m + ".json")
        if not os.path.exists(dst):
            urllib.request.urlretrieve(f"{LIB_URL}/Modules/{m}/Functions.json", dst)


_FUNCS = None


def functions():
    """{id: (name, argc)}"""
    global _FUNCS
    if _FUNCS is None:
        fetch_lib()
        _FUNCS = {}
        for m in ("Common", "Field", "AI", "Event", "Facility", "Net"):
            for f in _load_json(os.path.join(LIB, m + ".json")):
                _FUNCS[int(f["Index"], 16)] = (f["Name"], len(f.get("Parameters") or []))
    return _FUNCS


def fname(fid):
    f = functions().get(fid)
    return f[0] if f else f"FUNC_{fid:04X}"


class Script:
    def __init__(self, data, name=""):
        self.name = name
        if data[8:12] != b"FLW0":
            raise ValueError(f"{name}: not a flowscript")
        nsec = struct.unpack_from("<I", data, 0x10)[0]
        self.procs, self.labels, self.code, self.strings = [], [], [], b""
        self.msg = b""
        for i in range(nsec):
            typ, esz, cnt, off = struct.unpack_from("<IIII", data, 0x20 + i * 16)
            body = data[off: off + esz * cnt]
            if typ in (0, 1):
                lst = [(body[j * esz: j * esz + 24].split(b"\0")[0].decode("latin-1"),
                        struct.unpack_from("<I", body, j * esz + 24)[0]) for j in range(cnt)]
                (self.procs if typ == 0 else self.labels).extend(lst)
            elif typ == 2:
                self.code = [struct.unpack_from("<Hh", body, j * 4) for j in range(cnt)]
                self.raw = body
            elif typ == 3:
                self.msg = body
            elif typ == 4:
                self.strings = body

    def proc_of(self, ip):
        best = None
        for n, start in self.procs:
            if start <= ip and (best is None or start > best[1]):
                best = (n, start)
        return best[0] if best else "?"

    def instructions(self):
        """Yields (ip, op, operand) with PUSHI/PUSHF immediates folded in."""
        ip = 0
        while ip < len(self.code):
            op, short = self.code[ip]
            name = OPS[op] if op < len(OPS) else f"OP{op}"
            if name in ("PUSHI", "PUSHF"):
                raw = struct.unpack_from("<I" if name == "PUSHI" else "<f", self.raw, (ip + 1) * 4)[0]
                yield ip, name, raw
                ip += 2
                continue
            yield ip, name, short
            ip += 1

    def calls(self):
        """Yields (proc, ip, func_id, [args]) — args are ints when constant, else None."""
        stack = []
        for ip, op, v in self.instructions():
            if op in ("PUSHI", "PUSHIS"):
                stack.append(v)
            elif op in ("PUSHF", "PUSHIX", "PUSHIF", "PUSHREG", "PUSHLIX", "PUSHLFX", "PUSHSTR"):
                stack.append(None)
            elif op in ("ADD", "SUB", "MUL", "DIV", "OR", "AND", "EQ", "NEQ", "S", "L", "SE", "LE"):
                b = stack.pop() if stack else None
                a = stack.pop() if stack else None
                # Flags are written as section base + offset (e.g. 783 + 1024 + 0): fold them.
                val = None
                if a is not None and b is not None and op in ("ADD", "SUB", "MUL", "DIV"):
                    val = {"ADD": a + b, "SUB": a - b, "MUL": a * b,
                           "DIV": a // b if b else None}[op]
                stack.append(val)
            elif op in ("MINUS", "NOT"):
                if stack:
                    stack[-1] = None
            elif op in ("POPIX", "POPFX", "POPLIX", "POPLFX", "IF"):
                if stack:
                    stack.pop()
            elif op == "COMM":
                fid = v & 0xFFFF
                argc = functions().get(fid, (None, 0))[1]
                args = [stack.pop() if stack else None for _ in range(argc)]
                yield self.proc_of(ip), ip, fid, args
            elif op in ("PROC", "END", "JUMP", "GOTO"):
                stack = []


def load(spec):
    """A path on disk, or 'cpk:<path>' / '<cpkname>:<path>' read straight from the game."""
    if ":" in spec and not os.path.exists(spec) and not re.match(r"^[A-Za-z]:\\", spec):
        cpk_name, path = spec.split(":", 1)
        sys.path.insert(0, HERE)
        from cpk import open_cpk
        c = open_cpk("data_e.cpk" if cpk_name == "cpk" else cpk_name)
        return Script(c.read(path), path)
    return Script(open(spec, "rb").read(), os.path.basename(spec))


def fmt_args(args):
    return ", ".join("?" if a is None else str(a) for a in args)


def main():
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    cmd, spec = sys.argv[1], sys.argv[2]
    s = load(spec)
    if cmd == "calls":
        for proc, ip, fid, args in s.calls():
            print(f"{proc:<24} {ip:6d}  {fname(fid)}({fmt_args(args)})")
    elif cmd == "dis":
        starts = {i: n for n, i in s.procs}
        labels = {i: n for n, i in s.labels}
        for ip, op, v in s.instructions():
            if ip in starts:
                print(f"\nPROC {starts[ip]}:")
            if ip in labels:
                print(f"  {labels[ip]}:")
            extra = f"  ; {fname(v & 0xFFFF)}" if op == "COMM" else ""
            print(f"    {ip:6d} {op:<8} {v}{extra}")


if __name__ == "__main__":
    main()
