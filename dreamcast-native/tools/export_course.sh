#!/bin/sh
# Course pack export (dreamcast-native/include/course_pack.h): build the host reference, drive the desert practice
# race headless while src/host/lift_export_course.c captures every GEO object draw, merge the drives, and write
#   build/course/desert.course   the pack
#   build/course/desert.png      top-down view of its static instances (course_pack_info.py)
#   build/course/desert.textures.png  contact sheet of the first 64 textures
#   build/course/desert.light    the cars' per-triangle lighting terms (lit per frame on the console)
#   build/course/desert.attach   moving objects (cars, wheels, helicopter) attached to their poses, fitted from the
#                                autopilot drive's capture dump (course_pack_info.py --attach)
#
#   dreamcast-native/tools/export_course.sh [BUILD_DIR [OUT]]      default build-export build/course/desert.course
#
# COURSE=forest (or mountain, lakeside; default desert) exports that course, by default to
# build/course_<course>/<course>.course.
#
# Drives:
#   autopilot  no --inputs; I960_HOST_EXPORT_AUTOPILOT steers the player's car along the path the road-following
#              car drives (full lap, until the practice race ends; the capture stops when the race scene ends)
#   mix1       gen_inputs.py --seed 1: slow, erratic driving near the start (other camera positions there)
#   replay     the autopilot's recorded controls (--inputs): the dust and debris quads appear only in such a run
#   countdown  the first 700 frames with the countdown captured (its digits); merged last, so the meshes before
#              keep their indices
set -e
cd "$(dirname "$0")/../.." >/dev/null
COURSE=${COURSE:-desert}
case $COURSE in desert) c=0 ;; forest) c=1 ;; mountain) c=2 ;; lakeside) c=3 ;; *) echo "COURSE: desert, forest, mountain or lakeside"; exit 2 ;; esac
export I960_HOST_PRACTICE_COURSE=$c
BUILD=${1:-build-export}
OUT=build/course/desert.course; [ "$COURSE" = desert ] || OUT=build/course_$COURSE/$COURSE.course
OUT=${2:-$OUT}
DIR=$(dirname "$OUT")
DRIVES=$DIR/drives
TOOLS=dreamcast-native/tools

cmake -B "$BUILD" -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build "$BUILD" -j8 --target segamod2 >/dev/null
mkdir -p "$DRIVES"

drive() { # name, extra env (VAR=value ...), then -- and segamod2 args
    name=$1
    shift
    envs=
    while [ "$1" != "--" ]; do
        envs="$envs $1"
        shift
    done
    shift
    rm -f "$DRIVES/$name.nvram.yaml"
    # shellcheck disable=SC2086
    env I960_HOST_NVRAM="$DRIVES/$name.nvram.yaml" $envs "$BUILD/segamod2" --headless --practice "$@" \
        --export-course "$DRIVES/$name.course" 2>&1 | grep 'export-course' || true
    test -s "$DRIVES/$name.course"
}

drive autopilot I960_HOST_EXPORT_AUTOPILOT=1 I960_HOST_EXPORT_DYNDUMP="$DRIVES/autopilot.dyn" \
    I960_HOST_EXPORT_AUTOPILOT_INPUTS="$DRIVES/autopilot.inputs" -- --frames 6000
# The same drive replayed from its recorded controls in the behind view (VIEW CHANGE held at race ticks 2..7): the
# player's dust and debris quads are spawned only by the behind view's pen, so this drive exports them.
awk '/^#/ {print; next} {v = ($1 >= 2 && $1 < 8) ? 1 : 0; print $1, $2, $3, $4, $5, v}
     NR == 3 {print 2, $2, $3, $4, $5, 1; print 8, $2, $3, $4, $5, 0}' "$DRIVES/autopilot.inputs" |
    sort -n -s -k1,1 | awk '!seen[$1]++' > "$DRIVES/replay.inputs"
drive replay -- --frames 3000 --inputs "$DRIVES/replay.inputs"
# The countdown before GO (I960_HOST_EXPORT_COUNTDOWN): the start's 3D digits and lights.
drive countdown I960_HOST_EXPORT_COUNTDOWN=1 -- --frames 700 --inputs "$DRIVES/replay.inputs"
python3 "$TOOLS/gen_inputs.py" --seed 1 --ticks 4000 > "$DRIVES/mix1.inputs"
drive mix1 -- --frames 3000 --inputs "$DRIVES/mix1.inputs"

python3 "$TOOLS/course_pack_info.py" --merge "$OUT" "$DRIVES/autopilot.course" "$DRIVES/mix1.course" \
    "$DRIVES/replay.course" "$DRIVES/countdown.course@meshes" \
    --png "${OUT%.course}.png" --skip 0x7c11 --sheet "${OUT%.course}.textures.png" \
    --attach "${OUT%.course}.attach" --dyn "$DRIVES/autopilot.dyn"
# the cars' lighting terms (OUT.light), merged in the packs' order
python3 "$TOOLS/merge_light.py" "${OUT%.course}.light" "$DRIVES/autopilot.light" "$DRIVES/mix1.light" \
    "$DRIVES/replay.light"
