/* The race HUD: see include/race/hud.h. Each routine follows its lifted original (src/game/game_start_race_hud_*.c,
 * draw_scene_dispatch.c, erase_scene_dispatch.c, boot_tile_map_fill.c, tile_map_bank_clear.c,
 * tile_attract_time_split.c, game_start_race_cam_matrix_long.c). */
#include "race/hud.h"
#include "model2_tgp.h"
#include "race/course.h"
#include "race/course_hooks.h"
#include "race/fp.h"
#include "sr_mem.h"
#include "sr_tgp.h"

uint8_t *sr_tile_map;
int sr_tile_map_dirty = 1;
uint64_t sr_tile_rows_dirty[4] = {~0ull, ~0ull, ~0ull, ~0ull};
float sr_hud_needle_matrix[12];
uint32_t sr_hud_needle_tick;

#define L2_MAP 0x01000000u
#define L1_MAP 0x01004000u
#define BATCHES 0x20b950u       /* scene batches: u16 colorbase, u32 record (8 bytes each) */
#define BATCH_LIMIT 0x20c954u

static void tile_st16(uint32_t va, uint32_t v) {
    if(sr_tile_map && va - SR_TILE_MAP_BASE < SR_TILE_MAP_SIZE - 1u) {
        uint8_t *p = sr_tile_map + (va - SR_TILE_MAP_BASE);
        if(p[0] != (uint8_t)v || p[1] != (uint8_t)(v >> 8)) {
            const uint32_t off = va - SR_TILE_MAP_BASE;
            sr_tile_map_dirty = 1;
            if(off < 0x8000u) sr_tile_rows_dirty[off >> 13] |= 1ull << ((off & 0x1fffu) >> 7);
        }
        p[0] = (uint8_t)v;
        p[1] = (uint8_t)(v >> 8);
    }
}

static int32_t s16(uint32_t v) { return (int32_t)(int16_t)(uint16_t)v; }

/* The cell address of (x, y) in the map the flags pick. */
static uint32_t cell_va(uint32_t x, uint32_t y, uint32_t flags) {
    uint32_t va = ((y << 6) + x) * 2u + ((flags & 1u) ? L1_MAP : L2_MAP);
    if(flags & 2u) va += 0x2000u;
    return va;
}

static int in_range(uint32_t batch, uint32_t glyph, uint32_t flags) {
    if((int32_t)batch < 0) return 0;
    if(batch + glyph >= sr_ld32(BATCH_LIMIT)) return 0;
    return (flags & 7u) <= 3u;
}

uint32_t sr_draw_scene(uint32_t x, uint32_t y, uint32_t batch, uint32_t glyph, uint32_t flags) {
    const uint32_t ix = batch + glyph;
    uint32_t base, record, va, pix, rows, row;
    if(!in_range(batch, glyph, flags)) return (uint32_t)-1; /* the original's error print (not seen in the race) */
    va = cell_va(x, y, flags);
    base = (flags & 1u) ? 0u : 0x8000u; /* L2 cells carry the priority bit */
    if(flags & 8u) base |= 0x4000u;
    record = sr_ld32(BATCHES + ix * 8u + 4u);
    if(record == 0) return ix;
    base |= sr_ld16(BATCHES + ix * 8u);
    rows = (uint32_t)s16(sr_ld16(record + 18u));
    pix = record + 0x14u;
    for(row = 0; (int32_t)row < (int32_t)rows; row++) {
        int32_t col;
        for(col = 0; col < s16(sr_ld16(record + 16u)); col++) {
            tile_st16(va, (sr_ld16(pix) + base) & 0xffffu);
            pix += 2u;
            va += 2u;
        }
        va += (uint32_t)(64 - s16(sr_ld16(record + 16u))) << 1;
        rows = (uint32_t)s16(sr_ld16(record + 18u));
    }
    return ix;
}

uint32_t sr_erase_scene(uint32_t x, uint32_t y, uint32_t batch, uint32_t glyph, uint32_t flags) {
    const uint32_t ix = batch + glyph;
    uint32_t record, va, rows, row;
    if(!in_range(batch, glyph, flags)) return (uint32_t)-1;
    va = cell_va(x, y, flags);
    record = sr_ld32(BATCHES + ix * 8u + 4u);
    if(record == 0) return ix;
    rows = (uint32_t)s16(sr_ld16(record + 18u));
    for(row = 0; (int32_t)row < (int32_t)rows; row++) {
        int32_t col;
        for(col = 0; col < s16(sr_ld16(record + 16u)); col++) {
            tile_st16(va, 0);
            va += 2u;
        }
        va += (uint32_t)(64 - s16(sr_ld16(record + 16u))) << 1;
        rows = (uint32_t)s16(sr_ld16(record + 18u));
    }
    return ix;
}

