#!/bin/sh
# A demo pack from a drive recorded on the console (make PRACTICE=1 RECORD=1 writes /pc/driveN.inputs): the
# practice pack's start (absolute tick 147, the countdown) and the drive's rows, with the race clock that does not
# run out as on the console (SR_ENDLESS_TIME). Play it with make PRACTICE=1 PRACTICE_REPLAY=1 DEMO=1, the pack as
# /pc/practice.pak. Needs tools/make_practice_pack.sh run once before (its start snapshot and ROM page lists).
#
#   dreamcast-native/tools/make_demo_pack.sh DRIVE.inputs [OUT [TICKS]]   default dreamcast-native/pc/demo.pak, 12000
set -e
cd "$(dirname "$0")/../.." >/dev/null
IN=$1
OUT=${2:-dreamcast-native/pc/demo.pak}
TICKS=${3:-12000}
case $IN in /*) ;; *) IN=$PWD/$IN ;; esac
case $OUT in /*) ;; *) OUT=$PWD/$OUT ;; esac
T=build/practice_pack
[ -f "$T/start.snap" ] || { echo "run dreamcast-native/tools/make_practice_pack.sh first"; exit 1; }
cmake --build build -j8 --target sr-native-run >/dev/null
# the ROM pages this drive reads, with those the practice pack already carries
SR_ENDLESS_TIME=1 SR_SNAPSHOT_TICK=t147 SR_PAGES="$T/pages_drive.txt" \
    ./build/sr-native-run "$T/start.snap" "$IN" /dev/null "$TICKS" >/dev/null 2>&1 || true
SR_ENDLESS_TIME=1 SR_SNAPSHOT_TICK=t147 SR_PACK="$OUT" \
    SR_PAGES_UNION="$T/pages1.txt $T/pages2.txt $T/pages3.txt $T/pages4.txt $T/pages_ap.txt $T/pages_drive.txt" \
    ./build/sr-native-run "$T/start.snap" "$IN" /dev/null "$TICKS" 2>&1 | grep -v checkpoint
ls -la "$OUT"
