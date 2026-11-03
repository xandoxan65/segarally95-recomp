/* Differential checks: see sr_diff.h. */
#include "sr_diff.h"
#include "sr_mem.h"
#include "model2_rom.h"
#include "model2_tgp.h"
#include "i960_host.h"
#include "race/course.h"
#include "race/hud.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern u8 model2_bufferram[];

sr_mem_t sr_mem;

#define SR_DIFF_DEPTH 8
#define SR_DIFF_REPORT 6

typedef struct {
    uint8_t *crx, *work, *buffer, *comm, *backup, *tgp, *tile_map;
} sr_diff_image_t;

/* The 2D layer's tile map (race/hud.h): the host's, which the HUD routines write. */
static uint8_t *host_tile_map(void) { return (uint8_t *)(uintptr_t)model2_tile_map_ptr(); }

#define SR_DIFF_EXTRA_MAX 1024u

struct sr_diff_frame {
    sr_diff_image_t before, after;
    uint8_t *extra;
    unsigned extra_len;
    uint8_t extra_before[SR_DIFF_EXTRA_MAX], extra_after[SR_DIFF_EXTRA_MAX];
    int ret_set;
    uint32_t ret_lifted, ret_native;
};

typedef struct {
    const char *name;
    unsigned long calls, mismatches;
} sr_diff_stat_t;

static sr_diff_frame_t s_frames[SR_DIFF_DEPTH];
static int s_depth;
static size_t s_tgp_size;
static sr_diff_stat_t s_stats[256];
static unsigned s_nstats;
static int s_stop = -1;

static void image_alloc(sr_diff_image_t *im) {
    if(im->crx) return;
    im->crx = malloc(SR_CRX_SIZE);
    im->work = malloc(SR_WORK_SIZE);
    im->buffer = malloc(SR_BUFFER_SIZE);
    im->comm = malloc(SR_COMM_SIZE);
    im->backup = malloc(SR_BACKUP_SIZE);
    im->tgp = malloc(s_tgp_size);
    im->tile_map = malloc(SR_TILE_MAP_SIZE);
    if(!im->crx || !im->work || !im->buffer || !im->comm || !im->backup || !im->tgp || !im->tile_map) {
        fprintf(stderr, "sr_diff: out of memory\n");
        exit(2);
    }
}

static void image_save(sr_diff_image_t *im) {
    memcpy(im->crx, model2_crx_ram, SR_CRX_SIZE);
    memcpy(im->work, model2_workram, SR_WORK_SIZE);
    memcpy(im->buffer, model2_bufferram, SR_BUFFER_SIZE);
    memcpy(im->comm, model2_m2comm_share_ptr(), SR_COMM_SIZE);
    memcpy(im->backup, model2_backup_sram_ptr(), SR_BACKUP_SIZE);
    memcpy(im->tile_map, host_tile_map(), SR_TILE_MAP_SIZE);
    model2_tgp_state_save(im->tgp);
}

static void image_load(const sr_diff_image_t *im) {
    memcpy(model2_crx_ram, im->crx, SR_CRX_SIZE);
    memcpy(model2_workram, im->work, SR_WORK_SIZE);
    memcpy(model2_bufferram, im->buffer, SR_BUFFER_SIZE);
    memcpy(model2_m2comm_share_ptr(), im->comm, SR_COMM_SIZE);
    memcpy(model2_backup_sram_ptr(), im->backup, SR_BACKUP_SIZE);
    memcpy(host_tile_map(), im->tile_map, SR_TILE_MAP_SIZE);
    model2_tgp_state_load(im->tgp);
}

static void summary(void) {
    unsigned i;
    fprintf(stderr, "sr_diff: summary\n");
    for(i = 0; i < s_nstats; i++)
        fprintf(stderr, "sr_diff:   %-44s calls %10lu  mismatches %lu\n", s_stats[i].name, s_stats[i].calls,
                s_stats[i].mismatches);
}

static sr_diff_stat_t *stat_for(const char *name) {
    unsigned i;
    for(i = 0; i < s_nstats; i++)
        if(s_stats[i].name == name || strcmp(s_stats[i].name, name) == 0) return &s_stats[i];
    if(s_nstats == 0) atexit(summary);
    if(s_nstats >= sizeof(s_stats) / sizeof(s_stats[0])) {
        fprintf(stderr, "sr_diff: too many routines\n");
        exit(2);
    }
    s_stats[s_nstats].name = name;
    return &s_stats[s_nstats++];
}

sr_diff_frame_t *sr_diff_begin(void) {
    sr_diff_frame_t *f;
    if(s_stop < 0) {
        const char *s = getenv("SR_DIFF_STOP");
        s_stop = s && *s && *s != '0';
        s_tgp_size = model2_tgp_state_size();
    }
    if(s_depth >= SR_DIFF_DEPTH) {
        fprintf(stderr, "sr_diff: nesting deeper than %d\n", SR_DIFF_DEPTH);
        exit(2);
    }
    sr_practice_skip = i960_host_skip_practice();
    sr_mem.rom = model2_maincpu_rom;
    sr_mem.crx = model2_crx_ram;
    sr_mem.work = model2_workram;
    sr_mem.buffer = model2_bufferram;
    sr_mem.comm = model2_m2comm_share_ptr();
    sr_mem.backup = model2_backup_sram_ptr();
    sr_mem.main_data = model2_main_data_rom;
    sr_mem.main_data_size = model2_main_data_size;
    sr_mem_map();
    sr_tile_map = host_tile_map();
    f = &s_frames[s_depth++];
    image_alloc(&f->before);
    image_alloc(&f->after);
    image_save(&f->before);
    f->extra = 0;
    f->extra_len = 0;
    f->ret_set = 0;
    return f;
}

