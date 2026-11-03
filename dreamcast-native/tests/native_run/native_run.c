/* sr-native-run: the native race tick on its own, on the host, from a reference snapshot.
 *
 *   sr-native-run SNAPSHOT INPUTS TRACE [TICKS]
 *
 * SNAPSHOT is segamod2 --snapshot FILE@N (state after race tick N); INPUTS the same --inputs script the reference
 * ran with; TRACE gets one row per race tick from N+1 on, in --trace-race's format, for
 * tools/compare_race_trace.py.
 *
 * Absolute ticks: a snapshot named FILE@tN (or SR_SNAPSHOT_TICK=tN) is the reference's --snapshot FILE@tN, after
 * absolute tick N, e.g. in the countdown before GO. TICKS then counts ticks, rows carry the absolute tick and the
 * race tick as the reference counts it (-1 before GO), and the controls follow the reference's rules: the wheel
 * idle (0x80, 0, 0, neutral) until the clock runs, input row r for race tick r from GO on (the GO tick's poll
 * samples row 0). The run stops after the tick that leaves the race scene (0x2020ac changes). ROMs come from out/i960 (maincpu, main data) and the ROM directory (copro data),
 * as for segamod2. Nothing of the lifted code is linked: only dreamcast-native's code and the shared TGP. */
#include "preview.h"
#include "race/hud.h"
#include "race/particles.h"
#include "race/checkpoint.h"
#include "race/course.h"
#include "race/io_poll.h"
#include "race/race_tick.h"
#include "sr_mem.h"
#include "sr_tgp.h"
#include "model2_tgp.h"
#include "model2_tgp_fw.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t *read_file(const char *path, size_t *size);

/* SR_DUMP_AT=N: the native RAM after race tick N (absolute tick N in the absolute-tick mode) as
 * build/trace/native@N.snap (snapshot layout, without TGP; with TILE when the run has a tile map). */
static void dump_ram(uint32_t race_tick) {
    char path[128];
    FILE *f;
    const struct { const char *tag; const void *p; uint32_t n; } sec[] = {
        {"CRX ", 0, SR_CRX_SIZE}, {"WORK", 0, SR_WORK_SIZE}, {"BUFF", 0, SR_BUFFER_SIZE},
        {"COMM", 0, SR_COMM_SIZE}, {"BKUP", 0, SR_BACKUP_SIZE}};
    const void *ptr[5] = {sr_mem.crx, sr_mem.work, sr_mem.buffer, sr_mem.comm, sr_mem.backup};
    const uint32_t tile_size = SR_TILE_MAP_SIZE;
    unsigned i;
    snprintf(path, sizeof(path), "build/trace/native@%u.snap", race_tick);
    f = fopen(path, "wb");
    if(!f) return;
    fwrite("SRSNAP01", 1, 8, f);
    for(i = 0; i < 5; i++) {
        fwrite(sec[i].tag, 1, 4, f);
        fwrite(&sec[i].n, 4, 1, f);
        fwrite(ptr[i], 1, sec[i].n, f);
    }
    if(sr_tile_map) {
        fwrite("TILE", 1, 4, f);
        fwrite(&tile_size, 4, 1, f);
        fwrite(sr_tile_map, 1, SR_TILE_MAP_SIZE, f);
    }
    fclose(f);
}

/* Pages (4 KiB of main data, 1 Ki words of copro data) the run reads; SR_PAGES=FILE writes them out. */
static uint8_t s_main_pages[0x2000000 >> 12];
static uint8_t s_copro_pages[0x1000000 >> 10];

void sr_mem_trace_main(uint32_t off) { s_main_pages[off >> 12] = 1; }

static void trace_copro(u32 word_addr) {
    if(word_addr < (sizeof(s_copro_pages) << 10)) s_copro_pages[word_addr >> 10] = 1;
}

static void write_pages(const char *path) {
    FILE *f = fopen(path, "w");
    size_t i, nm = 0, nc = 0;
    if(!f) return;
    fprintf(f, "# pages read by sr-native-run: main <4 KiB page> / copro <1 Ki-word page>\n");
    for(i = 0; i < sizeof(s_main_pages); i++)
        if(s_main_pages[i]) fprintf(f, "main %zu\n", i), nm++;
    for(i = 0; i < sizeof(s_copro_pages); i++)
        if(s_copro_pages[i]) fprintf(f, "copro %zu\n", i), nc++;
    fclose(f);
    fprintf(stderr, "sr-native-run: %zu main data pages (%zu KiB), %zu copro pages (%zu KiB)\n", nm, nm * 4, nc,
            nc * 4);
}

