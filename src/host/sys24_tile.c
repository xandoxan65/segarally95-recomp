/* System-24 tile chip — lift harness port of MAME segas24_tile_device.
 *
 * license: BSD-3-Clause (MAME segaic24.cpp, Olivier Galibert)
 * Adapted for host C99 without device_t / tilemap_t.
 */

#include "sys24_tile.h"
#include "sys24_gfx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    SYS24_TILES           = 0x4000,
    SYS24_TILE_RAM_WORDS    = 0x10000 / 2,
    SYS24_CHAR_RAM_WORDS    = 0x80000 / 2,
    SYS24_LAYER_TILES       = 64 * 64,
    SYS24_LAYER_COUNT       = 4,
    SYS24_PEN_COUNT         = 2048,
    /* emu/tilemap.h — flagsmap = LAYER0 | category (low 4 bits). */
    TILEMAP_PIXEL_LAYER0    = 0x10,
    TILEMAP_DRAW_OPAQUE     = 0x02
};

typedef struct {
    u16 pixmap[SYS24_PIXMAP_WIDTH * SYS24_PIXMAP_HEIGHT];
    u8  flags[SYS24_PIXMAP_WIDTH * SYS24_PIXMAP_HEIGHT];
} sys24_layer_cache_t;

struct sys24_tile_state {
    u16                 tile_mask;
    const u8           *tile_map_bytes;
    const u8           *char_ram_bytes;
    const u16          *tile_ram;
    const u16          *char_ram;
    sys24_layer_cache_t layers[SYS24_LAYER_COUNT];
    /* MAME mark_tile_dirty equivalent — scroll must not invalidate. */
    u32                 content_sig;
    int                 layers_valid;
};

/* MAME tile_layer[] map bases; standard draw_common uses tile_layer[layer>>1] only. */
static const u32 s_layer_map_offset[SYS24_LAYER_COUNT] = {
    0x0000u, 0x1000u, 0x2000u, 0x3000u
};
/* Pair index → tilemap used by draw_common when ctrl&0x6000==0 (MAME line 462). */

static u32 rgb15_pen(const u8 *palram, u16 pen)
{
    const u16 *p = (const u16 *)palram;
    u16 c;
    u32 r;
    u32 g;
    u32 b;

    if (!palram)
        return 0xff000000u;
    if (pen >= SYS24_PEN_COUNT)
        pen = (u16)(pen % SYS24_PEN_COUNT);
    c = (u16)(p[pen] & 0x7fffu);
    /* Model 2 color15: xGGGGGRRRRRBBBBB — not PC RGB555 (tools/model2_palette rgb15). */
    r = ((c >> 0) & 31u) * 255u / 31u;
    g = ((c >> 5) & 31u) * 255u / 31u;
    b = ((c >> 10) & 31u) * 255u / 31u;
    return 0xff000000u | (r << 16) | (g << 8) | b;
}

sys24_tile_state_t *sys24_tile_create(u16 tile_mask)
{
    sys24_tile_state_t *st;

    st = (sys24_tile_state_t *)calloc(1, sizeof(*st));
    if (!st)
        return NULL;
    st->tile_mask = tile_mask ? tile_mask : SYS24_TILE_MASK_M2;
    return st;
}

void sys24_tile_destroy(sys24_tile_state_t *st)
{
    free(st);
}

void sys24_tile_bind(sys24_tile_state_t *st, const u8 *tile_map, const u8 *char_ram)
{
    if (!st)
        return;
    /* New pointers → force rebuild even if bytes look identical. */
    if (st->tile_map_bytes != tile_map || st->char_ram_bytes != char_ram)
        st->layers_valid = 0;
    st->tile_map_bytes = tile_map;
    st->char_ram_bytes = char_ram;
    st->tile_ram = tile_map ? (const u16 *)(const void *)tile_map : NULL;
    st->char_ram = char_ram ? (const u16 *)(const void *)char_ram : NULL;
}

/* FNV-ish over tile name tables + char — excludes hscr line tables (scroll-only).
 * MAME segaic24 mark_tile_dirty on map/char writes; scroll is draw-time only. */
