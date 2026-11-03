# Dreamcast port

Sega Rally runs on the Dreamcast as a **native reimplementation** in readable C (`dreamcast-native/`), not as the
lifted i960 code plus an emulated Model 2 board. The lifted host build (`src/`, `lib/model2/`, `build/segamod2`)
stays the behavioural reference: every native race tick must equal it bit for bit.

Decisions (Daniel, 2026-10-23):

- Native rewrite, following the Daytona USA port (`../daytona-arcade-recomp`, branch `dreamcast-native`), which
  reached 60 fps that way. Its earlier attempt to run the lifted code on the SH-4 was an order of magnitude too slow.
- 60 Hz: one simulation tick per Dreamcast vblank (59.94 Hz; the arcade ticks once per 57.5 Hz vblank,
  `render_mode` bit 2 set). The game runs about 4% faster than the cabinet; each tick stays exact.
- First milestone: desert practice race on hardware. Delta MT, the native car physics in lockstep with the host
  reference, and a reduced view (road, car, barriers).

## Quick start

You need the ROM set in `ROMS/srallyc-b/` (ROMS/README.md), KallistiOS 2.3 with the SH-4 toolchain, an enDjinn
checkout beside this repository (`../enDJinn`, or `make ENDJINN_ROOT=...`), and Python 3 with numpy. Everything the
console loads is made from the ROM on your machine into `dreamcast-native/pc/`, which is never committed.

```sh
cmake -B build && cmake --build build                       # the host reference (build/segamod2, sr-native-run, ...)
dreamcast-native/tools/export_course.sh                     # build/course/desert.{course,attach,light}
python3 dreamcast-native/tools/vq_textures.py build/course/desert.course dreamcast-native/pc/desert.course
cp build/course/desert.attach build/course/desert.light dreamcast-native/pc/
dreamcast-native/tools/make_practice_pack.sh                # pc/practice.pak and pc/desert.title
source ../enDJinn/environ.sh
cd dreamcast-native && make PRACTICE=1                      # bin/segarally-native.elf
dc-tool-ip -t <dreamcast-ip> -c pc -x bin/segarally-native.elf   # /pc is served from the host
```

On the pad: the stick steers, R accelerates, L brakes, Y / X shift up / down, A changes the view, START restarts.
`make PRACTICE=1 PRACTICE_REPLAY=1` drives the pack's recorded inputs instead; the other build options are listed
in `dreamcast-native/local.cfg.mk`. `dreamcast-native/tools/diff_check.sh` checks every ported routine against
the lifted code (below).

## Order of work

1. Host reference trace (done: `--trace-race`, `--inputs`, `--frames`, `--snapshot`).
2. `dreamcast-native/` skeleton: enDjinn mode, vblank tick clock, finite runs that return to the loader (done;
   first hardware run 2026-10-23: 600 ticks in 600 frames, returned to the loader).
3. Native race tick on the host: port the per-tick race update routine by routine from `src/game/` into
   `dreamcast-native/include/race` and `code/race`, each checked at every call against its lifted original
   ("Differential checks" below). Then run the native tick on its own from a RAM snapshot and compare trace rows.
   Status 2026-10-23: 88 routines bit-exact (both cars, camera, race flow and phases, controls, sound queue, the
   helicopter); the standalone run matches the reference from tick 147 (countdown) to the end of the race on all
   four input seeds. Not yet checked: lap best / lap record (no scripted drive completes a lap).
   Particles (code/race/particles.c, 2026-10-24): the race frame's car pens (0x24C50 walk, 0x248B0 / 0x247D0 /
   0x23A80 / 0x24560 / 0x240C0) and the dust / debris they spawn (0x317B0, RNG 0x33DC8 at 0x213864) into the pool
   0x20cd90 (36-byte records; 0x20d600 live, 0x20d6f0 free, counts 0x20d7e0..f4), stepped and retired by the
   submit (0x31FC0 / 0x32150); with the frame's geo_fifo_bootstrap / preset RAM (0x20b940 <- 0 each frame). 10
   routines exact on all seeds; the standalone run's pool equals the reference's (autopilot drive, tick 805).
   `sr_particles_draws()` lists what the submit drew last tick: TGP 0x55 places GEO object 0x82c95b (dust,
   catalog 0x28655c0) or 0x82c96c (debris, 0x28655d0) at the world point (record +4/+8/+0xc) through the camera
   view, unrotated, view-space scale (1.5, size = record +0x1c, 1.0): a screen-facing quad. Not ported: the course
   events' record walk (0x3F410; their polygon counts are missing from 0x20b940, which only gates drawing budgets
   that the cars never reach).