/* SR_PACK=FILE: everything the Dreamcast lockstep run needs (dreamcast-native/code/main.c reads it). */
static int pack_section(FILE *f, const char *tag, const void *data, uint32_t size) {
    return fwrite(tag, 1, 4, f) == 4 && fwrite(&size, 4, 1, f) == 1 && (!size || fwrite(data, 1, size, f) == size);
}

static void write_pack(const char *path, const uint8_t *snap, size_t snap_size, const char *inputs_path,
                       uint32_t start, uint32_t ticks, uint32_t absolute, const uint32_t *copro, uint32_t copro_words) {
    FILE *f = fopen(path, "wb");
    size_t off = 8, i, n;
    uint8_t *buf;
    /* start and ticks count race ticks, or with absolute = 1 absolute ticks (a snapshot before GO, FILE@tN) */
    uint32_t meta[3] = {start, ticks, absolute}, count, page;
    int ok;
    if(!f) {
        fprintf(stderr, "sr-native-run: cannot write %s\n", path);
        exit(2);
    }
    ok = fwrite("SRPACK01", 1, 8, f) == 8 && pack_section(f, "META", meta, sizeof(meta))
        && pack_section(f, "ROM ", sr_mem.rom, SR_ROM_SIZE);
    /* Main data and copro pages: count, then (page, 4 KiB) each. */
    for(count = 0, i = 0; i < sizeof(s_main_pages); i++) count += s_main_pages[i] != 0;
    ok = ok && fwrite("MAIN", 1, 4, f) == 4;
    n = 4 + count * (4 + 4096u);
    ok = ok && fwrite(&(uint32_t){(uint32_t)n}, 4, 1, f) == 1 && fwrite(&count, 4, 1, f) == 1;
    for(i = 0; ok && i < sizeof(s_main_pages); i++) {
        if(!s_main_pages[i]) continue;
        page = (uint32_t)i;
        ok = fwrite(&page, 4, 1, f) == 1 && fwrite(sr_mem.main_data + (i << 12), 1, 4096, f) == 4096;
    }
    for(count = 0, i = 0; i < sizeof(s_copro_pages); i++) count += s_copro_pages[i] && ((i + 1) << 10) <= copro_words;
    ok = ok && fwrite("COPR", 1, 4, f) == 4;
    n = 4 + count * (4 + 4096u);
    ok = ok && fwrite(&(uint32_t){(uint32_t)n}, 4, 1, f) == 1 && fwrite(&count, 4, 1, f) == 1;
    for(i = 0; ok && i < sizeof(s_copro_pages); i++) {
        if(!s_copro_pages[i] || ((i + 1) << 10) > copro_words) continue;
        page = (uint32_t)i;
        ok = fwrite(&page, 4, 1, f) == 1 && fwrite(copro + (i << 10), 1, 4096, f) == 4096;
    }
    /* The snapshot's sections as they are. */
    while(ok && off + 8 <= snap_size) {
        uint32_t len;
        memcpy(&len, snap + off + 4, 4);
        ok = pack_section(f, (const char *)snap + off, snap + off + 8, len);
        off += 8 + len;
    }
    buf = read_file(inputs_path, &n);
    ok = ok && pack_section(f, "INPT", buf, (uint32_t)n);
    free(buf);
    if(!ok || fclose(f)) {
        fprintf(stderr, "sr-native-run: writing %s failed\n", path);
        exit(2);
    }
    fprintf(stderr, "sr-native-run: pack %s\n", path);
}

static uint8_t *read_file(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    uint8_t *buf;
    long n;
    if(!f || fseek(f, 0, SEEK_END) || (n = ftell(f)) < 0 || fseek(f, 0, SEEK_SET)) {
        fprintf(stderr, "sr-native-run: cannot read %s\n", path);
        exit(2);
    }
    buf = malloc((size_t)n ? (size_t)n : 1);
    if(!buf || fread(buf, 1, (size_t)n, f) != (size_t)n) {
        fprintf(stderr, "sr-native-run: cannot read %s\n", path);
        exit(2);
    }
    fclose(f);
    *size = (size_t)n;
    return buf;
}

