#!/bin/sh
# The practice pack (pc/practice.pak) for driving on the pad: the start snapshot from the reference (with the 2D
# layer sections TILE, CHAR, PALR) and the title card shown before it (desert.title beside the pack): by default at
# absolute tick 147, the countdown (the fly-in from white, 3-2-1-GO;
# the native tick is exact from there), START=0 at GO, and the ROM pages several native drives read (gen_inputs.py
# seeds 1-4 and the exporter's autopilot), so free driving finds the course's tables. Its inputs (the autopilot's, with the behind
# view from race tick 300) are what PRACTICE_REPLAY=1 drives.
#
#   dreamcast-native/tools/make_practice_pack.sh [BUILD_DIR [OUT]]   default build, dreamcast-native/pc/practice.pak
set -e
cd "$(dirname "$0")/../.." >/dev/null
BUILD=${1:-build}
OUT=${2:-dreamcast-native/pc/practice.pak}
case $OUT in /*) ;; *) OUT=$PWD/$OUT ;; esac
T=build/practice_pack
TOOLS=dreamcast-native/tools
START=${START:-t147}
mkdir -p "$T"
cmake --build "$BUILD" -j8 --target segamod2 sr-native-run >/dev/null

# the race start and the autopilot's controls (I960_HOST_EXPORT_AUTOPILOT_INPUTS needs --export-course)
rm -f "$T/nv.yaml"
I960_HOST_NVRAM="$T/nv.yaml" I960_HOST_EXPORT_AUTOPILOT=1 I960_HOST_EXPORT_AUTOPILOT_INPUTS="$T/autopilot.inputs" \
    "$BUILD/segamod2" --headless --practice --frames 6000 --export-course "$T/autopilot.course" \
    --snapshot "$T/start.snap@$START" >/dev/null 2>&1
# the title card before the countdown (absolute ticks 26..146, static: composed from the 2D layer in its middle),
# next to the pack as <dir>/desert.title (tools/make_title_image.py)
rm -f "$T/nv_title.yaml"
I960_HOST_NVRAM="$T/nv_title.yaml" "$BUILD/segamod2" --headless --practice --frames 200 \
    --snapshot "$T/title.snap@t100" >/dev/null 2>&1
python3 "$TOOLS/make_title_image.py" "$T/title.snap" "$(dirname "$OUT")/desert.title" --frames 121
# the replay's controls: the autopilot, VIEW CHANGE held at race ticks 300..305 (the behind view)
awk '/^#/ {print; next} {v = ($1 >= 300 && $1 < 306) ? 1 : 0; print $1, $2, $3, $4, $5, v}' "$T/autopilot.inputs" \
    > "$T/replay.inputs"

pages=
for seed in 1 2 3 4; do
    python3 "$TOOLS/gen_inputs.py" --seed "$seed" --ticks 6000 > "$T/seed$seed.inputs"
    SR_SNAPSHOT_TICK="$START" SR_PAGES="$T/pages$seed.txt" \
        "$BUILD/sr-native-run" "$T/start.snap" "$T/seed$seed.inputs" /dev/null 6000 \
        >/dev/null 2>&1 || true
    pages="$pages $T/pages$seed.txt"
done
SR_SNAPSHOT_TICK="$START" SR_PAGES="$T/pages_ap.txt" \
    "$BUILD/sr-native-run" "$T/start.snap" "$T/autopilot.inputs" /dev/null 6000 \
    >/dev/null 2>&1 || true
SR_PACK="$OUT" SR_PAGES_UNION="$pages $T/pages_ap.txt" \
    SR_SNAPSHOT_TICK="$START" "$BUILD/sr-native-run" "$T/start.snap" "$T/replay.inputs" /dev/null 2400 2>&1 |
    grep -v checkpoint
ls -la "$OUT"