void sr_tile_map_fill(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    uint32_t row_base = (y << 7) + (x << 1), row, col;
    for(row = 0; row < height && width; row++) {
        for(col = 0; col < width; col++) tile_st16(L2_MAP + row_base + col * 2u, 0);
        row_base += 0x80u;
    }
}

void sr_tile_banks_clear(void) {
    uint32_t i;
    for(i = 0; i < 0x2000u; i += 2) {
        tile_st16(L2_MAP + i, 0);
        tile_st16(L1_MAP + i, 0);
    }
}

void sr_time_split(uint32_t value, uint16_t out[4]) {
    const int32_t v = (int32_t)value;
    out[0] = (uint16_t)(v / 0x34bc0);
    out[1] = (uint16_t)((v / 0xe10) % 60);
    out[2] = (uint16_t)((v / 60) % 60);
    out[3] = (uint16_t)(((v % 60) * 100 + 0x1e) / 60);
}

/* A cached digit at (x, y): drawn when it changed. */
static void digit(uint32_t cache, uint32_t value, uint32_t x, uint32_t y, uint32_t batch) {
    if(sr_ld32(cache) == value) return;
    sr_st32(cache, value);
    sr_draw_scene(x, y, batch, value, 0);
}

void sr_hud_layers(uint32_t g0) {
    uint32_t y_base = sr_ld32(0x2139d4u) == 0 ? 3u : 1u;
    if(g0 != 0) {
        uint16_t split[4];
        uint32_t y, batch;
        if(sr_race_course_index() == 3u && sr_ld32(0x202230u) == 0) {
            /* the lake in the championship: a blinking overlay instead */
            if(((sr_ld32(0x202008u) & ~31u) & 0xffu) == 0) return;
            if(sr_ld32(0x202008u) & (1u << 5)) sr_draw_scene(0, y_base + 2u, sr_ld32(0x20ac94u), 0, 0);
            else sr_erase_scene(0, y_base + 2u, sr_ld32(0x20ac94u), 0, 0);
            return;
        }
        y = y_base + 2u;
        sr_time_split(sr_ld32(0x2020b8u), split);
        batch = sr_ld32(0x20ac88u);
        digit(0x20ab04u, (uint32_t)split[1] / 10u, 1, y, batch);
        digit(0x20aaf0u, (uint32_t)split[1] % 10u, 3, y, batch);
        digit(0x20aaf4u, (uint32_t)split[2] / 10u, 6, y, batch);
        digit(0x20aaf8u, (uint32_t)split[2] % 10u, 8, y, batch);
        digit(0x20aafcu, (uint32_t)split[3] / 10u, 11, y, batch);
        digit(0x20ab00u, (uint32_t)split[3] % 10u, 13, y, batch);
        return;
    }
    sr_draw_scene(3, y_base, sr_ld32(0x20ac88u), 24, 0);
    sr_draw_scene(3, y_base + 5u, sr_ld32(0x20ac88u), 23, 0);
    y_base += 2u;
    sr_draw_scene(5, y_base, sr_ld32(0x20ac88u), 21, 0);
    sr_draw_scene(10, y_base, sr_ld32(0x20ac88u), 22, 0);
    sr_st32(0x20aaf0u, (uint32_t)-1);
    sr_st32(0x20aaf4u, (uint32_t)-1);
    sr_st32(0x20aaf8u, (uint32_t)-1);
    sr_st32(0x20aafcu, (uint32_t)-1);
    sr_st32(0x20ab00u, (uint32_t)-1);
    sr_st32(0x20ab04u, 0);
}

void sr_hud_tach(void) {
    int32_t v = ((int32_t)sr_ld32(0x20b0b0u) + 0x3b) / 60;
    uint32_t slot_base, x, y, batch;
    if(v > 0x63) v = 0x63;
    if((uint32_t)v == sr_ld32(0x20aeb8u)) return;
    sr_st32(0x20aeb8u, (uint32_t)v);
    slot_base = (sr_ld32(0x20aeb4u) != 0 || v <= 5) ? 10u : 0u;
    x = sr_ld32(0x20aebcu);
    y = sr_ld32(0x20aec0u);
    batch = sr_ld32(0x20ac7cu);
    if(v <= 9) sr_erase_scene(x, y, batch, 0, 0);
    else sr_draw_scene(x, y, batch, slot_base + (uint32_t)((v / 10) % 10), 0);
    sr_draw_scene(x + 4u, y, batch, slot_base + (uint32_t)(v % 10), 0);
}

