#!/usr/bin/env python3
"""Functions by share of PC samples, per phase, from a PC_PROF=1 run (code/pc_prof.c).

    dreamcast-native/tools/pc_prof.py ELF SAMPLES [--top N] [--hz 5003] [--func NAME]

--func NAME lists that function's sampled addresses instead (sh-elf-objdump -d to read them).

SAMPLES is the /pc/pc_prof.bin the run wrote (little-endian words: phase << 24 | pc & 0xffffff). The ELF must be
the one that ran. Needs sh-elf-nm on the PATH (the KOS environment)."""
import argparse
import bisect
import collections
import struct
import subprocess

PHASES = {0: "other (waits, enDjinn, KOS)", 1: "race ticks", 2: "render callbacks"}


def symbols(elf):
    out = subprocess.run(["sh-elf-nm", "-S", "--defined-only", elf], capture_output=True, text=True, check=True)
    syms = []
    for line in out.stdout.splitlines():
        parts = line.split()
        if len(parts) == 4 and parts[2].lower() in ("t", "w"):
            syms.append((int(parts[0], 16), int(parts[1], 16), parts[3]))
    syms.sort()
    return syms


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("elf")
    ap.add_argument("samples")
    ap.add_argument("--top", type=int, default=25)
    ap.add_argument("--hz", type=float, default=5003.0)
    ap.add_argument("--func")
    args = ap.parse_args()
    syms = symbols(args.elf)
    starts = [s[0] for s in syms]
    data = open(args.samples, "rb").read()
    words = struct.unpack("<%dI" % (len(data) // 4), data)
    by_phase = collections.defaultdict(collections.Counter)
    in_func = collections.Counter()
    for w in words:
        pc = 0x8C000000 | (w & 0xFFFFFF)
        i = bisect.bisect_right(starts, pc) - 1
        name = "?"
        if i >= 0 and pc < syms[i][0] + max(syms[i][1], 2):
            name = syms[i][2]
        by_phase[w >> 24][name] += 1
        if name == args.func:
            in_func[pc] += 1
    total = len(words)
    if args.func:
        n = sum(in_func.values())
        print("%s: %d samples" % (args.func, n))
        for pc, c in in_func.most_common(args.top):
            print("  %6.2f%%  %08x" % (100.0 * c / n, pc))
        return
    print("%d samples, %.1f s" % (total, total / args.hz))
    for phase in sorted(by_phase):
        counts = by_phase[phase]
        n = sum(counts.values())
        print("\n%s: %d samples (%.1f%%), %.2f s" % (PHASES.get(phase, phase), n, 100.0 * n / total, n / args.hz))
        for name, c in counts.most_common(args.top):
            print("  %6.2f%%  %7.1f ms  %s" % (100.0 * c / n, 1000.0 * c / args.hz, name))


if __name__ == "__main__":
    main()
