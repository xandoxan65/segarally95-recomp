/* Host race reference: scripted inputs, per-tick trace rows, tick limit.
 *
 * The Dreamcast native port is checked tick for tick against these rows
 * (docs/dreamcast-port.md). One tick is one game_dispatch_main pass, ended by
 * i960_host_frame_present. Race ticks count from the first tick with
 * 0x214120 > 0 (GO), the same gate the live keyboard path uses.
 *
 * Env (also set by --frames / --inputs / --trace-race / --snapshot):
 *   I960_HOST_MAX_FRAMES=N        halt after N ticks
 *   I960_HOST_RACE_INPUTS=FILE    "race_tick steer accel brake gear [view]" rows,
 *                                 each held until the next row (view: VIEW CHANGE held)
 *   I960_HOST_TRACE_RACE=FILE     one row per tick (format below)
 *   I960_HOST_SNAPSHOT=FILE@TICK  state after race tick TICK (>= 0), for the native port's standalone run;
 *                                 FILE@tN: after absolute tick N (the trace's tick column, e.g. in the countdown):
 *                                 "SRSNAP01", then sections {u32 tag, u32 size, bytes}: CRX, WORK,
 *                                 BUFF, COMM, BKUP RAM and the TGP HLE state (model2_tgp_state_save), then the
 *                                 2D layer: TILE (tile map 0x01000000), CHAR (0x01080000), PALR (0x01800000)
 *
 * Row: T tick race_tick mode inner scene clock steer accel brake gear ncars node
 *      then 64 words (+0x00..0xfc, raw bits) of the first car_frame node (0x500100 in practice: the road
 *      follower), then 64 words of the scene node at 0x500000 (the player's car, driven by the inputs through
 *      geo_view_scene_frame).
 */

#include "i960_host.h"
#include "i960_mem.h"
#include "model2_rom.h"
#include "model2_tgp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRACE_NODE_WORDS 64u
#define TRACE_SCENE_NODE 0x500000u
#define INPUT_ROWS_MAX   16384u

typedef struct {
    u32 tick;
    u8 steer;
    u8 accel;
    u8 brake;
    u8 gear;
    u8 view; /* VIEW CHANGE held (optional sixth column) */
} race_input_row_t;

static int s_init;
static FILE *s_trace;
static u32 s_max_frames;
static u32 s_tick;
static i32 s_race_tick = -1;
static race_input_row_t s_inputs[INPUT_ROWS_MAX];
static u32 s_input_count;
static race_input_row_t s_cur = { 0, 0x80u, 0, 0, 0, 0 };
static u32 s_node;
static u32 s_ncars;
static char s_snapshot_path[512];
static i32 s_snapshot_tick = -1;
static int s_snapshot_abs; /* FILE@tN: s_snapshot_tick counts absolute ticks */

static void load_inputs(const char *path)
{
    FILE *f = fopen(path, "r");
    char line[256];

    if (!f) {
        fprintf(stderr, "lift: race inputs %s: cannot open\n", path);
        exit(2);
    }
    while (fgets(line, sizeof(line), f)) {
        unsigned t, ge, vw = 0;
        int st, ac, br;
        char *p = line;

        while (*p == ' ' || *p == '\t')
            p++;
        if (*p == '#' || *p == '\n' || *p == '\0')
            continue;
        if (sscanf(p, "%u %i %i %i %u %u", &t, &st, &ac, &br, &ge, &vw) < 5) {
            fprintf(stderr, "lift: race inputs %s: bad row: %s", path, line);
            exit(2);
        }
        if (s_input_count >= INPUT_ROWS_MAX) {
            fprintf(stderr, "lift: race inputs %s: more than %u rows\n", path,
                    INPUT_ROWS_MAX);
            exit(2);
        }
        if (s_input_count && t <= s_inputs[s_input_count - 1].tick) {
            fprintf(stderr, "lift: race inputs %s: ticks must increase\n", path);
            exit(2);
        }
        s_inputs[s_input_count].tick = t;
        s_inputs[s_input_count].steer = (u8)st;
        s_inputs[s_input_count].accel = (u8)ac;
        s_inputs[s_input_count].brake = (u8)br;
        s_inputs[s_input_count].gear = (u8)ge;
        s_inputs[s_input_count].view = (u8)(vw != 0);
        s_input_count++;
    }
    fclose(f);
}

