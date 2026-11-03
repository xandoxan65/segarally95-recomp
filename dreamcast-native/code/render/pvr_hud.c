/* The race HUD's 2D layer on the PVR (render/hud2d.h for what is drawn).
 *
 * The HUD is kept as a picture of its screen: one 512 x 512 texture of 4-bit palette indices (128 KiB of VRAM)
 * where screen cell (x, y) is the 8 x 8 block at texel (8x, 8y), holding the front-most map's character there (the
 * race draws on map 0 only). In a twiddled texture an aligned 8 x 8 block is 32 contiguous bytes, so a cell is one
 * 32-byte copy, and only the rows the HUD routines changed are copied again (sr_tile_rows_dirty). Each row is drawn
 * as runs of neighbouring cells with the same palette line, one PVR sprite a run (64 bytes to the TA, not 128 a
 * cell: the TA's intake is what the HUD costs). Palette lines go to the PVR's 64 banks of 16 entries as they are
 * first used; pen 0 and black are transparent. Sprites in the punch-through list, after the 3D, depth compare off:
 * the HUD is over everything. */
#include "render/pvr_backend.h"
#include "render/ta.h"
#include "render/hud2d.h"
#include "render/twiddle.h"
#include "race/hud.h"
#include "sr_alloc.h"
#include "sr_mem.h"
#include <dc/pvr.h>
#include <arch/timer.h>
#include <stdio.h>

#define TEX 512u /* texels across */
#define COLS (SR_HUD_W / 8u)
#define ROWS (SR_HUD_H / 8u)
#define BANKS 64u
/* The needle's colour, measured on the reference's frames: the pack's flat colour for this one polygon comes out
 * black (its luma is taken from the course's lighting, not the HUD's). */
#define NEEDLE_ARGB 0xffe08030u

typedef struct {
    uint8_t x, len;
    uint16_t colour;
} run_t;

static pvr_sprite_hdr_t s_hdr[BANKS] __attribute__((aligned(32)));
static pvr_ptr_t s_vram;
static const uint8_t *s_chars, *s_palette;
static uint8_t s_visible[SR_HUD_CHARS / 8];
static run_t s_runs[ROWS][COLS];
static uint32_t s_cell[ROWS][COLS];
static uint8_t s_tw8[64];
static uint8_t s_map_used[4]; /* the map has had a cell (the race only ever writes map 0) */ /* twiddled index of (x, y) in an 8 x 8 block, at y * 8 + x */ /* what each texture block holds: code | colour << 16 | 1 << 31 drawn; 0 empty */
static uint8_t s_n_runs[ROWS];
static int16_t s_bank_of[256]; /* palette line -> PVR bank, -1 none yet */
static uint16_t s_bank_line[BANKS];
static uint32_t s_banks_used;
static int s_ready;
static uint64_t s_prof_update, s_prof_emit, s_prof_rows, s_prof_sprites, s_prof_frames; /* sr_pvr_hud_report */
static pvr_poly_hdr_t s_needle_hdr __attribute__((aligned(32)));
static const sr_course_mesh_t *s_needle; /* the tach needle's GEO object 0x801a7a in the course pack */

int sr_pvr_hud_init(const uint8_t *chars, const uint8_t *palette, const sr_course_t *course) {
    uint32_t c, b;
    void *zero;
    if(!chars || !palette) return -1;
    for(c = 0; c < course->n_meshes; c++)
        if(course->meshes[c].oba == 0x801a7au) s_needle = &course->meshes[c];
    {
        pvr_poly_cxt_t cxt;
        pvr_poly_cxt_col(&cxt, PVR_LIST_PT_POLY);
        cxt.gen.culling = PVR_CULLING_NONE;
        cxt.depth.comparison = PVR_DEPTHCMP_ALWAYS;
        cxt.depth.write = PVR_DEPTHWRITE_DISABLE;
        pvr_poly_compile(&s_needle_hdr, &cxt);
    }
    s_vram = pvr_mem_malloc(TEX * TEX / 2u);
    zero = sr_alloc_zero(TEX * TEX / 2u);
    if(!s_vram || !zero) {
        printf("sr: no VRAM for the HUD (free %lu KiB)\n", (unsigned long)(pvr_mem_available() >> 10));
        return -1;
    }
    pvr_txr_load(zero, s_vram, TEX * TEX / 2u);
    free(zero);
    sr_fill(s_cell, 0, sizeof(s_cell));
    for(c = 0; c < 64u; c++) s_tw8[c] = (uint8_t)sr_twiddle_index(c & 7u, c >> 3, 8, 8);
    sr_hud2d_scan_chars(chars, s_visible);
    for(c = 0; c < 256u; c++) s_bank_of[c] = -1;
    s_banks_used = 0;
    pvr_set_pal_format(PVR_PAL_ARGB1555);
    for(b = 0; b < BANKS; b++) {
        pvr_sprite_cxt_t cxt;
        pvr_sprite_cxt_txr(&cxt, PVR_LIST_PT_POLY, PVR_TXRFMT_PAL4BPP | PVR_TXRFMT_4BPP_PAL(b) | PVR_TXRFMT_TWIDDLED,
                           TEX, TEX, s_vram, PVR_FILTER_NONE);
        cxt.gen.culling = PVR_CULLING_NONE;
        cxt.depth.comparison = PVR_DEPTHCMP_ALWAYS;
        cxt.depth.write = PVR_DEPTHWRITE_DISABLE;
        cxt.txr.env = PVR_TXRENV_REPLACE;
        pvr_sprite_compile(&s_hdr[b], &cxt);
    }
    s_chars = chars;
    s_palette = palette;
    s_ready = 1;
    return 0;
}