void sr_diff_extra(sr_diff_frame_t *f, void *ptr, unsigned len) {
    if(len > SR_DIFF_EXTRA_MAX) {
        fprintf(stderr, "sr_diff: extra buffer over %u bytes\n", SR_DIFF_EXTRA_MAX);
        exit(2);
    }
    f->extra = ptr;
    f->extra_len = len;
    memcpy(f->extra_before, ptr, len);
}

void sr_diff_ret(sr_diff_frame_t *f, uint32_t lifted, uint32_t native) {
    f->ret_set = 1;
    f->ret_lifted = lifted;
    f->ret_native = native;
}

void sr_diff_native(sr_diff_frame_t *f) {
    image_save(&f->after);
    image_load(&f->before);
    if(f->extra) {
        memcpy(f->extra_after, f->extra, f->extra_len);
        memcpy(f->extra, f->extra_before, f->extra_len);
    }
}

static unsigned compare(const char *what, uint32_t base, const uint8_t *want, const uint8_t *got, size_t size,
                        unsigned reported) {
    size_t i;
    unsigned n = 0;
    if(memcmp(want, got, size) == 0) return 0;
    for(i = 0; i + 4 <= size; i += 4) {
        uint32_t a, b;
        memcpy(&a, want + i, 4);
        memcpy(&b, got + i, 4);
        if(a == b) continue;
        if(reported + n < SR_DIFF_REPORT)
            fprintf(stderr, "sr_diff:     %s %#08lx lifted %08x native %08x\n", what, (unsigned long)(base + i), a, b);
        n++;
    }
    return n ? n : 1;
}

extern int i960_host_race_tick_now(void);

void sr_diff_end(sr_diff_frame_t *f, const char *name) {
    sr_diff_stat_t *st = stat_for(name);
    sr_diff_image_t native;
    unsigned n = 0;

    /* The native end state is the live one; compare it in place against the lifted end state. */
    native.crx = model2_crx_ram;
    native.work = model2_workram;
    native.buffer = model2_bufferram;
    native.comm = model2_m2comm_share_ptr();
    native.backup = model2_backup_sram_ptr();
    native.tgp = malloc(s_tgp_size);
    model2_tgp_state_save(native.tgp);
    st->calls++;
    if(memcmp(f->after.crx, native.crx, SR_CRX_SIZE) || memcmp(f->after.work, native.work, SR_WORK_SIZE) ||
       memcmp(f->after.buffer, native.buffer, SR_BUFFER_SIZE) || memcmp(f->after.comm, native.comm, SR_COMM_SIZE) ||
       memcmp(f->after.backup, native.backup, SR_BACKUP_SIZE) || memcmp(f->after.tgp, native.tgp, s_tgp_size) ||
       memcmp(f->after.tile_map, host_tile_map(), SR_TILE_MAP_SIZE) ||
       (f->extra && memcmp(f->extra_after, f->extra, f->extra_len)) ||
       (f->ret_set && f->ret_lifted != f->ret_native)) {
        if(st->mismatches < 3)
            fprintf(stderr, "sr_diff: %s differs (race tick %d, call %lu)\n", name, i960_host_race_tick_now(),
                    st->calls);
        if(st->mismatches < 3) {
            n += compare("crx", SR_CRX_BASE, f->after.crx, native.crx, SR_CRX_SIZE, n);
            n += compare("work", SR_WORK_BASE, f->after.work, native.work, SR_WORK_SIZE, n);
            n += compare("buffer", SR_BUFFER_BASE, f->after.buffer, native.buffer, SR_BUFFER_SIZE, n);
            n += compare("comm", SR_COMM_BASE, f->after.comm, native.comm, SR_COMM_SIZE, n);
            n += compare("backup", SR_BACKUP_BASE, f->after.backup, native.backup, SR_BACKUP_SIZE, n);
            n += compare("tgp", 0, f->after.tgp, native.tgp, s_tgp_size, n);
            n += compare("tile_map", SR_TILE_MAP_BASE, f->after.tile_map, host_tile_map(), SR_TILE_MAP_SIZE, n);
            if(f->extra) n += compare("frame", 0, f->extra_after, f->extra, f->extra_len, n);
            if(f->ret_set && f->ret_lifted != f->ret_native)
                fprintf(stderr, "sr_diff:     return lifted %08x native %08x\n", f->ret_lifted, f->ret_native);
        }
        st->mismatches++;
        if(s_stop) {
            summary();
            exit(1);
        }
    }
    free(native.tgp);
    image_load(&f->after);
    if(f->extra) memcpy(f->extra, f->extra_after, f->extra_len);
    s_depth--;
}