static u32 tile_content_sig(const sys24_tile_state_t *st)
{
    u32 h = 2166136261u;
    unsigned i;
    const u8 *map;

    if (!st || !st->tile_map_bytes)
        return 0;
    map = st->tile_map_bytes;
    /* Layer maps: 4 × 0x1000 words = 0x8000 bytes @ tile_ram[0]. */
    for (i = 0; i < 0x8000u; i++)
        h = (h ^ map[i]) * 16777619u;
    if (st->char_ram_bytes) {
        /* Char RAM is large; sample densely enough for CGM glyph uploads. */
        for (i = 0; i < 0x10000u; i += 4u)
            h = (h ^ st->char_ram_bytes[i]) * 16777619u;
    }
    h ^= (u32)st->tile_mask;
    return h;
}

static void refresh_one_layer(sys24_tile_state_t *st, int layer_idx)
{
    sys24_layer_cache_t *L;
    u32 map_base;
    int ty;
    int tx;

    L = &st->layers[layer_idx];
    map_base = s_layer_map_offset[layer_idx];
    memset(L->pixmap, 0, sizeof(L->pixmap));
    memset(L->flags, 0, sizeof(L->flags));

    if (!st->tile_ram || !st->char_ram_bytes)
        return;

    for (ty = 0; ty < 64; ty++) {
        for (tx = 0; tx < 64; tx++) {
            u32 tidx = (u32)ty * 64u + (u32)tx;
            u16 val = st->tile_ram[map_base + tidx];
            u16 code = (u16)(val & st->tile_mask);
            u8 pal = (u8)((val >> 7) & 0xffu);
            /* MAME tileinfo.category = (val & 0x8000) != 0; opaque pens get
             * flagsmap = TILEMAP_PIXEL_LAYER0 | category. */
            u8 tile_cat = (u8)((val & 0x8000u) ? 1u : 0u);
            int dx = tx * 8;
            int dy = ty * 8;
            int py;
            int px;

            for (py = 0; py < 8; py++) {
                for (px = 0; px < 8; px++) {
                    u8 nib = sys24_gfx_pixel(st->char_ram_bytes, code, px, py);
                    int pxi = (dy + py) * SYS24_PIXMAP_WIDTH + (dx + px);
                    if (nib == 0) {
                        L->pixmap[pxi] = 0;
                        L->flags[pxi] = 0;
                    } else {
                        L->pixmap[pxi] = (u16)(pal * 16u + nib);
                        L->flags[pxi] = (u8)(TILEMAP_PIXEL_LAYER0 | tile_cat);
                    }
                }
            }
        }
    }
}

void sys24_tile_refresh(sys24_tile_state_t *st)
{
    int i;

    if (!st)
        return;
    for (i = 0; i < SYS24_LAYER_COUNT; i++)
        refresh_one_layer(st, i);
    st->content_sig = tile_content_sig(st);
    st->layers_valid = 1;
}

void sys24_tile_ensure_refreshed(sys24_tile_state_t *st)
{
    u32 sig;

    if (!st)
        return;
    sig = tile_content_sig(st);
    if (st->layers_valid && sig == st->content_sig)
        return;
    sys24_tile_refresh(st);
}

/*
 * MAME segas24_tile_device::draw_rect (bitmap_rgb32).
 * tile_layer[*]->set_transparent_pen(0): pen 0 leaves the destination
 * untouched unless TILEMAP_DRAW_OPAQUE. Writing pens[0] as solid RGB made
 * priority CGMs cover car-select L/R panel holes (cars looked centred).
 */
static int draw_pen_ok(u16 pen, u8 layer_pri, u16 tpri, int flags)
{
    if (!(flags & TILEMAP_DRAW_OPAQUE) && pen == 0u)
        return 0;
    return layer_pri == tpri || (flags & TILEMAP_DRAW_OPAQUE) != 0;
}

