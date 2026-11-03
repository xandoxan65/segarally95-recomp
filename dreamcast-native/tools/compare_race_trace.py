#!/usr/bin/env python3
"""Compare two race traces (segamod2 --trace-race, or the native run) row by row on the race tick.

Prints the first race tick where they differ and the differing fields: phase words, inputs, and the words of the
road-following car's node (w, 0x500100 in practice) and the player's node (s, 0x500000), by byte offset and as
float. Exit status 0 when every common race tick matches.

  compare_race_trace.py REFERENCE NATIVE [--from TICK] [--max N]
"""
import argparse
import struct
import sys

HEAD = ["T", "tick", "race_tick", "mode", "inner", "scene", "clock", "steer", "accel", "brake", "gear", "ncars",
        "node"]


def load(path):
    rows = {}
    with open(path) as f:
        for line in f:
            if not line.startswith("T "):
                continue
            parts = line.split()
            rows[int(parts[2])] = parts
    return rows


def as_float(word):
    return struct.unpack("<f", struct.pack("<I", int(word, 16)))[0]


def describe(i, a, b):
    if i < len(HEAD):
        return f"{HEAD[i]}: {a} vs {b}"
    k = i - len(HEAD)
    block, word = ("w", k) if k < 64 else ("s", k - 64)
    return f"{block}+0x{word * 4:02x}: {a} ({as_float(a):.6g}) vs {b} ({as_float(b):.6g})"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("reference")
    ap.add_argument("native")
    ap.add_argument("--from", dest="start", type=int, default=0, help="first race tick to compare")
    ap.add_argument("--max", type=int, default=12, help="differing fields to print")
    ap.add_argument("--ignore-tick", action="store_true", default=True,
                    help="ignore the absolute tick column (the native run starts mid-race)")
    args = ap.parse_args()
    ref, nat = load(args.reference), load(args.native)
    common = sorted(t for t in nat if t in ref and t >= args.start)
    if not common:
        print("compare: no common race ticks")
        return 2
    for t in common:
        a, b = ref[t], nat[t]
        diffs = [i for i in range(min(len(a), len(b))) if a[i] != b[i] and not (i == 1 and args.ignore_tick)]
        if len(a) != len(b):
            diffs.append(min(len(a), len(b)))
        if diffs:
            print(f"race tick {t}: {len(diffs)} fields differ (compared {common[0]}..{t - 1} equal)")
            for i in diffs[:args.max]:
                if i < min(len(a), len(b)):
                    print("  " + describe(i, a[i], b[i]))
                else:
                    print(f"  row length {len(a)} vs {len(b)}")
            return 1
    print(f"compare: race ticks {common[0]}..{common[-1]} equal ({len(common)} rows)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
