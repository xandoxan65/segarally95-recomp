/* Lockstep run: the race pack written by tests/native_run (SR_PACK=...), replayed with the same inputs; the
 * checkpoint hashes must equal the host run's. */
#include "lockstep.h"
#include "race/hud.h"
#include "sr_alloc.h"
#include "race/checkpoint.h"
#include "race/course.h"
#include "race/io_poll.h"
#include "race/race_tick.h"
#include "sr_mem.h"
#include "sr_tgp.h"
#include "model2_tgp.h"
#include <arch/timer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t tick, steer, accel, brake, gear, view; /* view: VIEW CHANGE held (optional sixth column) */
} input_row_t;

#define MAX_CHECKPOINTS 256

static uint8_t *s_chars, *s_palette; /* the pack's CHAR and PALR sections (the 2D layer), NULL without them */

const uint8_t *sr_lockstep_chars(void) { return s_chars; }
const uint8_t *sr_lockstep_palette(void) { return s_palette; }

static struct {
    /* race_tick: the tick counter of the pack's run, race ticks or (absolute) absolute ticks */
    uint32_t start, ticks, race_tick, hash, absolute;
    int32_t game_race_tick; /* absolute packs: the race tick, -1 before GO (as the inputs count them) */
    input_row_t *inputs, cur;
    uint32_t n_inputs, next_input;
    uint32_t checkpoints[MAX_CHECKPOINTS][2];
    uint32_t n_checkpoints;
    uint64_t tick_us_total, tick_us_max;
    uint32_t over_16ms;
} s;

static const uint8_t *find(const uint8_t *pack, uint32_t size, const char *tag, uint32_t *len) {
    uint32_t off = 8;
    while(off + 8 <= size) {
        uint32_t n;
        sr_copy(&n, pack + off + 4, 4);
        if(!memcmp(pack + off, tag, 4)) {
            *len = n;
            return pack + off + 8;
        }
        off += 8 + n;
    }
    printf("sr: pack has no %.4s section\n", tag);
    return 0;
}

static void *copy_section(const uint8_t *pack, uint32_t size, const char *tag, uint32_t want) {
    uint32_t len;
    const uint8_t *p = find(pack, size, tag, &len);
    void *dst;
    if(!p || len != want) return 0;
    dst = sr_alloc(len);
    if(dst) sr_copy(dst, p, len);
    return dst;
}

static void parse_inputs(const char *text, uint32_t len) {
    uint32_t cap = 256, i = 0;
    s.inputs = sr_alloc(cap * sizeof(*s.inputs));
    while(i < len) {
        char line[128];
        uint32_t j = 0;
        unsigned t, ge, vw = 0;
        int st, ac, br;
        while(i < len && text[i] != '\n' && j < sizeof(line) - 1) line[j++] = text[i++];
        while(i < len && text[i] != '\n') i++;
        i++;
        line[j] = 0;
        if(line[0] == '#' || sscanf(line, "%u %i %i %i %u %u", &t, &st, &ac, &br, &ge, &vw) < 5) continue;
        if(s.n_inputs == cap) s.inputs = realloc(s.inputs, (cap *= 2) * sizeof(*s.inputs));
        s.inputs[s.n_inputs++] = (input_row_t){t, (uint32_t)st, (uint32_t)ac, (uint32_t)br, ge, vw != 0};
    }
}