static void draw_rect_rgb32(
    const u16 *source_base,
    const u8 *trans_base,
    u32 *dest_base,
    int dest_stride,
    const u16 *mask,
    u16 tpri,
    int flags,
    int win,
    int sx,
    int sy,
    int xx1,
    int yy1,
    int xx2,
    int yy2,
    const u32 *pens)
{
    const u16 *source = source_base + sy * SYS24_PIXMAP_WIDTH + sx;
    const u8 *trans = trans_base + sy * SYS24_PIXMAP_WIDTH + sx;
    u32 *dest = dest_base + yy1 * dest_stride + xx1;
    int y;

    tpri = (u16)(tpri | TILEMAP_PIXEL_LAYER0);
    mask += yy1 * 4;
    yy2 -= yy1;

    while (xx1 >= 128) {
        xx1 -= 128;
        xx2 -= 128;
        mask++;
    }

    for (y = 0; y < yy2; y++) {
        const u16 *src = source;
        const u8 *srct = trans;
        u32 *dst = dest;
        const u16 *mask1 = mask;
        int llx = xx2;
        int cur_x = xx1;

        while (llx > 0) {
            u16 m = *mask1++;

            if (win)
                m = (u16)~m;

            if (!cur_x && llx >= 128) {
                if (!m) {
                    int x;
                    for (x = 0; x < 128; x++) {
                        if (draw_pen_ok(*src, *srct, tpri, flags))
                            *dst = pens[*src];
                        src++;
                        srct++;
                        dst++;
                    }
                } else if (m == 0xffffu) {
                    src += 128;
                    srct += 128;
                    dst += 128;
                } else {
                    int x;
                    for (x = 0; x < 128; x += 8) {
                        if (!(m & 0x8000u)) {
                            int xx;
                            for (xx = 0; xx < 8; xx++) {
                                if (draw_pen_ok(src[xx], srct[xx], tpri, flags))
                                    dst[xx] = pens[src[xx]];
                            }
                        }
                        src += 8;
                        srct += 8;
                        dst += 8;
                        m = (u16)(m << 1);
                    }
                }
            } else {
                int llx1 = llx >= 128 ? 128 : llx;

                if (!m) {
                    int x;
                    for (x = cur_x; x < llx1; x++) {
                        if (draw_pen_ok(*src, *srct, tpri, flags))
                            *dst = pens[*src];
                        src++;
                        srct++;
                        dst++;
                    }
                } else if (m == 0xffffu) {
                    src += 128 - cur_x;
                    srct += 128 - cur_x;
                    dst += 128 - cur_x;
                } else {
                    int x;
                    for (x = cur_x; x < llx1; x++) {
                        if (draw_pen_ok(*src, *srct, tpri, flags) &&
                            !(m & (u16)(0x8000u >> (x >> 3)))) {
                            *dst = pens[*src];
                        }
                        src++;
                        srct++;
                        dst++;
                    }
                }
            }
            llx -= 128;
            cur_x = 0;
        }
        source += SYS24_PIXMAP_WIDTH;
        trans += SYS24_PIXMAP_WIDTH;
        dest += dest_stride;
        mask += 4;
    }
}

/* Draw one tilemap cache with scroll + win mask (MAME draw_rect path).
 * scroll_pair selects the hscr line table (pair index after layer>>=1). */
