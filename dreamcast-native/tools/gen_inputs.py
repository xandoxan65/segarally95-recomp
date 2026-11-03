#!/usr/bin/env python3
"""Deterministic race input scripts for the reference trace and the differential checks.

Rows are "race_tick steer accel brake gear [view]" (segamod2 --inputs). A seed gives a reproducible drive with steering
sweeps, throttle lifts, braking and, for the manual car, gear changes, so more of the physics' branches run than
in a straight full-throttle run. --views adds the VIEW CHANGE button, held for some rows (each press toggles the
camera), from its own random stream so the drive itself is the same as without it.

  gen_inputs.py --seed 1 --ticks 4000 > build/trace/mix1.inputs
"""
import argparse
import random


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--ticks", type=int, default=4000)
    ap.add_argument("--no-gears", action="store_true", help="leave the shifter in neutral (automatic cars)")
    ap.add_argument("--views", action="store_true", help="press VIEW CHANGE now and then")
    args = ap.parse_args()
    rng = random.Random(args.seed)
    view_rng = random.Random(args.seed + 1000)
    t = 0
    gear = 1
    print(f"# gen_inputs.py --seed {args.seed} --ticks {args.ticks}{' --no-gears' if args.no_gears else ''}"
          f"{' --views' if args.views else ''}")
    while t < args.ticks:
        steer = max(0, min(255, int(rng.gauss(0x80, 0x38))))
        brake = 0xe0 if rng.random() < 0.12 else 0
        accel = 0 if brake else (0xe0 if rng.random() < 0.8 else rng.randrange(0, 0x100))
        if not args.no_gears:
            gear = max(1, min(4, gear + rng.choice((-1, 0, 0, 1, 1))))
        view = f" {int(view_rng.random() < 0.15)}" if args.views else ""
        print(f"{t} {steer:#04x} {accel:#04x} {brake:#04x} {0 if args.no_gears else gear}{view}")
        t += rng.randrange(15, 120)


if __name__ == "__main__":
    main()
