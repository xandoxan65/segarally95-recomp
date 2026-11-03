#!/bin/sh
# Build segamod2-diff and run it on four scripted practice drives in parallel; print each routine's calls and
# mismatches. Every ported routine is checked against its lifted original at each call (tests/diff/sr_diff.h).
#
#   dreamcast-native/tools/diff_check.sh [BUILD_DIR [FRAMES]]     (default build, 6000)
#
# Separate BUILD_DIRs let several checks run at once. Logs: BUILD_DIR/diffrun/seedN/log.txt.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
build=${1:-build}
frames=${2:-6000}
cd "$root"
[ -f "$build/CMakeCache.txt" ] || cmake -B "$build" >/dev/null
if ! cmake --build "$build" -j8 --target segamod2-diff > "$build/diff_build.log" 2>&1; then
    grep -E "error" "$build/diff_build.log" | head -20
    echo "diff_check: build failed ($build/diff_build.log)"
    exit 2
fi
grep -E "dreamcast-native/.*warning" "$build/diff_build.log" || true
out="$build/diffrun"
mkdir -p "$out"
for seed in 1 2 3 4; do
    d="$out/seed$seed"
    mkdir -p "$d"
    # seed 4 also presses VIEW CHANGE (the other camera)
    views=; [ "$seed" = 4 ] && views=--views
    python3 dreamcast-native/tools/gen_inputs.py --seed "$seed" --ticks "$frames" $views > "$d/inputs"
    rm -f "$d/nvram.yaml"
    ( I960_HOST_NVRAM="$d/nvram.yaml" "$build/segamod2-diff" --headless --practice --frames "$frames" \
          --inputs "$d/inputs" > "$d/log.txt" 2>&1; echo "exit $?" >> "$d/log.txt" ) &
done
wait
status=0
for seed in 1 2 3 4; do
    log="$out/seed$seed/log.txt"
    bad=$(grep "sr_diff:   .* calls " "$log" | awk '$6 != 0' | wc -l | tr -d ' ')
    # A run that stopped early (crash, harness abort) has checked only part of the drive: fail it.
    if ! grep -q "^exit 0$" "$log" || grep -q "too many routines\|nesting deeper" "$log"; then
        echo "seed $seed: the reference run did not finish ($(tail -1 "$log"))"
        grep "sr_diff: \(too many\|nesting\)" "$log" | head -2
        status=1
        continue
    fi
    echo "seed $seed: $(grep -c 'sr_diff:   .* calls ' "$log") routines, $bad with mismatches"
    [ "$bad" = 0 ] || { status=1; grep "sr_diff:   .* calls " "$log" | awk '$6 != 0'; }
done
[ "$status" = 0 ] || echo "first mismatches: grep -A8 differs $out/seed1/log.txt"
exit $status