/* A section's bytes, or NULL when the snapshot has none (older snapshots lack the 2D layer). */
static const uint8_t *find_section(const uint8_t *snap, size_t size, const char *tag, uint32_t *len) {
    size_t off = 8;
    while(off + 8 <= size) {
        memcpy(len, snap + off + 4, 4);
        if(memcmp(snap + off, tag, 4) == 0) return snap + off + 8;
        off += 8 + *len;
    }
    return NULL;
}

static void load_section(const uint8_t *snap, size_t size, const char *tag, void *dst, size_t want) {
    size_t off = 8;
    while(off + 8 <= size) {
        uint32_t len;
        memcpy(&len, snap + off + 4, 4);
        if(memcmp(snap + off, tag, 4) == 0) {
            if(len != want) {
                fprintf(stderr, "sr-native-run: section %.4s is %u bytes, want %zu\n", tag, len, want);
                exit(2);
            }
            memcpy(dst, snap + off + 8, len);
            return;
        }
        off += 8 + len;
    }
    fprintf(stderr, "sr-native-run: snapshot has no %.4s section\n", tag);
    exit(2);
}

typedef struct {
    uint32_t tick, steer, accel, brake, gear, view; /* view: VIEW CHANGE held (optional sixth column) */
} input_row_t;

static input_row_t *load_inputs(const char *path, size_t *count) {
    FILE *f = fopen(path, "r");
    input_row_t *rows = NULL;
    size_t n = 0, cap = 0;
    char line[256];
    if(!f) {
        fprintf(stderr, "sr-native-run: cannot open %s\n", path);
        exit(2);
    }
    while(fgets(line, sizeof(line), f)) {
        unsigned t, ge, vw = 0;
        int st, ac, br;
        if(line[0] == '#' || sscanf(line, "%u %i %i %i %u %u", &t, &st, &ac, &br, &ge, &vw) < 5) continue;
        if(n == cap) rows = realloc(rows, (cap = cap ? cap * 2 : 256) * sizeof(*rows));
        rows[n++] = (input_row_t){t, (uint32_t)st, (uint32_t)ac, (uint32_t)br, ge, vw != 0};
    }
    fclose(f);
    *count = n;
    return rows;
}

static void trace_row(FILE *f, uint32_t tick, int32_t race_tick, const input_row_t *in) {
    uint32_t i;
    fprintf(f, "T %u %d %u %u %u %d %u %u %u %u %u %08x", tick, (int)race_tick, sr_ld32(0x202098u), sr_ld32(0x20209cu),
            sr_ld32(0x2020acu), (int)(int32_t)sr_ld32(0x214120u), in->steer, in->accel, in->brake, in->gear, 1u,
            0x500100u);
    for(i = 0; i < 64; i++) fprintf(f, " %08x", sr_ld32(0x500100u + i * 4));
    for(i = 0; i < 64; i++) fprintf(f, " %08x", sr_ld32(0x500000u + i * 4));
    fputc('\n', f);
}

/* Absolute-tick mode (see the top). */
/* SR_COURSE / SR_ATTACH previews, for both modes */
static sr_course_t s_course;
static sr_actors_t s_actors;
static int s_course_loaded, s_actors_loaded;
static uint32_t s_render_every = 300;

static void preview_at(const char *tag, uint32_t n) {
    char path[96];
    float vm[12];
    if(!s_course_loaded || n % s_render_every) return;
    snprintf(path, sizeof(path), "build/trace/render%s@%u.ppm", tag, n);
    if(model2_tgp_view_matrix(vm)) sr_preview_render(&s_course, s_actors_loaded ? &s_actors : NULL, vm, path);
}

