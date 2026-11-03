#!/bin/zsh
# usage: dreamcast-native/tools/run_hw.sh RUN [ELF [ASSETS]]   one console run, optionally with an HDMI video.
# Takes the shared console lock, checks the loader, runs ELF (default dreamcast-native/bin/segarally-native.elf) with
# ASSETS (default dreamcast-native/pc) as /pc. VIDEO=1 records the HDMI picture (avfoundation "Live Gamer Ultra
# 2.1-Video": 1280x960 if the card offers it, else its 1080p scaled to 1280x960) and FRAMES="2 5 10"
# writes PNG stills (640x480) at those seconds. Out: build/hw/RUN.log, RUN.mp4,
# RUN@Ns.png. The program must end by itself (RACE_TICK_LIMIT); TIMEOUT=seconds (default 600) ends a hung run.
root=$(cd "$(dirname "$0")/../.." && pwd)
run=$1; elf=${2:-$root/dreamcast-native/bin/segarally-native.elf}; assets=${3:-$root/dreamcast-native/pc}
out=$root/build/hw; mkdir -p $out
source $root/../enDJinn/environ.sh >/dev/null 2>&1
caffeinate -i -s -w $$ &
export DCTOOL_LOCK_WAIT=${DCTOOL_LOCK_WAIT:-3600}
dctool_lock || { echo "console lock not taken"; exit 75; }
# a console in the loader is ready: reset only when --ready fails (and only under the lock)
gtimeout 5 kos-tool --ready -t $DCTOOL_HOST >/dev/null 2>&1 || reset_dctool_wait >/dev/null 2>&1 ||
  { echo "loader not ready"; dctool_unlock; exit 1; }
cap=
if [[ ${VIDEO:-0} == 1 ]]; then
  # 1280x960 (4:3) straight from the card when it offers that mode, else its 1080p scaled to 1280x960
  dev="Live Gamer Ultra 2.1-Video:none"
  sizes="1280x960 1920x1080"
  ffmpeg -hide_banner -f avfoundation -list_devices true -i "" 2>&1 | grep -q "Live Gamer Ultra 2.1-Video" || sizes=
  for size in ${=sizes}; do
    ffmpeg -hide_banner -loglevel error -f avfoundation -framerate 60 -video_size $size -pixel_format uyvy422 \
      -i "$dev" -vf scale=1280:960 -c:v h264_videotoolbox -b:v 12M -y $out/$run.mp4 < /dev/null > $out/$run.ffmpeg.log 2>&1 &
    cap=$!
    sleep 2
    kill -0 $cap 2>/dev/null && ! grep -q "Error" $out/$run.ffmpeg.log && break
    kill -INT $cap 2>/dev/null; wait $cap 2>/dev/null
    cap=
  done
  [[ -n $cap ]] && echo "video: $size from the card, recorded at 1280x960" || echo "video: capture card not found"
fi
gtimeout ${TIMEOUT:-600} kos-tool -t $DCTOOL_HOST -m $assets -x $elf > $out/$run.log 2>&1
rc=$?
dctool_unlock
sleep 1
[[ -n $cap ]] && { kill -INT $cap; wait $cap; }
grep -a "sr:\|Program returned" $out/$run.log
for s in ${=FRAMES:-}; do
  ffmpeg -hide_banner -loglevel error -ss $s -i $out/$run.mp4 -frames:v 1 -vf scale=640:480 -y $out/$run@${s}s.png
done
exit $rc