static void race_trace_init(void)
{
    const char *s;

    if (s_init)
        return;
    s_init = 1;
    s = getenv("I960_HOST_MAX_FRAMES");
    if (s && *s)
        s_max_frames = (u32)strtoul(s, NULL, 0);
    s = getenv("I960_HOST_RACE_INPUTS");
    if (s && *s)
        load_inputs(s);
    s = getenv("I960_HOST_TRACE_RACE");
    if (s && *s) {
        s_trace = fopen(s, "w");
        if (!s_trace) {
            fprintf(stderr, "lift: race trace %s: cannot open\n", s);
            exit(2);
        }
        fprintf(s_trace,
                "# segamod2 race trace v2: T tick race_tick mode inner scene clock "
                "steer accel brake gear ncars node w0..w63 s0..s63\n");
    }
    s = getenv("I960_HOST_SNAPSHOT");
    if (s && *s) {
        const char *at = strrchr(s, '@');
        size_t n = at ? (size_t)(at - s) : 0;

        if (!at || n == 0 || n >= sizeof(s_snapshot_path)) {
            fprintf(stderr, "lift: I960_HOST_SNAPSHOT expects FILE@TICK or FILE@tTICK\n");
            exit(2);
        }
        memcpy(s_snapshot_path, s, n);
        s_snapshot_path[n] = '\0';
        s_snapshot_abs = at[1] == 't';
        s_snapshot_tick = (i32)strtol(at + 1 + s_snapshot_abs, NULL, 0);
    }
}

int i960_host_race_scripted(void)
{
    race_trace_init();
    return s_input_count != 0;
}

static int race_clock_running(void)
{
    return (i32)i960_ld_u32(I960_WORKRAM, 0x214120, 0) > 0;
}

/* Called before the 315-5649 sample in game_io_poll: the scripted row for
 * the coming race tick replaces the analog mux and shifter. */
void i960_host_race_inputs_apply(void)
{
    u32 next;
    u32 i;

    race_trace_init();
    /* I960_HOST_ENDLESS_TIME=1 (demo drives, not the arcade): below 15 s left, 30 s more, as a checkpoint gives; the
     * native run does the same at the same point (dreamcast-native race_tick.c, sr_demo_endless_time) */
    {
        static int endless = -1;
        if (endless < 0) {
            const char *e = getenv("I960_HOST_ENDLESS_TIME");
            endless = e && e[0] == '1';
        }
        if (endless && race_clock_running()) {
            const i32 left = (i32)i960_ld_u32(I960_WORKRAM, 0x20b0b0, 0);
            if (left < 15 * 60)
                i960_st_u32(I960_WORKRAM, 0x20b0b0, 0, (u32)(left + 30 * 60));
        }
    }
    if (!s_input_count || !race_clock_running())
        return;
    next = (u32)(s_race_tick + 1);
    for (i = 0; i < s_input_count && s_inputs[i].tick <= next; i++)
        s_cur = s_inputs[i];
    model2_io_analog_set(MODEL2_IO_AN_STEER, s_cur.steer);
    model2_io_analog_set(MODEL2_IO_AN_ACCEL, s_cur.accel);
    model2_io_analog_set(MODEL2_IO_AN_BRAKE, s_cur.brake);
    model2_io_shifter_set(s_cur.gear);
    model2_io_in0_set_mask(0x20u, s_cur.view);
}

/* First car node run this tick is the player's (seed_cars starts at slot 1). */
void i960_host_race_trace_car(u32 node)
{
    if (s_ncars++ == 0)
        s_node = node;
}

extern u8 model2_bufferram[];

static int snap_section(FILE *f, const char *tag, const void *data, u32 size)
{
    u32 t;

    memcpy(&t, tag, 4);
    return fwrite(&t, 4, 1, f) == 1 && fwrite(&size, 4, 1, f) == 1
        && fwrite(data, 1, size, f) == size;
}