void sr_pvr_hud_reset_palette(void) {
    uint32_t c;
    for(c = 0; c < 256u; c++) s_bank_of[c] = -1;
    s_banks_used = 0;
}

/* The PVR bank holding palette line `line`, loading it into a free bank (or the oldest) the first time. */
static uint32_t bank_for(uint32_t line) {
    uint32_t b, i;
    if(s_bank_of[line] >= 0) return (uint32_t)s_bank_of[line];
    b = s_banks_used++ % BANKS;
    if(s_banks_used > BANKS) s_bank_of[s_bank_line[b]] = -1;
    s_bank_of[line] = (int16_t)b;
    s_bank_line[b] = (uint16_t)line;
    for(i = 0; i < 16u; i++) {
        const uint32_t pen = (line * 16u + i) & 2047u;
        const uint16_t c = (uint16_t)(s_palette[pen * 2u] | s_palette[pen * 2u + 1u] << 8);
        pvr_set_pal_entry(b * 16u + i, i ? sr_hud2d_argb1555(c) : 0);
    }
    return b;
}

/* Row y again: each cell's front-most visible character into the texture where it changed, and the row's runs. */
static void update_row(uint32_t y) {
    uint32_t x, n = 0;
    int32_t last_x = -2;
    for(x = 0; x < COLS; x++) {
        uint32_t code = 0, colour = 0, px, py;
        int found = 0, map;
        for(map = 0; map < 4 && !found; map++) {
            const uint8_t *p;
            if(!s_map_used[map]) continue;
            p = sr_tile_map + (uint32_t)map * 0x2000u + (y * 64u + x) * 2u;
            const uint32_t cell = (uint32_t)p[0] | (uint32_t)p[1] << 8;
            code = cell & 0x3fffu;
            if((cell & 0x8000u) && (s_visible[code >> 3] & (1u << (code & 7u)))) {
                colour = (cell >> 7) & 0xffu;
                found = 1;
            }
        }
        {
            const uint32_t want = found ? (0x80000000u | colour << 16 | code) : 0;
            if(want != s_cell[y][x]) {
                /* the block in VRAM: 32 bytes, one store-queue burst (single stores to VRAM wait on the PVR) */
                static uint32_t words[8] __attribute__((aligned(32)));
                uint32_t w;
                for(w = 0; w < 8u; w++) words[w] = 0;
                if(found) {
                    const uint8_t *row = s_chars + code * 32u;
                    for(py = 0; py < 8u; py++, row += 4) {
                        /* a row's 8 pixels: two little-endian words, the leftmost pixel in the top nibble */
                        const uint32_t pix = (uint32_t)row[1] << 24 | (uint32_t)row[0] << 16 | (uint32_t)row[3] << 8 | row[2];
                        for(px = 0; px < 8u; px++) {
                            const uint32_t i = s_tw8[py * 8u + px]; /* 8 texels a word, the first lowest */
                            words[i >> 3] |= ((pix >> (28u - 4u * px)) & 15u) << ((i & 7u) * 4u);
                        }
                    }
                }
                pvr_txr_load(words, (uint8_t *)s_vram + sr_twiddle_index(x * 8u, y * 8u, TEX, TEX) / 2u, sizeof(words));
                s_cell[y][x] = want;
            }
        }
        if(found) {
            if(n && last_x == (int32_t)x - 1 && s_runs[y][n - 1].colour == colour)
                s_runs[y][n - 1].len++;
            else
                s_runs[y][n++] = (run_t){(uint8_t)x, 1, (uint16_t)colour};
            last_x = (int32_t)x;
        }
    }
    s_n_runs[y] = (uint8_t)n;
}