void sr_hud_cam_matrix(void) {
    const uint32_t cam = sr_ld32(0x2140ccu);
    uint32_t cursor, geo_wr, bump;
    double ang, scaled, roll;
    if(cam == 0) return;
    sr_draw_scene(10, 42, sr_ld32(0x20ac78u), sr_ld32(cam + 0x78u) + 1u, 0);
    /* the HUD's GEO window and focus */
    sr_hook_course_reg(0x800030u, 0);
    sr_hook_course_prg(0x80u);
    sr_hook_course_prg(0x01f00200u);
    sr_hook_course_prg(0x00f80140u);
    sr_hook_course_prg(0x00f80140u);
    sr_hook_course_prg(0x00f80140u);
    sr_hook_course_prg(0x00f80140u);
    sr_st32(0x202270u, 0x43000000u);
    sr_st32(0x202274u, 0x43000000u);
    sr_hook_course_reg(0x800090u, 0);
    sr_hook_course_prg(0x43000000u);
    sr_hook_course_prg(0x43000000u);
    /* the needle: push, identity, T(-185, -127, 128), scale 40, roll by the engine speed */
    {
        static const uint32_t t[3] = {0xc3390000u, 0xc2fe0000u, 0x43000000u};
        static const uint32_t s[3] = {0x42200000u, 0x42200000u, 0x42200000u};
        sr_tgp_cmd(0x10002020u, 0, 0, 0, 0);
        sr_tgp_cmd(0x12802525u, 0, 0, 0, 0);
        sr_tgp_cmd(0x13802727u, t, 3, 0, 0);
        sr_tgp_cmd(0x14002828u, s, 3, 0, 0);
    }
    ang = D(sr_ld32(cam + 0x7cu));
    if(ang < sr_d(0, 0x40890000u)) ang = D(0x44480000u); /* 800 */
    scaled = ang / sr_d(0x9999999au, 0x3ff19999u);         /* / 1.1 */
    {
        const double bound = sr_d(0, 0x40af4000u);
        const double quarter = sr_d(0x54442d18u, 0x3fe921fbu);
        const double half_pi = sr_d(0x54442d18u, 0x3ff921fbu);
        if(scaled < bound) roll = (scaled * quarter) / bound - half_pi;
        else roll = ((scaled - bound) * quarter) / sr_d(0, 0x408f4000u) - quarter;
    }
    {
        const uint32_t r = F(roll);
        sr_tgp_cmd(0x15802b2bu, &r, 1, 0, 0);
    }
    model2_tgp_matrix(sr_hud_needle_matrix); /* what the GEO object gets with the 0x05 below */
    sr_hud_needle_tick = sr_ld32(0x202008u);
    cursor = sr_ld32(0x20a290u);
    geo_wr = sr_hook_course_write_ptr();
    if(cursor != 0) sr_st32(cursor, geo_wr);
    sr_tgp_cmd(0x02800505u, 0, 0, 0, 0);
    sr_hook_course_reg(0x801008u, geo_wr + 0x34u);
    sr_hook_course_bank(sr_ld32(0x202278u) & 3u);
    {
        const uint32_t c0 = sr_ld32(0x02865130u), c1 = sr_ld32(0x02865134u), c2 = sr_ld32(0x02865138u),
                       c3 = sr_ld32(0x0286513cu);
        bump = sr_ld32(0x20b940u);
        sr_hook_course_catalog(c0, c1, c2, c3);
        sr_tgp_cmd(0x10802121u, 0, 0, 0, 0);
        sr_st32(0x20a290u, cursor + 4u);
        sr_st32(0x20b940u, bump + c3);
    }
}

void sr_hud_lap_time(uint32_t lap) {
    uint16_t split[4];
    uint32_t y, batch;
    if((int32_t)lap < 0) {
        sr_st32(0x20ab10u, (uint32_t)-1);
        sr_st32(0x20ab14u, (uint32_t)-1);
        sr_st32(0x20ab18u, (uint32_t)-1);
        sr_st32(0x20ab1cu, (uint32_t)-1);
        sr_st32(0x20ab20u, (uint32_t)-1);
        return;
    }
    y = lap * 3u + (sr_ld32(0x2139d4u) == 0 ? 10u : 8u);
    sr_time_split(sr_ld32(0x2020d0u + lap * 4u), split);
    batch = sr_ld32(0x20ac88u);
    digit(0x20ab10u, split[1], 3, y, batch);
    digit(0x20ab14u, (uint32_t)split[2] / 10u, 6, y, batch);
    digit(0x20ab18u, (uint32_t)split[2] % 10u, 8, y, batch);
    digit(0x20ab1cu, (uint32_t)split[3] / 10u, 11, y, batch);
    digit(0x20ab20u, (uint32_t)split[3] % 10u, 13, y, batch);
}