int sr_lockstep_load(const char *path) {
    FILE *f = fopen(path, "rb");
    uint8_t *pack;
    long size;
    uint32_t len, n, n_main = 0, i, max_page = 0;
    const uint8_t *p;
    static const uint8_t *main_pages[0x2000000 >> 12];
    uint32_t *copro;

    if(!f) {
        printf("sr: cannot open %s\n", path);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    pack = sr_alloc((size_t)size);
    if(!pack || fread(pack, 1, (size_t)size, f) != (size_t)size || memcmp(pack, "SRPACK01", 8)) {
        printf("sr: cannot read pack %s\n", path);
        return -1;
    }
    fclose(f);

    if(!(p = find(pack, (uint32_t)size, "META", &len))) return -1;
    sr_copy(&s.start, p, 4);
    sr_copy(&s.ticks, p + 4, 4);
    s.absolute = 0;
    if(len >= 12) sr_copy(&s.absolute, p + 8, 4); /* a pack from before GO (the countdown) */
    sr_mem.rom = copy_section(pack, (uint32_t)size, "ROM ", SR_ROM_SIZE);
    sr_mem.crx = copy_section(pack, (uint32_t)size, "CRX ", SR_CRX_SIZE);
    sr_mem.work = copy_section(pack, (uint32_t)size, "WORK", SR_WORK_SIZE);
    sr_mem.buffer = copy_section(pack, (uint32_t)size, "BUFF", SR_BUFFER_SIZE);
    sr_mem.comm = copy_section(pack, (uint32_t)size, "COMM", SR_COMM_SIZE);
    sr_mem.backup = copy_section(pack, (uint32_t)size, "BKUP", SR_BACKUP_SIZE);
    if(!sr_mem.rom || !sr_mem.crx || !sr_mem.work || !sr_mem.buffer || !sr_mem.comm || !sr_mem.backup) {
        printf("sr: pack RAM sections missing\n");
        return -1;
    }
    /* the 2D layer (race/hud.h): optional, packs from before the HUD port lack it */
    if(find(pack, (uint32_t)size, "TILE", &len) && len == SR_TILE_MAP_SIZE) {
        sr_tile_map = copy_section(pack, (uint32_t)size, "TILE", SR_TILE_MAP_SIZE);
        s_chars = copy_section(pack, (uint32_t)size, "CHAR", SR_CHAR_SIZE);
        s_palette = copy_section(pack, (uint32_t)size, "PALR", SR_PALETTE_SIZE);
    }

    /* Main data: the pages the host run read, behind a page table. */
    if(!(p = find(pack, (uint32_t)size, "MAIN", &len))) return -1;
    sr_copy(&n, p, 4);
    n_main = n;
    for(i = 0; i < n; i++) {
        uint32_t page;
        uint8_t *copy = sr_alloc(4096);
        sr_copy(&page, p + 4 + i * 4100, 4);
        sr_copy(copy, p + 8 + i * 4100, 4096);
        main_pages[page] = copy;
    }
    sr_mem.main_pages = main_pages;
    sr_mem.main_data_size = 0x2000000u;
    sr_mem_map();

    /* Copro data: the TGP firmware wants it linear; unread pages stay zero. */
    if(!(p = find(pack, (uint32_t)size, "COPR", &len))) return -1;
    sr_copy(&n, p, 4);
    for(i = 0; i < n; i++) {
        uint32_t page;
        sr_copy(&page, p + 4 + i * 4100, 4);
        if(page + 1 > max_page) max_page = page + 1;
    }
    copro = sr_alloc_zero((size_t)(max_page ? max_page : 1) * 4096);
    for(i = 0; i < n; i++) {
        uint32_t page;
        sr_copy(&page, p + 4 + i * 4100, 4);
        sr_copy(copro + page * 1024, p + 8 + i * 4100, 4096);
    }
    sr_tgp_init(copro, max_page * 1024);
    if(!(p = find(pack, (uint32_t)size, "TGP ", &len)) || len != model2_tgp_state_size()) {
        printf("sr: pack TGP state missing or a different size\n");
        return -1;
    }
    model2_tgp_state_load(p);

    if(!(p = find(pack, (uint32_t)size, "INPT", &len))) return -1;
    parse_inputs((const char *)p, len);
    free(pack);

    sr_practice_skip = 1;
    sr_io_board_reset();
    s.cur = (input_row_t){0, 0x80, 0, 0, 0, 0};
    s.race_tick = s.start;
    s.game_race_tick = -1;
    s.hash = 2166136261u;
    printf("sr: lockstep pack %s: %s ticks %lu..%lu, %lu main pages, %lu copro pages\n", path,
           s.absolute ? "absolute" : "race",
           (unsigned long)s.start + 1, (unsigned long)(s.start + s.ticks), (unsigned long)n_main, (unsigned long)n);
    return 0;
}

/* An absolute pack's tick, as tests/native_run/native_run.c run_abs drives the reference's inputs: idle controls
 * until GO, the rows from race tick 0 on, and at GO the poll taken again with row 0 (the reference's harness
 * applies the rows only once the race clock runs). */
static void absolute_tick(void) {
    static const input_row_t idle = {0, 0x80, 0, 0, 0, 0};
    const int running = (int32_t)sr_ld32(0x214120u) > 0;
    if(running)
        while(s.next_input < s.n_inputs && (int32_t)s.inputs[s.next_input].tick <= s.game_race_tick + 1)
            s.cur = s.inputs[s.next_input++];
    else
        s.cur = idle;
    sr_io_board_set(s.cur.steer, s.cur.accel, s.cur.brake, s.cur.gear);
    sr_io_board_view((int)s.cur.view);
    sr_race_tick();
    if(!running && (int32_t)sr_ld32(0x214120u) > 0) {
        while(s.next_input < s.n_inputs && (int32_t)s.inputs[s.next_input].tick <= 0) s.cur = s.inputs[s.next_input++];
        sr_io_board_set(s.cur.steer, s.cur.accel, s.cur.brake, s.cur.gear);
        sr_io_board_view((int)s.cur.view);
        sr_io_poll();
    }
    if((int32_t)sr_ld32(0x214120u) > 0) s.game_race_tick++;
}

#ifdef SR_DUMP_AT
/* DUMP_AT=tick: RAM after that tick to /pc/dc@tick.snap, in the native run's snapshot form (SR_DUMP_AT there), for
 * tools/diff_snapshots.py. */
static void dump_ram(void) {
    const char *tags[5] = {"CRX ", "WORK", "BUFF", "COMM", "BKUP"};
    const void *ptr[5] = {sr_mem.crx, sr_mem.work, sr_mem.buffer, sr_mem.comm, sr_mem.backup};
    const uint32_t size[5] = {SR_CRX_SIZE, SR_WORK_SIZE, SR_BUFFER_SIZE, SR_COMM_SIZE, SR_BACKUP_SIZE};
    char path[40];
    FILE *f;
    int i;
    snprintf(path, sizeof(path), "/pc/dc@%lu.snap", (unsigned long)s.race_tick);
    if(!(f = fopen(path, "wb"))) return;
    fwrite("SRSNAP01", 1, 8, f);
    for(i = 0; i < 5; i++) {
        fwrite(tags[i], 1, 4, f);
        fwrite(&size[i], 4, 1, f);
        fwrite(ptr[i], 1, size[i], f);
    }
    fclose(f);
    printf("sr: RAM after tick %lu to %s\n", (unsigned long)s.race_tick, path);
}
#endif

int sr_lockstep_tick(void) {
    uint64_t t0, dt;
    if(s.race_tick >= s.start + s.ticks) return 0;
    s.race_tick++;
    t0 = timer_us_gettime64();
    if(s.absolute)
        absolute_tick();
    else {
        while(s.next_input < s.n_inputs && s.inputs[s.next_input].tick <= s.race_tick)
            s.cur = s.inputs[s.next_input++];
        sr_io_board_set(s.cur.steer, s.cur.accel, s.cur.brake, s.cur.gear);
        sr_io_board_view((int)s.cur.view);
        sr_race_tick();
    }
    dt = timer_us_gettime64() - t0;
    s.tick_us_total += dt;
    if(dt > s.tick_us_max) s.tick_us_max = dt;
    if(dt > 16000) s.over_16ms++;
    s.hash = sr_checkpoint_fold(s.hash);
#ifdef SR_DUMP_AT
    if(s.race_tick == SR_DUMP_AT) dump_ram();
#endif
    if((s.race_tick - s.start) % SR_CHECKPOINT_TICKS == 0 && s.race_tick >= SR_CHECKPOINT_FROM &&
       s.n_checkpoints < MAX_CHECKPOINTS) {
        s.checkpoints[s.n_checkpoints][0] = s.race_tick;
        s.checkpoints[s.n_checkpoints++][1] = s.hash;
    }
    return 1;
}

uint32_t sr_lockstep_race_tick(void) { return s.race_tick; }


static uint32_t s_rec_cap;

void sr_session_record_reset(void) {
    s.n_inputs = s.next_input = 0;
}

void sr_session_record_tick(uint32_t steer, uint32_t accel, uint32_t brake, uint32_t gear, int view) {
    const int running = (int32_t)sr_ld32(0x214120u) > 0;
    /* the row absolute_tick takes this tick: before GO the one it applies at GO (tick 0, replaced each tick),
     * after it the next race tick's */
    const input_row_t row = {running ? (uint32_t)(s.game_race_tick + 1) : 0u, steer, accel, brake, gear,
                             (uint32_t)(view != 0)};
    const input_row_t *last = s.n_inputs ? &s.inputs[s.n_inputs - 1] : 0;
    uint64_t t0, dt;
    if(!running && last && last->tick == 0 && s.next_input < s.n_inputs) s.n_inputs--, last = 0;
    if(!running && s.game_race_tick >= 0) {
        /* the clock stopped (the finish): the replay applies no rows, nor is anything recorded */
    } else if(!last || !running || last->steer != row.steer || last->accel != row.accel || last->brake != row.brake ||
       last->gear != row.gear || last->view != row.view) {
        if(s.n_inputs == s_rec_cap) {
            input_row_t *grown = realloc(s.inputs, (s_rec_cap = s_rec_cap ? s_rec_cap * 2 : 4096) * sizeof(*grown));
            if(!grown) return;
            s.inputs = grown;
        }
        s.inputs[s.n_inputs++] = row;
    }
    s.race_tick++;
    t0 = timer_us_gettime64();
    absolute_tick();
    dt = timer_us_gettime64() - t0;
    s.tick_us_total += dt;
    if(dt > s.tick_us_max) s.tick_us_max = dt;
}

int sr_session_record_save(const char *path) {
    FILE *f = fopen(path, "w");
    uint32_t i;
    if(!f) return -1;
    fprintf(f, "# recorded on the Dreamcast (make PRACTICE=1 RECORD=1): race_tick steer accel brake gear view\n");
    for(i = 0; i < s.n_inputs; i++) {
        const input_row_t *r = &s.inputs[i];
        fprintf(f, "%lu 0x%02lx 0x%02lx 0x%02lx %lu %lu\n", (unsigned long)r->tick, (unsigned long)r->steer,
                (unsigned long)r->accel, (unsigned long)r->brake, (unsigned long)r->gear, (unsigned long)r->view);
    }
    fclose(f);
    return 0;
}

void sr_lockstep_report(void) {
    uint32_t i;
    const uint32_t done = s.race_tick - s.start;
    for(i = 0; i < s.n_checkpoints; i++)
        printf("sr: checkpoint %lu %08lx\n", (unsigned long)s.checkpoints[i][0], (unsigned long)s.checkpoints[i][1]);
    printf("sr: ticks %lu, race tick CPU avg %lu us, max %lu us, over 16 ms %lu\n", (unsigned long)done,
           (unsigned long)(done ? s.tick_us_total / done : 0), (unsigned long)s.tick_us_max, (unsigned long)s.over_16ms);
}

static struct {
    uint8_t *crx, *work, *buffer, *comm, *backup, *tgp, *tile;
} s_start;

static void *copy_of(const void *p, size_t n) {
    void *d = sr_alloc(n);
    if(d) sr_copy(d, p, n);
    return d;
}

void sr_session_keep_start(void) {
    s_start.crx = copy_of(sr_mem.crx, SR_CRX_SIZE);
    s_start.work = copy_of(sr_mem.work, SR_WORK_SIZE);
    s_start.buffer = copy_of(sr_mem.buffer, SR_BUFFER_SIZE);
    s_start.comm = copy_of(sr_mem.comm, SR_COMM_SIZE);
    s_start.backup = copy_of(sr_mem.backup, SR_BACKUP_SIZE);
    s_start.tile = sr_tile_map ? copy_of(sr_tile_map, SR_TILE_MAP_SIZE) : 0;
    s_start.tgp = sr_alloc(model2_tgp_state_size());
    if(s_start.tgp) model2_tgp_state_save(s_start.tgp);
}

void sr_session_restart(void) {
    if(!s_start.crx || !s_start.tgp) return;
    sr_copy(sr_mem.crx, s_start.crx, SR_CRX_SIZE);
    sr_copy(sr_mem.work, s_start.work, SR_WORK_SIZE);
    sr_copy(sr_mem.buffer, s_start.buffer, SR_BUFFER_SIZE);
    sr_copy(sr_mem.comm, s_start.comm, SR_COMM_SIZE);
    sr_copy(sr_mem.backup, s_start.backup, SR_BACKUP_SIZE);
    if(s_start.tile && sr_tile_map) {
        sr_copy(sr_tile_map, s_start.tile, SR_TILE_MAP_SIZE);
        sr_tile_map_dirty = 1;
        sr_tile_rows_dirty[0] = sr_tile_rows_dirty[1] = sr_tile_rows_dirty[2] = sr_tile_rows_dirty[3] = ~0ull;
    }
    model2_tgp_state_load(s_start.tgp);
    sr_io_board_reset();
    s.race_tick = s.start;
    s.game_race_tick = -1;
}

void sr_session_tick(uint32_t steer, uint32_t accel, uint32_t brake, uint32_t gear) {
    uint64_t t0, dt;
    sr_io_board_set(steer, accel, brake, gear);
    t0 = timer_us_gettime64();
    sr_race_tick();
    dt = timer_us_gettime64() - t0;
    s.tick_us_total += dt;
    if(dt > s.tick_us_max) s.tick_us_max = dt;
    s.race_tick++;
}