static void draw_one_map_rgb32(
    sys24_tile_state_t *st,
    u32 *bitmap,
    int map_idx,
    int scroll_pair,
    u16 hscr,
    u16 vscr,
    const u16 *mask,
    u16 tpri,
    int flags,
    int win,
    const u32 *pens)
{
    const u16 *tile_ram = st->tile_ram;
    const sys24_layer_cache_t *layer_cache = &st->layers[map_idx];
    const u16 *bm = layer_cache->pixmap;
    const u8 *tm = layer_cache->flags;

    if (hscr & 0x8000u) {
        int y;
        const u16 *hscrtb = tile_ram + 0x4000u + 0x200u * (u32)scroll_pair;
        u16 v = (u16)(vscr & 0x1ffu);

        for (y = 0; y < SYS24_FB_HEIGHT; y++) {
            u16 h = (u16)((-hscrtb[y]) & 0x1ffu);
            if (h + SYS24_FB_WIDTH <= SYS24_PIXMAP_WIDTH) {
                draw_rect_rgb32(bm, tm, bitmap, SYS24_FB_WIDTH, mask, tpri, flags, win,
                                h, v, 0, y, SYS24_FB_WIDTH, y + 1, pens);
            } else {
                draw_rect_rgb32(bm, tm, bitmap, SYS24_FB_WIDTH, mask, tpri, flags, win,
                                h, v, 0, y, SYS24_PIXMAP_WIDTH - h, y + 1, pens);
                draw_rect_rgb32(bm, tm, bitmap, SYS24_FB_WIDTH, mask, tpri, flags, win,
                                0, v, SYS24_PIXMAP_WIDTH - h, y, SYS24_FB_WIDTH, y + 1,
                                pens);
            }
            v = (u16)((v + 1u) & 0x1ffu);
        }
    } else {
        u16 h = (u16)((-hscr) & 0x1ffu);
        u16 v = (u16)((+vscr) & 0x1ffu);

        if (h + SYS24_FB_WIDTH <= SYS24_PIXMAP_WIDTH) {
            if (v + SYS24_FB_HEIGHT <= SYS24_PIXMAP_WIDTH) {
                draw_rect_rgb32(bm, tm, bitmap, SYS24_FB_WIDTH, mask, tpri, flags, win,
                                h, v, 0, 0, SYS24_FB_WIDTH, SYS24_FB_HEIGHT, pens);
            } else {
                draw_rect_rgb32(bm, tm, bitmap, SYS24_FB_WIDTH, mask, tpri, flags, win,
                                h, v, 0, 0, SYS24_FB_WIDTH, SYS24_PIXMAP_WIDTH - v, pens);
                draw_rect_rgb32(bm, tm, bitmap, SYS24_FB_WIDTH, mask, tpri, flags, win,
                                h, 0, 0, SYS24_PIXMAP_WIDTH - v, SYS24_FB_WIDTH,
                                SYS24_FB_HEIGHT, pens);
            }
        } else {
            if (v + SYS24_FB_HEIGHT <= SYS24_PIXMAP_WIDTH) {
                draw_rect_rgb32(bm, tm, bitmap, SYS24_FB_WIDTH, mask, tpri, flags, win,
                                h, v, 0, 0, SYS24_PIXMAP_WIDTH - h, SYS24_FB_HEIGHT, pens);
                draw_rect_rgb32(bm, tm, bitmap, SYS24_FB_WIDTH, mask, tpri, flags, win,
                                0, v, SYS24_PIXMAP_WIDTH - h, 0, SYS24_FB_WIDTH,
                                SYS24_FB_HEIGHT, pens);
            } else {
                draw_rect_rgb32(bm, tm, bitmap, SYS24_FB_WIDTH, mask, tpri, flags, win,
                                h, v, 0, 0, SYS24_PIXMAP_WIDTH - h,
                                SYS24_PIXMAP_WIDTH - v, pens);
                draw_rect_rgb32(bm, tm, bitmap, SYS24_FB_WIDTH, mask, tpri, flags, win,
                                0, v, SYS24_PIXMAP_WIDTH - h, 0, SYS24_FB_WIDTH,
                                SYS24_PIXMAP_WIDTH - v, pens);
                draw_rect_rgb32(bm, tm, bitmap, SYS24_FB_WIDTH, mask, tpri, flags, win,
                                h, 0, 0, SYS24_PIXMAP_WIDTH - v,
                                SYS24_PIXMAP_WIDTH - h, SYS24_FB_HEIGHT, pens);
                draw_rect_rgb32(bm, tm, bitmap, SYS24_FB_WIDTH, mask, tpri, flags, win,
                                0, 0, SYS24_PIXMAP_WIDTH - h, SYS24_PIXMAP_WIDTH - v,
                                SYS24_FB_WIDTH, SYS24_FB_HEIGHT, pens);
            }
        }
    }
}

/* One scanline span of a map (special window modes). win=0. */
static void draw_map_row_span(
    sys24_tile_state_t *st,
    u32 *bitmap,
    int map_idx,
    u16 src_x,
    u16 src_y,
    int x0,
    int y,
    int x1,
    const u16 *mask,
    u16 tpri,
    int flags,
    const u32 *pens)
{
    const sys24_layer_cache_t *layer_cache;
    const u16 *bm;
    const u8 *tm;
    int w;

    if (x1 <= x0 || y < 0 || y >= SYS24_FB_HEIGHT)
        return;
    if (x0 < 0)
        x0 = 0;
    if (x1 > SYS24_FB_WIDTH)
        x1 = SYS24_FB_WIDTH;
    w = x1 - x0;
    if (w <= 0)
        return;

    layer_cache = &st->layers[map_idx];
    bm = layer_cache->pixmap;
    tm = layer_cache->flags;
    src_x = (u16)(src_x & 0x1ffu);
    src_y = (u16)(src_y & 0x1ffu);

    if (src_x + (u16)w <= SYS24_PIXMAP_WIDTH) {
        draw_rect_rgb32(bm, tm, bitmap, SYS24_FB_WIDTH, mask, tpri, flags, 0,
                        src_x, src_y, x0, y, x1, y + 1, pens);
    } else {
        int first = SYS24_PIXMAP_WIDTH - (int)src_x;

        draw_rect_rgb32(bm, tm, bitmap, SYS24_FB_WIDTH, mask, tpri, flags, 0,
                        src_x, src_y, x0, y, x0 + first, y + 1, pens);
        draw_rect_rgb32(bm, tm, bitmap, SYS24_FB_WIDTH, mask, tpri, flags, 0,
                        0, src_y, x0 + first, y, x1, y + 1, pens);
    }
}

