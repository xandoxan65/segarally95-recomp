ENJ_BASENAME := segarally-native
ENJ_INJECT_QFONT := 1
# enDjinn's default -Og hits a GCC 15.2 SH-4 internal compiler error in player_input.c; the game is built optimized.
OPTLEVEL ?= 2
ENJ_CFLAGS += -I$(abspath include)
# The host reference's portable parts, shared unchanged (TGP HLE, deterministic math).
ENJ_CFLAGS += -I$(abspath ../lib/model2/include) -I$(abspath ../lib/model2/host) -I$(abspath ../lib/model2/tgp/include)

# Gameplay must stay bit-exact against the host reference (docs/dreamcast-port.md):
# no fast-math and no FMA contraction. Under -m4-single every float op is IEEE single.
# No -fsingle-precision-constant: the double chains (include/race/fp.h) need double constants.
# Renderer files may opt into -ffast-math per object in the Makefile later.
ENJ_CFLAGS += -fno-fast-math -ffp-contract=off

# RACE_TICK_LIMIT=N: run N ticks, print the run's figures, return to the loader.
ifdef RACE_TICK_LIMIT
ENJ_CFLAGS += -DSR_RACE_TICK_LIMIT=$(RACE_TICK_LIMIT)
endif

# LOCKSTEP=1: replay /pc/race.pak (tests/native_run, SR_PACK=...) and print its checkpoints.
ifdef LOCKSTEP
ENJ_CFLAGS += -DSR_LOCKSTEP
endif

# PRACTICE=1: drive the desert practice from /pc/practice.pak, drawn from /pc/desert.course.
ifdef PRACTICE
ENJ_CFLAGS += -DSR_PRACTICE
endif

# RENDER_NULL=1: run the course renderer without submitting to the PVR (timing the CPU side).
ifdef RENDER_NULL
ENJ_CFLAGS += -DSR_RENDER_NULL
endif

# RENDER_PROF=1: also time the renderer's emit paths one by one (costs time itself).
ifdef RENDER_PROF
ENJ_CFLAGS += -DSR_RENDER_PROF
endif

# PRACTICE_REPLAY=1 (with PRACTICE=1): drive the pack's recorded inputs, not the pad.
ifdef PRACTICE_REPLAY
ENJ_CFLAGS += -DSR_PRACTICE_REPLAY
endif

# SH4ZAM (KOS addons): block copies and fills (include/sr_alloc.h).
ENJ_LDLIBS += -lsh4zam

# DEMO=1 (with PRACTICE=1 PRACTICE_REPLAY=1): the demo drive's race clock does not run out (race/race_tick.h
# sr_demo_endless_time; the pack is recorded the same way, I960_HOST_ENDLESS_TIME=1). Not the arcade's rules.
ifdef DEMO
ENJ_CFLAGS += -DSR_DEMO
endif

# TICK_PROF=1: time the race frame's parts (race/race_tick.h sr_tick_prof), printed at the end of the run.
ifdef TICK_PROF
ENJ_CFLAGS += -DSR_TICK_PROF
endif

# PC_PROF=1: sample the program counter (code/pc_prof.c) and write /pc/pc_prof.bin at the end (tools/pc_prof.py).
ifdef PC_PROF
ENJ_CFLAGS += -DSR_PC_PROF
endif

# RECORD=1 (with PRACTICE=1): record the drive on the pad to /pc/driveN.inputs (include/lockstep.h; START starts
# attempt N+1), for a demo pack: tools/make_practice_pack.sh with INPUTS=.
ifdef RECORD
ENJ_CFLAGS += -DSR_RECORD
endif

# CHECKPOINT_TICKS=n CHECKPOINT_FROM=tick: lockstep checkpoints every n ticks from that tick (race/checkpoint.h).
ifdef CHECKPOINT_TICKS
ENJ_CFLAGS += -DSR_CHECKPOINT_TICKS=$(CHECKPOINT_TICKS)u
endif
ifdef CHECKPOINT_FROM
ENJ_CFLAGS += -DSR_CHECKPOINT_FROM=$(CHECKPOINT_FROM)u
endif

# DUMP_AT=tick: write RAM after that lockstep tick to /pc/dc@tick.snap (code/lockstep.c; tools/diff_snapshots.py).
ifdef DUMP_AT
ENJ_CFLAGS += -DSR_DUMP_AT=$(DUMP_AT)u
endif