4. Desert asset recipe: road/contact data and the car model from the ROM into per-course packs, built on the host
   and never committed (`dreamcast-native/pc/`, gitignored). Status: `practice.pak` (the ROM pages and snapshot
   the race tick reads) and `desert.course` + `desert.attach` (below).
5. The same native tick on hardware, scripted races printing checkpoint rows that must equal the reference.
   Status 2026-10-23: a 600-tick lockstep replay on the console prints the same 10 checkpoint hashes as the host's
   native run; race tick CPU 3.2 ms average, 5.6 ms worst, at -O2 before any tuning.
6. Renderer: PVR direct with store queues, offline-built meshes, z-buffer instead of a runtime painter sort.
   Status 2026-10-23: reduced view v1. `dreamcast-native/tools/export_course.sh` captures the desert course from
   the reference (every GEO object draw as world = inverse(view) x model-view) into `build/course/desert.course`
   (`include/course_pack.h`; untextured, flat polygon colours). `code/render/course_render.c` cuts it into
   chunks at load, culls by bounding spheres, transforms with FTRV and the arcade's projection (GEO focus 350 on
   496x384: 451.6 / 437.5 px at 640x480, view +y up), clips at the near plane and writes to the TA through the
   store queues. Host previews (`SR_COURSE=... sr-native-run`) match the GEO pipeline's own frames
   (`I960_HOST_EXPORT_FRAMES`). `make PRACTICE=1`: the desert practice on a pad, 60 fps on the console (race
   tick 3.4 ms, render 11.6 ms average for about 8,000 triangles).
   v2 (textured): the pack carries the GEO's textures (ARGB1555 twiddled), per-triangle cut-out flags (punch-through
   list) and decal layers (depth bias `1/z * (1 + layer/4096)` against the arcade's painter order).
   `tools/vq_textures.py` stores every texture of 64x64 texels or more in the PVR's VQ form (k-means codebook of
   256 2x2 blocks, cut-out pattern kept exact): 17.2 MiB -> 3.2 MiB at full resolution, visually the same in the
   previews (`shrink_textures.py`, halving the largest, was the stopgap before it).
   Cars and helicopter: `desert.attach` (export_course.sh) lists the dynamic meshes with the RAM pose they follow;
   `code/render/actors.c` places them each frame from live RAM (car pose blocks 0x213b40 + 0x58 per car, helicopter
   node 0x500200; wheels by suspension, steering and spin; rotors by their angles; the arcade's near / mid detail
   bands at 25 and 400 units). The player's own car is drawn only in the behind view (0x2140c0 = 1; pad A).
   `SR_ATTACH=` draws them in the host previews. The sky meshes reach 212,000 units, beyond KOS's default
   background depth (1/z 0.0001): the backend lowers it to 1e-7.
   Back faces: triangles without SR_TRI_TWO_SIDED (GEO attr bit 17, MAME's rule) are dropped when they wind
   counter-clockwise on screen (`sr_render_cull = -1`): about 40% fewer triangles, previews within a few hundred
   pixels of drawing both sides (those were back faces showing through the car's spoiler and roof).
   One traversal a frame (SR_LIST_BOTH): opaque triangles go to the TA as they are made, the punch-through list is
   built in RAM and sent from its own callback, so nothing is culled or transformed twice. Instances are culled
   with world-space spheres against the frustum's world planes before any matrix work. Headers go through the
   store queue like vertices (pvr_prim cost ~290 cycles each). `make RENDER_PROF=1` times the parts on the
   console; `RENDER_NULL=1` skips the TA writes.
   Console, film drive (830 ticks): render 17.9 ms -> 13.1 ms a frame, late frames 231 -> 93.
   Where the rest goes (`RENDER_PROF=1` also counts SH-4 pipeline freezes by cause on PRFC1, a cause per 100
   frames): about 5.5 ms of the ~13 ms is waiting on the data cache (transform ~2 ms streaming 24-byte vertices
   from RAM, emit ~3 ms incl. TA store-queue back-pressure), ~640k instructions issued in all.
   Transforming two vertices per step (fv8/fv12) removed the FPU waits but they reappeared as cache waits: the
   transform is bound by fetching vertices (one line fill at a time). Tried without gain: per-batch tight loop,
   reading u/v/colour from the pack instead of copying them, culling instances under 2 px (-6% triangles), whole
   chunks facing away (5-20% of vertices at best), triangle strips (the GEO's polygons are separate quads with their
   own vertices: greedy strips give 2.06 vertices a triangle against 2.10 with the fanned-quad pairs).
   2026-10-25, the demo drive's first 1,250 ticks (title card, fly-in, countdown, the first corners): render
   15.4 -> 11.9 ms a frame, race tick 4.0 -> 3.15 ms, late ticks 286 -> 56 (of 1,250). The whole two-lap demo
   (12,000 ticks): late 1,905 -> 1,493, the busiest sections still over a frame (opaque pass up to 28 ms). What did it, from a PC
   sampling profile (`PC_PROF=1`, "Hardware runs" below) and the `RENDER_PROF=1` freeze counters:
   - The instruction cache (8 KiB, direct-mapped). Code layout is the linker's: a change elsewhere once put the
     triangle loop and `render_chunk` on the same cache lines, +2 ms a frame of I-cache misses. The frame's hot
     path (`render_instance`, `render_chunk`, the triangle loops, `sr_m2_compose`, the instance loop) now sits in
     one section, `.text.sr_hot` (5.7 KiB). Keep it under 8 KiB; `sh-elf-objdump -h` on course_render.o shows it.
   - Batch headers are written by the core from the backend's header tables (`sr_view_t.headers`), not through a
     callback in another file (whose code could not join the hot section).
   - Larger chunks: 96-unit cells for meshes over 64 units across (was 24 / 40). 315 chunks a frame instead of
     657, and more shared positions (6,950 vertices transformed instead of 8,000); the per-chunk work (sphere
     test, transform setup, batch headers) was a quarter of the render. 160 gained little more and worsened the
     slowest frames.
   - Car lighting (per polygon, `desert.light`) in its own triangle loop instance (`batch_dc_lit`), with the
     light, eye and up vectors brought into each car's model space once: three dot products a polygon, and only
     for polygons drawn. Done inside the course's loop it cost 2 ms through register pressure and branches.
   - The cycle counters for the report are read in place (`perf_cntr_count` was a call per chunk, 1%).
   Measured and dropped: vertex DMA (KOS's hybrid mode; `render/ta.h` notes it). Writing the opaque list to RAM
   through the store queues cost as much as writing it to the TA (5.6 ms against 5.3), and through the cache
   more (7.6 ms: 300 KiB a frame evicts the renderer's own data); with the DMA the frame was slower overall. Also
   without gain: prefetching the triangle loop's arrays (per triangle, per transform step, or the next chunk's
   packed block), and -Os for the race code (400 -> 125 KiB of code, the tick 3.2 -> 4.1 ms).
   Without the TA writes (`RENDER_NULL=1`) the opaque list's emit takes 3.1 ms instead of 5.3: the rest is the
   store queues waiting on the bus. Next levers: fewer triangles reaching the triangle loop (back faces are
   about 45% of those tested), the TGP HLE's FIFO protocol (about 1.3 ms of the race tick).
   Dust and debris (race/particles.h; ported bit-exact): each particle is the pack's quad 0x82c95b (dust) or
   0x82c96c (debris), placed by the TGP's working matrix at the submit, 1.5 wide and `size` tall, parallel to the
   screen, drawn as cut-out quads in the punch-through list, with the pack's texture coordinates as they are (one
   repeat). The texture is a puff of grains (dense in the middle, sparse at the edges): drawn once across a quad it
   gives the arcade's look, fine dots in ragged clouds (compared with the cabinet video at race ticks 330 and 450).
   Repeating it 4 times (tried first) filled whole quads and showed their rectangles. A screen-checker material
   (Daytona's u' trick) was tried before that: it looked too sparse, and over large quads u' outgrew the PVR's
   texture coordinate range (the dust vanished) unless the quads were cut into cells.
   The particle textures stay ARGB1555 (`vq_textures.py --raw-oba`: VQ turns single-texel grains into blobs), and
   the export's replay drive (behind view) is what captures the quads.
   Demo from a drive on the pad: `make PRACTICE=1 RECORD=1` records each attempt to `/pc/driveN.inputs` (START
   begins the next; saved every 10 s and at START), ticking as an absolute pack's replay does so the pack replays
   it exactly; `tools/make_demo_pack.sh driveN.inputs` makes the pack (fed the autopilot's rows it rebuilds the
   autopilot demo byte for byte).
   Film pack (a hands-free run for the capture): record the exporter's autopilot (`I960_HOST_EXPORT_AUTOPILOT=1
   I960_HOST_EXPORT_AUTOPILOT_INPUTS=FILE ... --export-course X`), add view rows, check the native replay equals
   the reference, then `SR_PACK=` it as `pc/practice.pak` and `make PRACTICE=1 PRACTICE_REPLAY=1
   RACE_TICK_LIMIT=830` (the autopilot sticks on a rock face near race tick 840).
7. Then HUD, front end, the other courses, rivals, full scenery, sound (AICAflow; the SCSP is the AICA's
   ancestor), disc image.

## Reference trace

```sh
export I960_HOST_NVRAM=build/trace/nvram.yaml; rm -f $I960_HOST_NVRAM   # fresh NVRAM: records affect the run
./build/segamod2 --headless --practice --frames 4000 \
    --inputs build/trace/fullgas.inputs --trace-race build/trace/fullgas.trace
```

`--headless` must come before `--practice`. Input rows are `race_tick steer accel brake gear [view]`, each held
until the next row (steer 0x80 is centre; view 1 holds VIEW CHANGE, IN0 bit 5, whose press toggles the camera
between the bumper and the behind view; `gen_inputs.py --views`, used for seed 4 of the diff check). Race ticks count from GO (`0x214120 > 0`, practice starts at tick 561).
A trace row is `T tick race_tick mode inner scene clock steer accel brake gear ncars node`, then 64 words of the
first `car_frame` node (0x500100 in practice: the computer-driven car that follows the road line) and 64 words of
the scene node at 0x500000 (the player's car, driven by the inputs through `geo_view_scene_frame`). In neutral
(gear 0) the player's Delta MT does not move. `--snapshot FILE@TICK` writes the state after that race tick: CRX, work, buffer, comm and backup RAM and the TGP
state, as sections (`SRSNAP01`, then tag, size, bytes). Work RAM holds the car nodes from 0x500000; above 0x59f000
it mirrors program ROM tables.
Two runs with the same inputs and a fresh NVRAM are byte-identical.

## Differential checks

`build/segamod2-diff` (`dreamcast-native/tests/diff`) is the host reference in which every ported routine runs
twice per call: the lifted original as the game uses it, then the port from the same RAM and TGP state. Both end
states are compared (CRX, work and buffer RAM, the whole TGP HLE state, a caller frame or return value when given),
and the game continues from the original's, so a wrong port shows at its own call. Run it on four seeded drives:

```sh
dreamcast-native/tools/diff_check.sh [BUILD_DIR [FRAMES]]    # e.g. build 6000; prints mismatches per routine
```

In this build the native code works directly on the host's RAM arrays (`sr_mem` points at them) and calls the
host's TGP (`tests/diff/sr_tgp_host.c`); on the Dreamcast `code/race/tgp.c` replaces it.

### Porting a routine

1. Read the lifted file in `src/game/` fully, including its comments: they record ROM addresses and the host
   workarounds the reference relies on (which the port must reproduce, since the reference is what it matches).
2. Write the port in `dreamcast-native/code/race/<module>.c`, declarations in `include/race/<module>.h`. Native
   names describe what the routine does (`sr_car_integrate`), with the ROM address in a comment. Keep the
   original's RAM layout: `sr_ld32/sr_st32/sr_ld16/...` at the same addresses (`include/sr_mem.h`), offsets as
   named constants where the meaning is known.
3. Floats stay raw bits (`uint32_t`) as in the lifted code. One op then a store is the IEEE single op:
   `fadd/fsub/fmul/fdiv` (`include/race/fp.h`). Where the lifted code keeps several ops in `double` before one
   rounding, compute in double too (`D()`, `F()`); `i960_rifl_read(lo, hi)` constants are `sr_d(lo, hi)`. Its
   "leave the destination when not finite" helpers are `gadd/gsub/gmul`. `sin/cos/atan2/asin` only via
   `lib/model2/include/m2_math.h`.
4. TGP: `sr_tgp_cmd(cmd, in, n_in, out, n_out)` with exactly the words the lifted code writes and reads.
   Host-only TGP calls (`model2_tgp_arm_*`) have `sr_tgp_arm_*` equivalents.
5. Implicit inputs become parameters: the caller's frame (`fp`) as `uint8_t *frame`, `g3` etc. as named
   arguments. Lifted helpers that accept a guest address or a host pointer take a host pointer natively
   (`sr_at(va)` for RAM).
6. Hidden host state matters: a lifted `static` buffer that is not cleared between calls (e.g.
   `s_force_slot_fp` in integrate) is a `static` in the port too.
7. Check it: add `lifted_name src/file.c` to `tests/diff/ported/<module>.list` and a wrapper to
   `tests/diff/wrap_<module>.c` (`SR_DIFF_WRAP3`, or by hand with `SR_DIFF_EXTRA` for a caller frame and
   `SR_DIFF_RET` for a return value; see `wrap_car.c`). Run `diff_check.sh` until every routine shows 0
   mismatches on all seeds. A routine a drive never reaches is not checked: say so in its comment.

## Standalone native run and the lockstep test

```sh
# reference trace and a snapshot after race tick 0 (the start)
I960_HOST_NVRAM=build/trace/nv.yaml ./build/segamod2 --headless --practice --frames 2600 \
    --inputs IN --trace-race build/trace/ref.trace --snapshot build/trace/start.snap@0
# the native tick alone (code/race/race_tick.c), same inputs; compare row by row
./build/sr-native-run build/trace/start.snap IN build/trace/native.trace 2000
dreamcast-native/tools/compare_race_trace.py build/trace/ref.trace build/trace/native.trace
# where RAM differs at a tick: SR_DUMP_AT=N on the native run, --snapshot X@N on the reference
dreamcast-native/tools/diff_snapshots.py REF.snap build/trace/native@N.snap
```

`SR_PACK=FILE` makes the native run also write a race pack: program ROM, the main data and copro data pages the run
read (a few hundred KiB, not the 16 MiB of ROM), the snapshot and the inputs. `make LOCKSTEP=1` builds the
Dreamcast program that replays `/pc/race.pak` one tick per frame and prints the same checkpoint hashes
(`include/race/checkpoint.h`) with tick CPU times:

```sh
SR_PACK=$PWD/dreamcast-native/pc/race.pak ./build/sr-native-run start.snap IN native.trace 600
cd dreamcast-native && make LOCKSTEP=1
dctool_lock && ensure_dctool_ready && kos-tool -t $DCTOOL_HOST -m $PWD/pc -x bin/segarally-native.elf; dctool_unlock
```

## What the race tick needs

Measured with a write/read dependency trace of the lifted run (which routines store RAM words that the two cars'
physics later read), the native race tick needs, besides the two car updates:

- inputs (`game_io_poll`: 0x202050 steer, 0x202051 accel, 0x202052 brake, 0x202044 gear);
- race flow and object bookkeeping: `game_start_race_slot3` (clock, checkpoints, laps, finish), `obj_bind` →
  `obj_publish` / `obj_sort_index` (running order 0x213b00 read by `car_peer`, object count 0x213978),
  `list_flag_prep`;
- the camera (`cam_bind` / `cam_chase`, 0x2139d8): the host restores the camera's latched view matrix on the TGP
  before the list walk, so the TGP state the cars start from depends on it;
- `geo_rng_u8` (RNG state 0x2197b0) where gameplay draws from it.

The TGP list cursor 0x20a290 is shared with the renderer; its value only says where TGP parameter blocks go. The
desert's animated objects (`game_start_race_desert_*`) are decoration with their own nodes from 0x500218.
Unlifted ROM calls (`i960_call_rom`) are no-ops in the reference, so the port leaves them out too (named in
comments): the reference, not the arcade, is what the port matches.

## Rules for the native code

- Gameplay units build with `-fno-fast-math -ffp-contract=off`. The lifted code computes each single-precision
  op in double and rounds to float, which equals the IEEE single result for one add, sub, mul, div or sqrt.
  So native gameplay uses `float` with `-m4-single`. Chains the lifted code keeps in double without rounding,
  and the i960's invalid-op "leave the destination" rule, must be reproduced where they occur.
- No `double` in hot code. `isfinite()` is a libgcc call on the SH-4; use `m2_isfinitef` / `m2_isfinite`.
- Without `-ffast-math` every float compare is a `__unordsf2` call on the SH-4. Gameplay keeps it (exactness);
  `code/render/` builds with `-ffast-math` (Makefile).
- The operand cache is direct-mapped (16 KiB): arrays a multiple of 16 KiB apart that are used together evict each
  other on every access. Interleave such data (the renderer's scratch vertices went from 22 to 11 ms that way).
- The TGP's non-IEEE results (tables, `lib/model2/tgp`) stay exact where gameplay reads them. The renderer may
  approximate (SH4ZAM, pinned).
- Big-endian ROM data is swapped when the assets are built, never per access.
- No per-frame decoding of ROM formats, and no `printf` over dcload in a measured scene.
- Budget the heap from the start: keep at least 1 MB free, and make the build fail when a scene's budget breaks.
- Measure speed on hardware with A/B runs and late-frame counts, not averages.

## The start: countdown packs, digits, fades

The practice now starts where the native tick is exact, absolute tick 147 (GO is 561): the fly-in camera from white
past the start gate to the cars, into the bumper view, then 3-2-1-GO.
- Packs from before GO: `sr-native-run FILE@tN ...` (or `SR_SNAPSHOT_TICK=tN`) with `SR_PACK=` writes META
  (start, ticks, 1): the counts are absolute ticks. code/lockstep.c replays them as run_abs does (idle controls until
  GO, the rows from race tick 0, the poll again at GO); restarts go back to the countdown. Race-tick packs (META of
  two words) work as before. `tools/make_practice_pack.sh` builds the countdown pack by default (`START=0`: GO).
  Absolute runs draw previews too (`SR_COURSE=` ..., build/trace/render_t@N.ppm). The reference recording's frame n
  is about absolute tick n + 40.
- The player's car is drawn unless the eye is inside it (the bumper camera): the fly-in and the behind view show
  it (code/render/actors.c; was: only with the behind-view toggle).
- The digits 3, 2, 1, GO: textured quads, catalog 0x9d..0xa0 = GEO 0x82c6fc, 0x82c70d, 0x82c71e, 0x82c72f, drawn
  by the countdown's object step (0x3F9A0, code/race/race_phases.c) in the start lights' GEO window (focus 512,
  centre (248, 192)). The step records each draw's TGP matrix (sr_countdown_draws; not RAM); code/render/countdown.c
  projects the pack's meshes, drawn in front of the course (punch-through list, 1/z 100). Export:
  `I960_HOST_EXPORT_COUNTDOWN=1` captures before GO; export_course.sh's countdown drive is merged as
  `countdown.course@meshes` (its new meshes only, appended: no scenery from the other GEO window, existing indices
  kept).
- Fades: the palette interrupt's mode machine (0x33180: promote 0x213840 -> 0x213850, mode 4 = wash the GEO colour
  tables toward white by scale / 256, fade_step) runs natively from the vblank (code/race/palette_irq.c, checked:
  ported/palette.list, 0 mismatches; modes 1, 2, 6, 7 only at boot and after the race: compared and run there by the
  original alone). The renderer reads sr_palette_wash(): a translucent white quad over the frame on the console
  (the translucent list follows the punch-through one, so it washes the HUD too, for the second the fade lasts),
  the 3D only in the previews.
- The title card ("1995 SEGA RALLY / DESERT / START!" over the sepia course picture): the practice's scene 4
  (attract_hud_setup 0x1B940, absolute ticks 26..146; the trace's scene column goes 4 -> 5 at tick 147). Its first
  tick draws two CGM pictures into the 2D layer (catalog_draw_setup 0x29EB0: the course picture on map 2, category
  0, palette lines 9..21 with the sepia in their own colours; the title on map 0, category 1), then it holds 120
  ticks and its last tick runs the whole race setup. The card is static (the composed picture is identical at ticks
  30, 100, 140, 146) and nothing 3D is drawn under it, so it is not ticked natively: tools/make_title_image.py
  composes it from the reference's 2D state at tick 100 (maps 3..0 category 0 then category 1, pen 0 see-through,
  black kept: the reference punches black out only over a 3D view; white where no map has a pixel) into one PAL8
  image (42 colours), pc/desert.title (made by make_practice_pack.sh). The console shows it for 121 frames before
  the countdown pack's first tick, and again after a restart (code/render/pvr_title.c, code/main.c); the countdown
  then fades in from white as the arcade does. Not ported for it: catalog_draw_setup (CGM decode), the setup ticks
  before 147; needed only if a card were not static or for other courses' cards (make one file per course instead).

## Memory access and allocation on the console

- `include/sr_mem.h` reads and writes the RAM images with one aligned load or store (a `may_alias` word type),
  bytes only for an odd address. A 4-byte `memcpy` from a pointer of unknown alignment is a library call on the
  SH-4 (`mov #4,r6; jsr @memcpy`): ~250 call sites in the race code before, race tick 3.52 -> 3.14 ms, checkpoint
  hashes unchanged.
- `sr_ld*` / `sr_st*` look the address up in a table of 64 KiB pages first (`sr_mem.page`, filled by
  `sr_mem_map()` once the blocks are set) and in main data's 4 KiB pages; only the rest (the work RAM's ROM
  mirror above 0x59f000, comm and backup RAM, unmapped addresses) walks the blocks with their rules. The walk was
  444 bytes inlined at every access, so GCC emitted it out of line 26 times: 5% of the race tick.
- `isfinite()` is `m2_isfinitef` / `m2_isfinite` (`lib/model2/include/m2_math.h`, the exponent bits): under IEEE
  compares GCC makes it a `__unordsf2` call, and the TGP HLE checks its matrix on every command (9% of the tick).
- `include/sr_alloc.h` (Daniel's rule for every port): `sr_alloc` = `memalign(32)` on the console, `sr_copy` /
  `sr_fill` = SH4ZAM (`-lsh4zam`, KOS addons), the C library on the host. Gameplay keeps exact math; SH4ZAM's
  approximate maths are for the renderer only.

## The 2D layer and the race HUD

The HUD (TOTAL TIME, LAP TIME 1-3, POSITION, BEST TIME, the time-left digits, km/h, the START..GOAL bar) is
Model 2's System 24 tile layer; the tachometer needle is one 3D GEO object.

Per race tick the HUD frame (0x20740: `sr_hud_frame`, code/race/hud.c, from the PRG slot handlers 0x21330 / 0x21340
and the countdown) runs layers (total time), tach (time-left digits, 0x20b0b0), the needle (0x203D0), lap time,
position, speed and progress. Each redraws only the digits whose cache word changed (0x20aaf0..0x20ab44,
0x20aeb8), by blitting scene records into the tile map (draw_scene_dispatch 0x29C10 = `sr_draw_scene`):
batch entry `batch + glyph` of the table at 0x20b950 (8 bytes: u16 colour base, u32 record), a record = u16 width
at +16, u16 rows at +18, then width x rows u16 cells from +0x14; each cell is written as cell + colour base (+0x8000
on the L2 map, +0x4000 with flags bit 3). L2 = 0x01000000 (map 0), L1 = 0x01004000 (map 2), flags bit 1 the second
page (+0x2000: maps 1, 3). Checked: every HUD routine 0 mismatches against its lifted original (ported/hud.list,
the tile map compared with the RAM images); the standalone native run's tile map equals the reference's at race
ticks 300 and 805 and in the countdown (absolute tick 450).

Memory (host: model2_rom.c; snapshots and race packs carry the 2D state as sections TILE, CHAR, PALR):
- 0x01000000 TILE, 64 KiB, u16 words: four 64 x 64 cell maps at word 0x0000, 0x1000, 0x2000, 0x3000; per-line
  hscroll tables at word 0x4000 (0x200 a pair); hscr / vscr at words 0x5000+pair / 0x5004+pair (vscr bit 15 = map
  off; bit 15 of hscr = per-line scroll), control at 0x5004 + (pair & 2) (0x6000 bits = special window modes);
  window masks at words 0x6000 (even maps) and 0x6800 (odd). In the race all scroll, control and mask words are 0
  and only map 0 has cells, all with bit 15.
- Cell: code = cell & 0x3fff (character), colour = (cell >> 7) & 0xff (overlapping the code's high bits, as the
  hardware does), pen = colour * 16 + pixel, bit 15 = priority category.
- 0x01080000 CHAR, 512 KiB: 8 x 8 characters, 4 bits a pixel, 32 bytes each (rows of 4 bytes). As little-endian
  u16 words a row is two words: pixels 0..3 in word 0 and 4..7 in word 1, the leftmost pixel in the top nibble.
  Pixel value 0 is transparent. Uploaded from the CGM catalogs in main data during the race setup (catalog_draw_setup
  0x29EB0, not ported: on the console the race pack's CHAR section is the source); it does not change after the
  countdown starts (about 10,100 characters used).
- 0x01800000 PALR, 16 KiB: u16 colours, red bits 0-4, green 5-9, blue 10-14 (as src/host/sys24_tile.c decodes
  them); the tile layer uses pens 0..2047 (the host wraps larger pens). Two bytes change at GO; otherwise static.
- Screen: 496 x 384 pixels, the same window as the 3D view (0,128)-(496,512) (scale 640/496 x 480/384 on the
  Dreamcast). Cell (x, y) of a map with zero scroll is at pixel (8x, 8y); columns 0..61 and rows 0..47 show.
- Order (model2_v): maps 3, 2 then 1, 0 with category 0 pixels under the polygons; then maps 3, 2, 1, 0 with
  category 1 (bit 15) pixels over them. Pen 0 leaves what is below. The race HUD is all in the over-polygon pass.
- The needle: catalog row 0x2865130 = GEO object 0x801a7a (one polygon; mesh 0x801a7a in the course pack), drawn
  through its own GEO window (prg 0x80, 0x01f00200, 4 x 0x00f80140) with focus 128 x 128 and the TGP matrix
  push, identity, T(-185, -127, 128), scale 40, roll(r), where r follows the engine speed at [0x2140cc] + 0x7c
  (code/race/hud.c, sr_hud_cam_matrix). With focus 128 at z = 128 the model maps 1:1 to pixels: the pivot is at
  (248 - 185, 192 + 127) = (63, 319) of the 496 x 384 screen.
- Not ported yet: catalog_draw_setup (CGM uploads; still a hook, e.g. the finish banners) and the scene text of the
  menus.

Drawing it (render/hud2d.h, code/render/hud2d.c, code/render/pvr_hud.c): a 512 x 512 PAL4 texture is a picture
of the HUD's screen (128 KiB of VRAM), screen cell (x, y) the 8 x 8 block at texel (8x, 8y) with the front-most
map's character; an aligned 8 x 8 block of a twiddled texture is 32 contiguous bytes, so a cell is one 32-byte
store-queue copy, and only cells that changed in rows the HUD routines wrote are copied (sr_tile_rows_dirty, set in
hud.c's tile store). Each row is drawn as runs of neighbouring cells with one palette line, a PVR sprite a run
(about 130 sprites instead of 630 quads), in the punch-through list after the 3D with depth compare off. The TA's
intake per record is what the HUD costs: 1.4 ms as quads, 0.47 ms now (`sr: HUD a frame` in the run report).
Palette lines go to the PVR's 64 banks as they are first used; pen 0 and black are transparent (the reference
punches black out of the 2D overlay, which is what shows the progress bar's track and the digits' rims
see-through). The needle: its
polygon through the TGP matrix sr_hud_cam_matrix leaves (sr_hud_needle_matrix), projected with focus 128, drawn
over the cells in orange (measured; the pack's flat colour for it is black). Cost on the console: about 1.3 ms a
frame for ~600 cells. The host previews draw the same (tests/native_run/preview.c).
Packs: `tools/make_practice_pack.sh` builds pc/practice.pak (start snapshot with the 2D sections, the ROM pages of
four gen_inputs drives and the autopilot's, the autopilot's controls with the behind view as its replay).

## Hardware runs

`dreamcast-native/tools/run_hw.sh RUN [ELF [ASSETS]]` does one run with the lock; `VIDEO=1 FRAMES="4 8"` also
records the HDMI capture and stills to `build/hw/`, `TIMEOUT=s` ends a hung run sooner than the default 600 s.

Where the time goes: `make PC_PROF=1 RACE_TICK_LIMIT=...` samples the program counter 5,000 times a second
(TMU1) and writes `/pc/pc_prof.bin` at the end; `tools/pc_prof.py ELF pc_prof.bin` lists the functions per phase
(race ticks, render callbacks, the rest), `--func NAME` the hot addresses inside one (`sh-elf-addr2line -e ELF`
gives their lines). Samples landing on a function's entry and return, as in the TGP HLE's FIFO, are I-cache
misses on the call. Stage the ELF and assets elsewhere when other work may rebuild
`bin/` or change `pc/` while the run waits for the lock (kos-tool serves `/pc` live). By hand:

```sh
source ../enDJinn/environ.sh
cd dreamcast-native && make RACE_TICK_LIMIT=600
DCTOOL_LOCK_WAIT=3600 dctool_lock && ensure_dctool_ready && \
  gtimeout 120 kos-tool -t $DCTOOL_HOST -x bin/segarally-native.elf; dctool_unlock
```

The console is shared with the other ports. Always take the lock, let programs exit by themselves, and never kill
another session's kos-tool.

## Further reading

- Daytona: `docs/NEXT_LLM_HANDOFF.md` (search it), `dreamcast-native/recipes/README.md`, `rules.md`.
- Virtua Racing: `integrations/dream_driving/docs/DREAMCAST_NOTES.md` (lessons from every port on this machine)
  and `DREAMCAST_RENDERER.md`.
- `../sh4-hotlayout`: TMU1 PC sampler, cache layout pinning.
- AICAflow (`../aica`) for sound; the aica session knows the firmware.