/*
 * MAME segas24_tile_device::draw_common special window (ctrl & 0x6000).
 * Ranking: scene_hud_alt stos 0x4000 → 0x20b91c → tile_ram[0x5004].
 *
 * Uses tilemap::draw after set_scrollx(0,-h), NOT draw_rect. MAME
 * effective_rowscroll = m_dx - scroll ⇒ set_scrollx(-h) samples pixmap
 * ((-h)&0x1ff) at screen x=0 (same negate as the standard line-scroll path).
 * No win mask — tilemap::draw ignores tile_ram[0x6000].
 */
static void draw_special_window_rgb32(
    sys24_tile_state_t *st,
    u32 *bitmap,
    int pair,
    u16 hscr,
    u16 vscr,
    u16 ctrl,
    u16 tpri,
    int flags,
    const u32 *pens)
{
    /* All-zero mask → draw_rect always takes the m==0 path (no win punch). */
    static const u16 zero_mask[384 * 4] = {0};
    const u16 *mask = zero_mask;
    const u16 *tile_ram = st->tile_ram;
    int mode = (int)((ctrl & 0x6000u) >> 13);
    int y;
    int layer = pair;

    /* MAME: set_scrolly(vscr & 0x1ff) on both maps of the pair. */
    if (hscr & 0x8000u) {
        const u16 *hscrtb = tile_ram + 0x4000u + 0x200u * (u32)pair;
        u16 sy0 = (u16)(vscr & 0x1ffu);

        switch (mode) {
        case 1: {
            u16 v = (u16)((-vscr) & 0x1ffu);

            if (!((-vscr) & 0x200))
                layer ^= 1;
            for (y = 0; y < SYS24_FB_HEIGHT; y++) {
                u16 line = hscrtb[y];
                u16 h = (u16)(line & 0x1ffu);
                /* set_scrollx(0,-h) → src = (-h)&0x1ff at screen 0 */
                u16 src = (u16)((-h) & 0x1ffu);
                int l1 = layer;

                if (y >= (int)v)
                    l1 ^= 1;
                draw_map_row_span(st, bitmap, l1, src,
                                  (u16)((sy0 + (u16)y) & 0x1ffu), 0, y,
                                  SYS24_FB_WIDTH, mask, tpri, flags, pens);
            }
            break;
        }
        case 2:
        case 3:
        default: {
            for (y = 0; y < SYS24_FB_HEIGHT; y++) {
                u16 line = hscrtb[y];
                u16 h = (u16)(line & 0x1ffu);
                u16 src = (u16)((-h) & 0x1ffu);
                u16 sy = (u16)((sy0 + (u16)y) & 0x1ffu);
                int l1 = pair;

                /* Both maps: set_scrollx(0,-h); clips c1=[0,h) c2=[h,W). */
                if (!(line & 0x200u))
                    l1 ^= 1;
                draw_map_row_span(st, bitmap, l1, src, sy, 0, y, (int)h, mask,
                                  tpri, flags, pens);
                draw_map_row_span(st, bitmap, l1 ^ 1,
                                  (u16)((src + h) & 0x1ffu), sy, (int)h, y,
                                  SYS24_FB_WIDTH, mask, tpri, flags, pens);
            }
            break;
        }
        }
    } else {
        u16 scroll_x = (u16)(hscr & 0x1ffu);
        u16 src0 = (u16)((-scroll_x) & 0x1ffu);
        u16 sy0 = (u16)(vscr & 0x1ffu);

        switch (mode) {
        case 1: {
            u16 v = (u16)((-vscr) & 0x1ffu);

            if (!((-vscr) & 0x200))
                layer ^= 1;
            for (y = 0; y < SYS24_FB_HEIGHT; y++) {
                int l1 = layer;

                if (y >= (int)v)
                    l1 ^= 1;
                draw_map_row_span(st, bitmap, l1, src0,
                                  (u16)((sy0 + (u16)y) & 0x1ffu), 0, y,
                                  SYS24_FB_WIDTH, mask, tpri, flags, pens);
            }
            break;
        }
        case 2:
        case 3:
        default: {
            u16 h = (u16)((+hscr) & 0x1ffu);

            if (!((+hscr) & 0x200))
                layer ^= 1;
            for (y = 0; y < SYS24_FB_HEIGHT; y++) {
                u16 sy = (u16)((sy0 + (u16)y) & 0x1ffu);

                draw_map_row_span(st, bitmap, layer, src0, sy, 0, y, (int)h,
                                  mask, tpri, flags, pens);
                draw_map_row_span(st, bitmap, layer ^ 1,
                                  (u16)((src0 + h) & 0x1ffu), sy, (int)h, y,
                                  SYS24_FB_WIDTH, mask, tpri, flags, pens);
            }
            break;
        }
        }
    }
}