static void write_snapshot(void)
{
    FILE *f = fopen(s_snapshot_path, "wb");
    size_t tgp_size = model2_tgp_state_size();
    void *tgp = malloc(tgp_size);
    int ok;

    if (!f || !tgp) {
        fprintf(stderr, "lift: snapshot %s: cannot open\n", s_snapshot_path);
        exit(2);
    }
    model2_tgp_state_save(tgp);
    ok = fwrite("SRSNAP01", 1, 8, f) == 8
        && snap_section(f, "CRX ", model2_crx_ram, sizeof(model2_crx_ram))
        && snap_section(f, "WORK", model2_workram, sizeof(model2_workram))
        && snap_section(f, "BUFF", model2_bufferram, BUFFERRAM_SIZE)
        && snap_section(f, "COMM", model2_m2comm_share_ptr(), MODEL2_M2COMM_SHARE_SIZE)
        && snap_section(f, "BKUP", model2_backup_sram_ptr(), MODEL2_BACKUP_SRAM_SIZE)
        && snap_section(f, "TGP ", tgp, (u32)tgp_size)
        /* The 2D layer (System 24 tiles): map and registers, character RAM, palette. */
        && snap_section(f, "TILE", model2_tile_map_ptr(), MODEL2_TILE_MAP_SIZE)
        && snap_section(f, "CHAR", model2_tile_char_ptr(), MODEL2_TILE_CHAR_SIZE)
        && snap_section(f, "PALR", model2_palram_ptr(), MODEL2_PALRAM_SIZE);
    free(tgp);
    if (!ok || fclose(f) != 0) {
        fprintf(stderr, "lift: snapshot %s: write failed\n", s_snapshot_path);
        exit(2);
    }
    fprintf(stderr, "lift: snapshot %s after race tick %d (tick %u)\n",
            s_snapshot_path, (int)s_race_tick, (unsigned)s_tick);
}

/* End of one tick (i960_host_frame_present). */
void i960_host_race_trace_tick(void)
{
    u32 i;

    race_trace_init();
    if (race_clock_running())
        s_race_tick++;
    if (s_trace) {
        fprintf(s_trace, "T %u %d %u %u %u %d %u %u %u %u %u %08x",
                (unsigned)s_tick, (int)s_race_tick,
                (unsigned)i960_ld_u32(I960_WORKRAM, 0x202098, 0),
                (unsigned)i960_ld_u32(I960_WORKRAM, 0x20209c, 0),
                (unsigned)i960_ld_u32(I960_WORKRAM, 0x2020ac, 0),
                (int)(i32)i960_ld_u32(I960_WORKRAM, 0x214120, 0),
                (unsigned)model2_io_analog_get(MODEL2_IO_AN_STEER),
                (unsigned)model2_io_analog_get(MODEL2_IO_AN_ACCEL),
                (unsigned)model2_io_analog_get(MODEL2_IO_AN_BRAKE),
                (unsigned)s_cur.gear, (unsigned)s_ncars, (unsigned)s_node);
        for (i = 0; i < TRACE_NODE_WORDS; i++)
            fprintf(s_trace, " %08x",
                    s_node ? (unsigned)i960_ld_u32(I960_ABS, s_node, i << 2) : 0u);
        for (i = 0; i < TRACE_NODE_WORDS; i++)
            fprintf(s_trace, " %08x",
                    (unsigned)i960_ld_u32(I960_ABS, TRACE_SCENE_NODE, i << 2));
        fputc('\n', s_trace);
    }
    if (s_snapshot_tick >= 0
        && (s_snapshot_abs ? (i32)s_tick == s_snapshot_tick : s_race_tick == s_snapshot_tick)) {
        write_snapshot();
        s_snapshot_tick = -1;
    }
    s_tick++;
    s_ncars = 0;
    s_node = 0;
    if (s_max_frames && s_tick >= s_max_frames) {
        if (s_trace)
            fflush(s_trace);
        i960_host_request_halt();
    }
}

void i960_host_race_trace_close(void)
{
    if (s_trace) {
        fclose(s_trace);
        s_trace = NULL;
    }
}

/* Race tick the coming tick will have (-1 before GO); for diagnostics. */
int i960_host_race_tick_now(void)
{
    return (int)s_race_tick + (race_clock_running() ? 1 : 0);
}
