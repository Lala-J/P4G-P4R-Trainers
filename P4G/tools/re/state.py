#!/usr/bin/env python3
"""Print the live game state the trainer's read-only pages show (for cross-checking the UI).

    python state.py [--json out.json]
"""
import json
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from p4gproc import Proc  # noqa: E402

BLOCK = 0x1451BCD70
NAMES_PERSONA, NAMES_SKILL, NAMES_ARCANA = 0x141047A00, 0x1410423C0, 0x141042120
TIMES = ["Early Morning", "Morning", "Lunchtime", "Afternoon", "After School", "Evening", "Night"]
DOW = ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"]
MONTH_DAYS = [30, 31, 30, 31, 31, 30, 31, 30, 31, 31, 29, 31]  # Apr 2011 .. Mar 2012


def date_of(day):
    m, d = 4, day + 1
    for md in MONTH_DAYS:
        if d <= md:
            break
        d -= md
        m = m % 12 + 1
    dow = (5 + day) % 7  # 2011-04-01 was a Friday
    return f"{m}/{d}", DOW[dow]


def main():
    p = Proc()
    s = lambda a, n: p.read(a, n).split(b"\0")[0].decode("latin-1")
    out = {}
    day = p.u16(BLOCK + 0x18DC)
    tod = p.u8(BLOCK + 0x18DE)
    md, dow = date_of(day)
    out["date"] = {"day": day, "md": md, "dow": dow, "time": TIMES[tod] if tod < 7 else tod}
    out["yen"] = p.u32(BLOCK)
    out["party"] = list(struct.unpack("<3H", p.read(BLOCK + 4, 6)))
    mc = BLOCK + 0xC74
    out["stats"] = dict(zip(["Courage", "Knowledge", "Diligence", "Understanding", "Expression"],
                            struct.unpack("<5H", p.read(mc + 0x34, 10))))
    out["mc"] = {"level": p.u8(mc + 6), "exp": p.u32(mc + 0x40), "hp": p.u16(mc + 8), "sp": p.u16(mc + 0xA)}
    links = []
    for i in range(23):
        cid, rank, pts = struct.unpack("<3H", p.read(BLOCK + 0x18F0 + i * 0x10, 6))
        if cid:
            links.append({"id": cid, "rank": rank, "points": pts})
    out["links"] = links
    stock = []
    for i in range(12):
        a = BLOCK + 0xA34 + i * 0x30
        if p.u8(a):
            pid = p.u16(a + 2)
            stock.append({"slot": i, "id": pid, "name": s(NAMES_PERSONA + pid * 0x15, 0x15),
                          "level": p.u8(a + 4)})
    out["stock"] = stock
    out["equipped_slot"] = struct.unpack("<h", p.read(BLOCK + 0xA30, 2))[0]
    inv = p.read(BLOCK + 0x30, 2560)
    names = p.u64(0x141165938)
    out["items"] = [{"id": i, "n": c, "name": s(names + i * 0x18, 0x18)} for i, c in enumerate(inv) if c]
    if "--json" in sys.argv:
        json.dump(out, open(sys.argv[sys.argv.index("--json") + 1], "w"), indent=1)
    print(json.dumps({k: v for k, v in out.items() if k != "items"}, indent=1))
    print(f"items: {len(out['items'])} kinds, e.g.",
          ", ".join(f"{i['name']} x{i['n']}" for i in out["items"][:8]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
