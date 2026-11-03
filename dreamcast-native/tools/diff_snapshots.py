#!/usr/bin/env python3
"""Word differences between two segamod2/sr-native-run snapshots (SRSNAP01 sections), grouped into ranges.

  diff_snapshots.py A.snap B.snap [--sections CRX,WORK] [--max 40]
"""
import argparse
import struct

BASE = {"CRX ": 0x200000, "WORK": 0x500000, "BUFF": 0x900000, "COMM": 0x1a00000, "BKUP": 0x1d00000,
        "TILE": 0x1000000}


def load(path):
    d = open(path, "rb").read()
    assert d[:8] == b"SRSNAP01", path
    off, out = 8, {}
    while off + 8 <= len(d):
        tag = d[off:off + 4].decode()
        n = struct.unpack("<I", d[off + 4:off + 8])[0]
        out[tag] = d[off + 8:off + 8 + n]
        off += 8 + n
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("a")
    ap.add_argument("b")
    ap.add_argument("--sections", default="CRX,WORK,BUFF,COMM,BKUP,TILE")
    ap.add_argument("--max", type=int, default=40)
    args = ap.parse_args()
    a, b = load(args.a), load(args.b)
    shown = 0
    for name in args.sections.split(","):
        tag = name.ljust(4)
        if tag not in a or tag not in b:
            continue
        x, y = a[tag], b[tag]
        words = [i for i in range(0, min(len(x), len(y)) - 3, 4) if x[i:i + 4] != y[i:i + 4]]
        print(f"{tag.strip()}: {len(words)} words differ")
        for i in words:
            if shown >= args.max:
                break
            va = BASE[tag] + i
            wa, wb = struct.unpack("<I", x[i:i + 4])[0], struct.unpack("<I", y[i:i + 4])[0]
            fa, fb = struct.unpack("<f", x[i:i + 4])[0], struct.unpack("<f", y[i:i + 4])[0]
            print(f"  {va:#09x}: {wa:08x} ({fa:.6g})  {wb:08x} ({fb:.6g})")
            shown += 1


if __name__ == "__main__":
    main()