static int run_abs(FILE *trace, uint32_t start, uint32_t ticks, const input_row_t *inputs, size_t n_inputs) {
    const input_row_t idle = {0, 0x80, 0, 0, 0, 0};
    input_row_t cur = idle;
    const uint32_t scene = sr_ld32(0x2020acu);
    int32_t race_tick = -1;
    uint32_t hash = 2166136261u, tick;
    size_t k = 0;

    if((int32_t)sr_ld32(0x214120u) > 0) {
        if(!getenv("SR_RACE_TICK")) {
            fprintf(stderr, "sr-native-run: the snapshot is after GO: give its race tick in SR_RACE_TICK\n");
            return 2;
        }
        race_tick = (int32_t)strtol(getenv("SR_RACE_TICK"), NULL, 0);
    }
    for(tick = start + 1; tick <= start + ticks; tick++) {
        const int running = (int32_t)sr_ld32(0x214120u) > 0;
        if(running)
            while(k < n_inputs && (int32_t)inputs[k].tick <= race_tick + 1) cur = inputs[k++];
        else
            cur = idle;
        sr_io_board_set(cur.steer, cur.accel, cur.brake, cur.gear);
        sr_io_board_view((int)cur.view);
        sr_race_tick();
        if(!running && (int32_t)sr_ld32(0x214120u) > 0) {
            /* GO this tick: the reference's poll already sampled input row 0. Poll again with it (the switch edges
             * against the idle poll are the reference's edges against the previous tick's). */
            while(k < n_inputs && inputs[k].tick <= 0) cur = inputs[k++];
            sr_io_board_set(cur.steer, cur.accel, cur.brake, cur.gear);
            sr_io_board_view((int)cur.view);
            sr_io_poll();
        }
        if((int32_t)sr_ld32(0x214120u) > 0) race_tick++;
        if(getenv("SR_DUMP_AT") && tick == (uint32_t)strtoul(getenv("SR_DUMP_AT"), NULL, 0)) dump_ram(tick);
        preview_at("_t", tick);
        trace_row(trace, tick, race_tick, &cur);
        hash = sr_checkpoint_fold(hash);
        if((tick - start) % SR_CHECKPOINT_TICKS == 0) printf("sr: checkpoint %u %08x\n", tick, hash);
        if(sr_ld32(0x2020acu) != scene) {
            printf("sr: tick %u (race tick %d) leaves the race scene (0x2020ac %u -> %u)\n", tick, (int)race_tick,
                   scene, sr_ld32(0x2020acu));
            break;
        }
    }
    fclose(trace);
    return 0;
}