void sr_pvr_hud_submit(void) {
    const float sx = 640.f / SR_HUD_W, sy = 480.f / SR_HUD_H, t = 1.f / TEX;
    uint64_t dirty;
    int map;
    uint32_t y, r, bank = BANKS;
    uint64_t t0, t1;
    if(!s_ready || !sr_tile_map) return;
    t0 = timer_us_gettime64();
    /* a map with written rows may have cells now: look through those rows once */
    for(map = 0; map < 4; map++)
        if(!s_map_used[map] && sr_tile_rows_dirty[map]) {
            const uint16_t *cells = (const uint16_t *)(sr_tile_map + (uint32_t)map * 0x2000u);
            uint32_t i;
            for(i = 0; i < 64u * ROWS && !s_map_used[map]; i++) s_map_used[map] = cells[i] != 0;
        }
    dirty = sr_tile_rows_dirty[0] | sr_tile_rows_dirty[1] | sr_tile_rows_dirty[2] | sr_tile_rows_dirty[3];
    sr_tile_rows_dirty[0] = sr_tile_rows_dirty[1] = sr_tile_rows_dirty[2] = sr_tile_rows_dirty[3] = 0;
    for(y = 0; dirty && y < ROWS; y++, dirty >>= 1)
        if(dirty & 1u) {
            update_row(y);
            s_prof_rows++;
        }
    t1 = timer_us_gettime64();
    s_prof_update += t1 - t0;
    s_prof_frames++;
    for(y = 0; y < ROWS; y++)
        for(r = 0; r < s_n_runs[y]; r++) {
            const run_t *run = &s_runs[y][r];
            const uint32_t b = bank_for(run->colour);
            const float x0 = run->x * 8.f * sx, y0 = y * 8.f * sy, x1 = (run->x + run->len) * 8.f * sx, y1 = (y + 1) * 8.f * sy;
            const float u0 = run->x * 8.f * t, v0 = y * 8.f * t, u1 = (run->x + run->len) * 8.f * t, v1 = (y + 1) * 8.f * t;
            pvr_sprite_txr_t *first, *second;
            if(b != bank) {
                pvr_sprite_hdr_t *h = sr_ta_target();
                *h = s_hdr[b];
                sr_ta_commit(h);
                bank = b;
            }
            /* corners a, b, c, d clockwise from the top left; d's texture coordinates follow from the others. The
             * 64-byte sprite takes two consecutive 32-byte targets: each half is sent once it is filled. */
            first = sr_ta_target();
            second = sr_ta_target();
            second = (pvr_sprite_txr_t *)((uint8_t *)second - 32);
            first->flags = PVR_CMD_VERTEX_EOL;
            first->ax = x0, first->ay = y0, first->az = 1.f;
            first->bx = x1, first->by = y0, first->bz = 1.f;
            first->cx = x1;
            sr_ta_commit(first);
            second->cy = y1, second->cz = 1.f;
            second->dx = x0, second->dy = y1;
            second->auv = PVR_PACK_16BIT_UV(u0, v0);
            second->buv = PVR_PACK_16BIT_UV(u1, v0);
            second->cuv = PVR_PACK_16BIT_UV(u1, v1);
            sr_ta_commit((uint8_t *)second + 32);
            s_prof_sprites++;
        }
    s_prof_emit += timer_us_gettime64() - t1;
}

void sr_pvr_hud_report(void) {
    if(s_prof_frames)
        printf("sr: HUD a frame: rows updated %lu, update %lu us, sprites %lu, emit %lu us\n",
               (unsigned long)(s_prof_rows / s_prof_frames), (unsigned long)(s_prof_update / s_prof_frames),
               (unsigned long)(s_prof_sprites / s_prof_frames), (unsigned long)(s_prof_emit / s_prof_frames));
}

/* The needle: its polygon through the matrix the HUD frame gave the TGP, projected with the HUD window's focus 128
 * (1:1 pixels at z = 128), over the cells. */
void sr_pvr_hud_needle(void) {
    const float *m = sr_hud_needle_matrix;
    uint32_t t, k;
    /* only when the last tick's HUD frame placed it (0x202008 counts the ticks; the needle is set before the count) */
    if(!s_ready || !s_needle || sr_hud_needle_tick == 0 || sr_ld32(0x202008u) - sr_hud_needle_tick > 1u) return;
    {
        pvr_poly_hdr_t *h = sr_ta_target();
        *h = s_needle_hdr;
        sr_ta_commit(h);
    }
    for(t = 0; t < s_needle->n_tris; t++)
        for(k = 0; k < 3; k++) {
            const sr_course_vertex_t *p = &s_needle->tris[t].v[k];
            const float x = m[0] * p->x + m[3] * p->y + m[6] * p->z + m[9];
            const float y = m[1] * p->x + m[4] * p->y + m[7] * p->z + m[10];
            const float z = m[2] * p->x + m[5] * p->y + m[8] * p->z + m[11];
            pvr_vertex_t *v = sr_ta_target(); /* z is 128: the needle is never behind the eye */
            v->flags = k == 2 ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;
            v->x = (248.f + 128.f * x / z) * (640.f / SR_HUD_W);
            v->y = (192.f - 128.f * y / z) * (480.f / SR_HUD_H);
            v->z = 1.f;
            v->u = v->v = 0.f;
            v->argb = NEEDLE_ARGB;
            v->oargb = 0;
            sr_ta_commit(v);
        }
}