/*
 * MAME segas24_tile_device::draw_common (RGB32).
 * ``layer`` is the Model 2 draw index 0..7 (see model2_v.cpp screen_update).
 * Ranking HUD (map @ 0x01002000 → tile_layer[1]) is win=1 in the standard
 * path; empty mask RAM skips it. scene_hud_alt stores 0x4000 into 0x20b91c
 * (→ tile_ram[0x5004] via boot_tile_splash_frame) so ctrl&0x6000 selects the
 * special window path.
 */
static void draw_common_rgb32(
    sys24_tile_state_t *st,
    u32 *bitmap,
    int layer,
    int flags,
    const u32 *pens)
{
    const u16 *tile_ram = st->tile_ram;
    u16 hscr;
    u16 vscr;
    u16 ctrl;
    const u16 *mask;
    u16 tpri;
    int win;
    int pair;

    hscr = tile_ram[0x5000u + (u32)(layer >> 1)];
    vscr = tile_ram[0x5004u + (u32)(layer >> 1)];
    ctrl = tile_ram[0x5004u + (u32)(((layer >> 1) & 2))];
    mask = tile_ram + (layer & 4 ? 0x6800u : 0x6000u);
    tpri = (u16)(layer & 1);
    pair = layer >> 1;

    if (vscr & 0x8000u)
        return;

    if (ctrl & 0x6000u) {
        if (pair & 1)
            return;
        draw_special_window_rgb32(st, bitmap, pair, hscr, vscr, ctrl, tpri,
                                  flags, pens);
        return;
    }

    win = pair & 1;
    draw_one_map_rgb32(st, bitmap, pair, pair, hscr, vscr, mask, tpri, flags, win, pens);
}

void sys24_tile_draw_layers_rgb32(sys24_tile_state_t *st, u32 *bitmap, const u8 *palram,
                                  u32 clear_color, unsigned pass)
{
    u32 pens[SYS24_PEN_COUNT];
    int i;
    int layer;

    if (!st || !bitmap)
        return;
    if (pass == 0)
        pass = SYS24_PASS_ALL;

    for (i = 0; i < SYS24_FB_WIDTH * SYS24_FB_HEIGHT; i++)
        bitmap[i] = clear_color;
    for (i = 0; i < SYS24_PEN_COUNT; i++)
        pens[i] = rgb15_pen(palram, (u16)i);

    sys24_tile_ensure_refreshed(st);

    /*
     * model2_v.cpp screen_update:
     *   B (6,4) opaque + A (2,0) → under polygons
     *   priority (7,5,3,1) → over polygons
     *
     * Pen 0 is skipped in draw_rect (set_transparent_pen). GL upload still
     * RGB-black-punches residual zeros for copybitmap_trans key 0.
     */
    if (pass & SYS24_PASS_BOTTOM) {
        for (layer = 3; layer >= 2; layer--)
            draw_common_rgb32(st, bitmap, layer << 1, 0, pens);
        for (layer = 1; layer >= 0; layer--)
            draw_common_rgb32(st, bitmap, layer << 1, 0, pens);
    }
    if (pass & SYS24_PASS_PRIORITY) {
        for (layer = 3; layer >= 0; layer--)
            draw_common_rgb32(st, bitmap, (layer << 1) | 1, 0, pens);
    }
}

void sys24_tile_draw_frame_rgb32(sys24_tile_state_t *st, u32 *bitmap, const u8 *palram,
                                 u32 clear_color)
{
    sys24_tile_draw_layers_rgb32(st, bitmap, palram, clear_color, SYS24_PASS_ALL);
}