int main(int argc, char **argv) {
    size_t snap_size, rom_size, data_size, n_inputs, k = 0;
    uint8_t *snap, *tgp_state;
    input_row_t *inputs, cur = {0, 0x80, 0, 0, 0, 0};
    sr_course_t course;
    sr_actors_t actors;
    int course_loaded = 0, actors_loaded = 0, abs_ticks;
    uint32_t render_every = 300;
    uint32_t hash = 2166136261u, start, race_tick, ticks = argc > 4 ? (uint32_t)strtoul(argv[4], NULL, 0) : 3000;
    FILE *trace;
    const char *at;

    if(argc < 4) {
        fprintf(stderr, "usage: sr-native-run SNAPSHOT INPUTS TRACE [TICKS]\n");
        return 2;
    }
    snap = read_file(argv[1], &snap_size);
    if(snap_size < 8 || memcmp(snap, "SRSNAP01", 8)) {
        fprintf(stderr, "sr-native-run: %s is not a segamod2 snapshot\n", argv[1]);
        return 2;
    }
    at = getenv("SR_SNAPSHOT_TICK") ? getenv("SR_SNAPSHOT_TICK") - 1 : strrchr(argv[1], '@');
    abs_ticks = at && at[1] == 't';
    start = at ? (uint32_t)strtoul(at + 1 + abs_ticks, NULL, 0) : 0;

    sr_mem.rom = read_file("out/i960/maincpu_deinterleaved.bin", &rom_size);
    sr_mem.main_data = read_file("out/i960/main_data_deinterleaved.bin", &data_size);
    sr_mem.main_data_size = (uint32_t)data_size;
    sr_mem.crx = calloc(1, SR_CRX_SIZE);
    sr_mem.work = calloc(1, SR_WORK_SIZE);
    sr_mem.buffer = calloc(1, SR_BUFFER_SIZE);
    sr_mem.comm = calloc(1, SR_COMM_SIZE);
    sr_mem.backup = calloc(1, SR_BACKUP_SIZE);
    sr_mem_map();
    load_section(snap, snap_size, "CRX ", sr_mem.crx, SR_CRX_SIZE);
    load_section(snap, snap_size, "WORK", sr_mem.work, SR_WORK_SIZE);
    load_section(snap, snap_size, "BUFF", sr_mem.buffer, SR_BUFFER_SIZE);
    load_section(snap, snap_size, "COMM", sr_mem.comm, SR_COMM_SIZE);
    load_section(snap, snap_size, "BKUP", sr_mem.backup, SR_BACKUP_SIZE);
    {
        uint32_t len;
        const uint8_t *tile = find_section(snap, snap_size, "TILE", &len);
        if(tile && len == SR_TILE_MAP_SIZE) {
            sr_tile_map = malloc(SR_TILE_MAP_SIZE);
            memcpy(sr_tile_map, tile, SR_TILE_MAP_SIZE);
            /* the 2D layer's graphics, for the previews' HUD */
            sr_preview_chars = find_section(snap, snap_size, "CHAR", &len);
            sr_preview_palette = find_section(snap, snap_size, "PALR", &len);
        }
    }

    sr_tgp_init(NULL, 0); /* host ops and reset; the copro data comes from the ROM directory below */
    if(model2_tgp_fw_load_copro_data_default() != 0) {
        fprintf(stderr, "sr-native-run: copro data ROM not found\n");
        return 2;
    }
    tgp_state = malloc(model2_tgp_state_size());
    load_section(snap, snap_size, "TGP ", tgp_state, model2_tgp_state_size());
    model2_tgp_state_load(tgp_state);

    model2_tgp_fw_read_trace = trace_copro;
    /* SR_COURSE=pack: render previews every SR_RENDER_EVERY ticks (default 300). */
    if(getenv("SR_COURSE")) {
        size_t n;
        const uint8_t *data = read_file(getenv("SR_COURSE"), &n);
        if(sr_course_parse(&course, data, n)) {
            fprintf(stderr, "sr-native-run: %s is not a course pack\n", getenv("SR_COURSE"));
            return 2;
        }
        course_loaded = 1;
        /* SR_ATTACH=file: also draw the cars and the helicopter */
        if(getenv("SR_ATTACH")) {
            const uint8_t *att = read_file(getenv("SR_ATTACH"), &n);
            if(sr_actors_parse(&actors, &course, att, n)) {
                fprintf(stderr, "sr-native-run: %s is not an attach file for this pack\n", getenv("SR_ATTACH"));
                return 2;
            }
            actors_loaded = 1;
            fprintf(stderr, "sr-native-run: %u attachments, shadow outline %u points\n", actors.n, actors.shadow_n);
        }
        /* SR_LIGHT=file: light the cars per triangle as the arcade does */
        if(getenv("SR_LIGHT")) {
            const uint8_t *lit = read_file(getenv("SR_LIGHT"), &n);
            fprintf(stderr, "sr-native-run: %d lit meshes\n", sr_actors_light_load(&course, lit, n));
        }
        if(getenv("SR_RENDER_EVERY")) render_every = (uint32_t)strtoul(getenv("SR_RENDER_EVERY"), NULL, 0);
    }
    sr_practice_skip = 1;
    sr_demo_endless_time = getenv("SR_ENDLESS_TIME") != NULL;
    sr_io_board_reset();
    inputs = load_inputs(argv[2], &n_inputs);
    trace = fopen(argv[3], "w");
    if(!trace) {
        fprintf(stderr, "sr-native-run: cannot write %s\n", argv[3]);
        return 2;
    }
    fprintf(trace, "# sr-native-run from %s after %s tick %u\n", argv[1], abs_ticks ? "absolute" : "race", start);
    if(abs_ticks) {
        s_course = course, s_actors = actors;
        s_course_loaded = course_loaded, s_actors_loaded = actors_loaded, s_render_every = render_every;
        {
            const int rc = run_abs(trace, start, ticks, inputs, n_inputs);
            if(rc) return rc;
        }
        goto pages; /* run_abs closed the trace */
    }
    /* The poll at the end of race tick r samples input row r, as the reference applies them (the snapshot already
     * holds the sample of row `start`). */
    for(race_tick = start + 1; race_tick <= start + ticks; race_tick++) {
        while(k < n_inputs && inputs[k].tick <= race_tick) cur = inputs[k++];
        sr_io_board_set(cur.steer, cur.accel, cur.brake, cur.gear);
        sr_io_board_view((int)cur.view);
        sr_race_tick();
        trace_row(trace, 0, (int32_t)race_tick, &cur);
        hash = sr_checkpoint_fold(hash);
        if(getenv("SR_DUMP_AT") && race_tick == (uint32_t)strtoul(getenv("SR_DUMP_AT"), NULL, 0)) dump_ram(race_tick);
        if(getenv("SR_PARTICLES_AT") && race_tick == (uint32_t)strtoul(getenv("SR_PARTICLES_AT"), NULL, 0)) {
            const sr_particle_draw_t *d;
            const uint32_t n = sr_particles_draws(&d);
            uint32_t i;
            float vm[12];
            model2_tgp_view_matrix(vm);
            for(i = 0; i < n && i < 8; i++)
                fprintf(stderr, "sr-native-run: particle %u oba %#x kind %u size %g pos %.2f %.2f %.2f view z %.2f\n", i,
                        d[i].oba, d[i].kind, d[i].size, d[i].pos[0], d[i].pos[1], d[i].pos[2],
                        vm[2] * d[i].pos[0] + vm[5] * d[i].pos[1] + vm[8] * d[i].pos[2] + vm[11]);
        }
        if(course_loaded && race_tick % render_every == 0) {
            char path[96];
            float vm[12];
            snprintf(path, sizeof(path), "build/trace/render@%u.ppm", race_tick);
            if(model2_tgp_view_matrix(vm)) sr_preview_render(&course, actors_loaded ? &actors : NULL, vm, path);
        }
        if((race_tick - start) % SR_CHECKPOINT_TICKS == 0) printf("sr: checkpoint %u %08x\n", race_tick, hash);
    }
    fclose(trace);
pages:
    if(getenv("SR_PAGES")) write_pages(getenv("SR_PAGES"));
    /* SR_PAGES_UNION="a.txt b.txt": add pages other drives read (SR_PAGES files), so the pack covers more of the
     * course than this run drove. The copro range is then filled without gaps (4 pages margin) and each main data
     * page gets its neighbours: the course's tables lie together. */
    if(getenv("SR_PAGES_UNION")) {
        char list[1024], *tok;
        size_t i;
        long lo = -1, hi = -1;
        snprintf(list, sizeof(list), "%s", getenv("SR_PAGES_UNION"));
        for(tok = strtok(list, " "); tok; tok = strtok(NULL, " ")) {
            FILE *f = fopen(tok, "r");
            char kind[16];
            unsigned long page;
            if(!f) continue;
            while(fscanf(f, "%15s %lu", kind, &page) == 2) {
                if(!strcmp(kind, "main") && page < sizeof(s_main_pages)) s_main_pages[page] = 1;
                if(!strcmp(kind, "copro") && page < sizeof(s_copro_pages)) s_copro_pages[page] = 1;
            }
            fclose(f);
        }
        for(i = 0; i < sizeof(s_copro_pages); i++)
            if(s_copro_pages[i]) {
                if(lo < 0) lo = (long)i;
                hi = (long)i;
            }
        for(i = (size_t)(lo > 4 ? lo - 4 : 0); lo >= 0 && i <= (size_t)hi + 4 && i < sizeof(s_copro_pages); i++)
            s_copro_pages[i] = 1;
        for(i = sizeof(s_main_pages) - 1; i > 0; i--)
            if(s_main_pages[i] == 1) {
                s_main_pages[i - 1] |= 2;
                if(i + 1 < sizeof(s_main_pages)) s_main_pages[i + 1] |= 2;
            }
    }
    if(getenv("SR_PACK")) {
        unsigned nw = 0;
        const uint32_t *words = (const uint32_t *)model2_tgp_fw_copro_words(&nw);
        write_pack(getenv("SR_PACK"), snap, snap_size, argv[2], start, ticks, (uint32_t)abs_ticks, words, nw);
    }
    return 0;
}