void sr_hud_position(uint32_t pos) {
    const uint32_t y_base = sr_ld32(0x2139d4u) == 0 ? 3u : 1u;
    const uint32_t batch = sr_ld32(0x20ac80u);
    uint32_t count;
    if(pos == 0) {
        sr_draw_scene(48, y_base, batch, 0, 0);
        sr_draw_scene(54, y_base + 2u, batch, 1, 0);
        sr_st32(0x20ab3cu, (uint32_t)-1);
        sr_st32(0x20ab40u, (uint32_t)-1);
        return;
    }
    count = sr_ld32(0x213978u);
    if(sr_ld32(0x20ab3cu) != count) {
        sr_st32(0x20ab3cu, count);
        sr_draw_scene(56, y_base + 4u, batch, (sr_ld32(0x202230u) == 0 ? 15u : count) + 1u, 0);
    }
    if(sr_ld32(0x20ab40u) != pos) {
        sr_st32(0x20ab40u, pos);
        sr_draw_scene(46, y_base + 2u, batch, pos + 17u, 0);
    }
}

/* TGP 0x11 (the reciprocal the original divides by) */
static double tgp_inverse(double ratio) {
    const uint32_t in = F(ratio);
    uint32_t out = 0;
    sr_tgp_cmd(0x08801111u, &in, 1, &out, 1);
    return D(out);
}

void sr_hud_speed(uint32_t g0) {
    int32_t speed;
    uint32_t batch, hundreds, tens, ones, had_hundreds;
    if(g0 == 0) {
        sr_draw_scene(20, 43, sr_ld32(0x20ac84u), 0, 0);
        return;
    }
    if((int32_t)sr_ld32(0x20a560u) <= 0) {
        double scaled = D(sr_ld32(0x20b0b4u)) * sr_d(0, 0x406b0000u); /* x 216 */
        if(scaled < 0.0) scaled = -scaled;
        speed = (int32_t)scaled;
    } else {
        const double ratio = sr_d(0x78722053u, 0x3f66f604u) / D(sr_ld32(0x214124u));
        const double prod = D(sr_ld32(0x20b0b4u)) * sr_d(0, 0x406b0000u);
        double scaled = prod / tgp_inverse(ratio);
        if(scaled < 0.0) scaled = -(prod / tgp_inverse(ratio));
        speed = (int32_t)scaled;
    }
    if(speed < 0) speed = -speed;
    hundreds = (uint32_t)(speed / 100);
    ones = (uint32_t)(speed % 10);
    tens = (uint32_t)((speed / 10) % 10);
    batch = sr_ld32(0x20ac88u);
    if(hundreds != 0) {
        sr_draw_scene(14, 43, batch, hundreds % 10u, 0);
        had_hundreds = 1;
    } else {
        sr_tile_map_fill(14, 43, 2, 3);
        had_hundreds = 0;
    }
    if(had_hundreds || tens != 0) sr_draw_scene(16, 43, batch, tens, 0);
    else sr_tile_map_fill(16, 43, 2, 3);
    sr_draw_scene(18, 43, batch, ones, 0);
}

void sr_hud_progress(uint32_t g0) {
    uint32_t tab, scale, i, full, rem;
    int32_t cells;
    double frac;
    if(g0 == 0) {
        sr_draw_scene(8, 0, sr_ld32(0x20ac90u), 9, 0);
        sr_st32(0x20ab44u, (uint32_t)-1);
        return;
    }
    tab = sr_ld32(0x005dcac8u + sr_race_course_index() * 12u);
    scale = sr_ld32(tab);
    {
        const uint32_t num = sr_ld32(0x2020c4u) * scale;
        double num_f = (double)(int32_t)num;
        const double den_f = (double)(int32_t)sr_ld32(0x2140c4u);
        if((int32_t)num < 0) num_f += D(0x4f800000u);
        frac = num_f == 0.0 ? 0.0 : den_f / num_f;
    }
    if(frac < 0.0) frac = 0.0;
    if(frac > 1.0) frac = 1.0;
    cells = (int32_t)(frac * sr_d(0, 0x40750000u)); /* x 336 */
    if((uint32_t)cells == sr_ld32(0x20ab44u)) return;
    sr_st32(0x20ab44u, (uint32_t)cells);
    full = (uint32_t)(cells + 7) >> 3;
    if(full != 0) full -= 1u;
    rem = (uint32_t)cells - (full << 3);
    for(i = 0; i < full; i++) sr_draw_scene(i + 9u, 2, sr_ld32(0x20ac90u), 8, 0);
    sr_draw_scene(i + 9u, 2, sr_ld32(0x20ac90u), rem, 0);
    if(i <= 40u) sr_draw_scene(i + 10u, 2, sr_ld32(0x20ac90u), 0, 0);
}

void sr_hud_frame(void) {
    sr_hud_layers(1);
    sr_hud_tach();
    sr_hud_cam_matrix();
    sr_hud_lap_time(sr_ld32(0x2020c0u));
    sr_hud_position(sr_ld32(0x2020c8u) + 1u);
    sr_hud_speed(1);
    if(sr_ld32(0x2139d4u) == 0) sr_hud_progress(1);
}
