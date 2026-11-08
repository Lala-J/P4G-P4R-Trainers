#!/usr/bin/env python3
"""Find memory that changes only when the player explores (e.g. dungeon minimap reveal data).

    python explorediff.py "<walk 1>" "<walk 2>" [...]

Each walk is a keys.py argument string (e.g. "w:1500" or "d:800 w:1200"). Between walks the
player stands still for the same time: anything that changes while idle is noise. Kept: bytes
that went 0 -> x during a walk and never changed again. Read-only.
"""
import bisect
import ctypes
import os
import subprocess
import sys
import time
from collections import defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from p4gproc import MBI, Proc, k32  # noqa: E402

PAGE = 4096


def private_rw(p):
    out, a, m = [], 0x10000, MBI()
    while a < 0x7FFFFFFF0000 and k32.VirtualQueryEx(p.h, ctypes.c_void_p(a), ctypes.byref(m), ctypes.sizeof(m)):
        base = m.BaseAddress or 0
        if m.State == 0x1000 and m.Protect in (0x04, 0x08) and m.Type == 0x20000 and m.RegionSize <= (64 << 20):
            out.append((base, m.RegionSize))
        a = base + m.RegionSize
    return out


def snapshot(p, regs):
    return {base: d for base, size in regs if (d := p.try_read(base, size))}


def changes(a_snap, b_snap):
    """{addr: (old, new)} for every byte that differs."""
    out = {}
    for base, a in a_snap.items():
        b = b_snap.get(base)
        if not b or len(b) != len(a):
            continue
        for off in range(0, len(a), PAGE):
            pa, pb = a[off:off + PAGE], b[off:off + PAGE]
            if pa != pb:
                for j in range(len(pa)):
                    if pa[j] != pb[j]:
                        out[base + off + j] = (pa[j], pb[j])
    return out


def keys(spec):
    here = os.path.dirname(os.path.abspath(__file__))
    subprocess.run([sys.executable, os.path.join(here, "keys.py"), *spec.split(), "--front", "--gap", "50"])


def main():
    walks = sys.argv[1:]
    p = Proc()
    regs = private_rw(p)
    snap = snapshot(p, regs)
    noisy, cand = set(), {}
    for i, w in enumerate(walks):
        # idle phase: whatever changes now is noise
        time.sleep(2.0)
        idle = snapshot(p, regs)
        noisy |= set(changes(snap, idle))
        # walk phase
        keys(w)
        time.sleep(0.5)
        after = snapshot(p, regs)
        diff = changes(idle, after)
        for a, (old, new) in diff.items():
            if a in noisy:
                continue
            if a in cand:            # changed again: not a one-way exploration flag
                noisy.add(a)
                cand.pop(a, None)
            elif old == 0:
                cand[a] = new
            else:
                noisy.add(a)
        for a in list(cand):
            if a in noisy:
                cand.pop(a)
        print(f"walk {i + 1} '{w}': {len(diff)} changed, {len(cand)} candidates, {len(noisy)} noisy")
        snap = after
    # final idle: drop anything still moving
    time.sleep(2.0)
    final = snapshot(p, regs)
    for a in list(changes(snap, final)):
        cand.pop(a, None)
    clusters = defaultdict(list)
    for a in sorted(cand):
        clusters[a & ~0xFFF].append(a)
    print(f"{len(cand)} candidates in {len(clusters)} pages")
    for page, addrs in sorted(clusters.items(), key=lambda kv: -len(kv[1]))[:20]:
        vals = sorted({cand[a] for a in addrs})
        print(f"0x{page:X}: {len(addrs):4d} bytes  0x{addrs[0]:X}..0x{addrs[-1]:X}  values {vals[:10]}")


if __name__ == "__main__":
    main()
