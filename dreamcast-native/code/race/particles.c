/* The cars' object pens and their dust and debris particles: see include/race/particles.h.
 *
 * Float rules as everywhere in the gameplay (race/fp.h): the lifted code computes in double and rounds once per
 * store, so chains of several operations stay in double (D/F). Catalog entries (main data 0x2864b40 + 16 * ix:
 * tpa, tha, oba, polygon count) are read from main data; pen tables (0x5c2780.., 0x5ddb70..) through the work RAM
 * ROM mirror. */
#include "race/particles.h"
#include "race/course_hooks.h"
#include "race/desert_hooks.h"
#include "race/fp.h"
#include "sr_mem.h"
#include "sr_tgp.h"
#include "model2_tgp.h"

#define CATALOG      0x2864b40u
#define PLACED       0x20b940u /* polygons queued this frame */
#define PARAM_CURSOR 0x20a290u /* TGP parameter list cursor */

#define POOL     0x20cd90u /* 36-byte particle records */
#define LIVE     0x20d600u /* live record indices */
#define FREE     0x20d6f0u /* free record indices */
#define N_LIVE   0x20d7e0u
#define N_FREE   0x20d7e4u
#define RNG_SEED 0x213864u

#define TGP_PUSH 0x10002020u
#define TGP_POP  0x10802121u
#define TGP_IDENTITY 0x12802525u
#define TGP_TRANSLATE 0x13802727u
#define TGP_ROT_X 0x14802929u
#define TGP_ROT_Y 0x15002a2au
#define TGP_ROT_Z 0x15802b2bu
#define TGP_XFORM 0x16002c2cu /* point through the current matrix */
#define TGP_EMIT  0x02800505u /* current matrix to the GEO */

static void tgp0(uint32_t cmd) { sr_tgp_cmd(cmd, 0, 0, 0, 0); }
static void tgp1(uint32_t cmd, uint32_t a) { sr_tgp_cmd(cmd, &a, 1, 0, 0); }
static void tgp3(uint32_t cmd, uint32_t a, uint32_t b, uint32_t c) {
    const uint32_t in[3] = {a, b, c};
    sr_tgp_cmd(cmd, in, 3, 0, 0);
}
static void tgp_xform(uint32_t a, uint32_t b, uint32_t c, uint32_t out[3]) {
    const uint32_t in[3] = {a, b, c};
    sr_tgp_cmd(TGP_XFORM, in, 3, out, 3);
}

/* model2_workram_mirror_*: the program ROM behind work RAM, without the RAM overlay. */
static uint32_t mirror32(uint32_t va) { return sr_rom_mirror32(va); }
static uint32_t mirror16(uint32_t va) {
    const uint8_t *p = sr_rom_mirror(va);
    return p ? (uint32_t)p[0] | (uint32_t)p[1] << 8 : 0;
}

static uint32_t catalog_word(uint32_t ix, uint32_t off) { return sr_ld32(CATALOG + (ix << 4) + off); }

static void placed_add(uint32_t n) { sr_st32(PLACED, sr_ld32(PLACED) + n); }

/* A catalog entry to the GEO program FIFO after the bank poke (`bank_word`: the link register the original
 * leaves there, or 0); `count` stands in for its word 3. */
static void catalog_emit(uint32_t ix, uint32_t bank_word, uint32_t count) {
    sr_hook_geo_bank_word(bank_word);
    sr_hook_course_catalog(catalog_word(ix, 0), catalog_word(ix, 4), catalog_word(ix, 8), count);
}

/* The GEO write pointer into the next parameter slot, the current matrix to the GEO, the object's data start. */
static void slot_bind(void) {
    const uint32_t slot = sr_ld32(PARAM_CURSOR), wr = sr_hook_geo_list_ptr();
    sr_st32(slot, wr);
    tgp0(TGP_EMIT);
    sr_st32(PARAM_CURSOR, slot + 4u);
    sr_hook_geo_object_start(wr);
}

/* ---- frame bookkeeping ---- */

void sr_geo_frame_begin(void) {
    sr_st32(0x202270u, 0x43af0000u);
    sr_st32(0x202274u, 0x43af0000u);
    sr_st32(PLACED, 0);
}

void sr_geo_frame_end(void) {
    sr_st32(0x214374u, 0);
    sr_st32(0x214370u, 0x800100u);
}

/* ---- 0x33DC8 geo_rng_next ---- */

uint32_t sr_particle_rng(void) {
    const uint32_t seed = sr_ld32(RNG_SEED) * 0x41c64e6du + 0x3039u;
    sr_st32(RNG_SEED, seed);
    return (seed >> 16) % 0x7fffu;
}

/* ---- 0x317B0 geo_attract_object_extra ---- */

static uint32_t rec_va(uint32_t slot) { return POOL + ((slot + (slot << 3)) << 2); }

/* frame[0..5]: the two TGP points of the spawn (0x40 / 0x50 of the original's frame). */
static void fill_record(uint32_t slot, uint32_t nibble, uint32_t r5, const uint32_t frame[6], uint32_t obj, int moving,
                        double k) {
    const uint32_t rec = rec_va(slot);
    uint32_t w, half_dx, q;
    sr_st8(rec, (uint8_t)((sr_ld8(rec) & 0xc0u) | ((sr_ld8(obj + 0x50) >> 5) & 3u) | (moving ? 4u : 0u)));
    sr_st16(rec, (uint16_t)((sr_ld16(rec) & 0xfc3fu) | (nibble << 6)));
    half_dx = F(D(r5) * k);
    if(!moving) {
        w = sr_ld32(rec);
        sr_st32(rec, (w & 0xfffc03ffu) | (15u << 11));
        sr_st32(rec + 0x04, frame[0]);
        sr_st32(rec + 0x08, frame[1]);
        sr_st32(rec + 0x0c, frame[2]);
        sr_st32(rec + 0x10, 0);
        sr_st32(rec + 0x14, half_dx);
        sr_st32(rec + 0x18, 0);
    } else {
        sr_st32(rec + 0x04, frame[0]);
        sr_st32(rec + 0x08, frame[1]);
        sr_st32(rec + 0x0c, frame[2]);
        sr_st32(rec + 0x10, F(D(frame[3]) - D(frame[0])));
        sr_st32(rec + 0x14, half_dx);
        sr_st32(rec + 0x18, F(D(frame[5]) - D(frame[2])));
        sr_st32(rec + 0x20, 0);
    }
    q = F((double)(int32_t)((sr_particle_rng() & 3u) + 1u));
    sr_st32(rec + 0x1c, F(D(r5) * D(q)));
}

void sr_particle_spawn(uint32_t obj) {
    const double half = sr_d(0u, 0x3fe00000u);
    uint32_t v, depth2, f60, f64, cnt, lim, tbl, nibbles, bit;
    {
        const uint32_t in[6] = {0, 0, 0, sr_ld32(obj + 0x0c), sr_ld32(obj + 0x10), sr_ld32(obj + 0x14)};
        sr_tgp_cmd(0x2c005858u, in, 6, &v, 1); /* distance from the eye */
    }
    depth2 = F(D(v) + D(v));
    if(D(depth2) <= 0.0) return;
    if(sr_ld8(0x20d7fcu) == 0) return;

    /* The spot below the car in the eye's yaw frame: its x (scaled 0.6) for the dust's sideways spread. */
    tgp0(TGP_PUSH);
    tgp0(TGP_IDENTITY);
    tgp1(TGP_ROT_Y, fneg(sr_ld32(obj + 0x1c)));
    {
        uint32_t out[3];
        tgp_xform(sr_ld32(obj + 0x0c), 0, sr_ld32(obj + 0x14), out);
        f60 = F(D(out[0]) * sr_d(0x33333333u, 0x3fe33333u));
        f64 = fabs_bits(f60);
    }
    tgp0(TGP_POP);

    if((sr_ld8(obj + 0x50) & 0x60u) == 0) cnt = 0x20d7e8u, lim = 0x20d7f0u;
    else cnt = 0x20d7ecu, lim = 0x20d7f4u;
    v = sr_ld8(obj + 0x50) & 15u;
    tbl = (v == 1u || v == 4u) ? 0x5d0690u : 0x5d0660u;

    tgp0(TGP_PUSH);
    tgp0(TGP_IDENTITY);
    tgp3(TGP_TRANSLATE, sr_ld32(obj), sr_ld32(obj + 4), sr_ld32(obj + 8));
    tgp1(TGP_ROT_Y, sr_ld32(obj + 0x1c));
    tgp1(TGP_ROT_X, sr_ld32(obj + 0x18));
    tgp1(TGP_ROT_Z, sr_ld32(obj + 0x20));

    nibbles = sr_ld16(obj + 0x52);
    for(bit = 0; bit <= 3u; bit++, nibbles >>= 4) {
        const uint32_t nibble = nibbles & 15u, scale = sr_ld32(obj + 0x30 + 4u * bit), jt = nibble - 2u;
        const double scale_raw = D(scale), scale_half = (bit & 1u) ? scale_raw * half : -(scale_raw * half);
        uint32_t half_sc, free_ix, slot, used, d[3], t[3], frame[6], r5;
        if(!((sr_ld8(0x20d7fcu) >> bit) & 1u)) continue;
        if((int32_t)sr_ld32(N_LIVE) >= (int32_t)sr_ld32(0x20d800u)) continue;
        if(scale_raw <= 0.0) continue;
        if(jt >= 5u) continue;
        half_sc = F(scale_half);
        if((int32_t)sr_ld32(cnt) >= (int32_t)sr_ld32(lim)) continue;
        sr_st32(cnt, sr_ld32(cnt) + 1u);
        free_ix = sr_ld32(N_FREE) - 1u;
        sr_st32(N_FREE, free_ix);
        slot = sr_ld32(FREE + (free_ix << 2));
        used = sr_ld32(N_LIVE);
        sr_st32(N_LIVE, used + 1u);
        sr_st32(LIVE + (used << 2), slot);

        tgp0(TGP_PUSH);
        tgp3(TGP_TRANSLATE, sr_ld32(tbl + bit * 12u), F(D(sr_ld32(tbl + bit * 12u + 4)) + D(scale)),
             sr_ld32(tbl + bit * 12u + 8));
        if(jt == 2u || jt == 3u) {
            /* Debris: thrown from the wheel, its start and a second point give the velocity. */
            const uint32_t y_r13 = F(D(depth2) * D(scale));
            tgp1(TGP_ROT_Y, sr_ld32(obj + 0x24));
            tgp_xform(0, 0, 0, d);
            tgp_xform(F(D(f60) + D(half_sc)), F(D(f64) + D(y_r13)), 0, t);
            frame[0] = d[0], frame[1] = d[1], frame[2] = d[2], frame[3] = t[0], frame[4] = t[1], frame[5] = t[2];
            r5 = F(D(t[1]) - D(d[1]));
            if(D(r5) < 0.0) r5 = fneg(r5);
            fill_record(slot, nibble, r5, frame, obj, 1, half);
        } else {
            /* Dust: a puff at the wheel, its height from the eye distance. */
            tgp_xform(0, 0, 0, d);
            tgp_xform(0, F(D(f64) + D(depth2) * sr_d(0x9999999au, 0x3fb99999u)), 0, t);
            frame[0] = d[0], frame[1] = d[1], frame[2] = d[2], frame[3] = t[0], frame[4] = t[1], frame[5] = t[2];
            r5 = F(D(t[1]) - D(d[1]));
            if(D(r5) < 0.0) r5 = fneg(r5);
            fill_record(slot, nibble, r5, frame, obj, 0, sr_d(0x33333333u, 0x3fd33333u));
        }
        tgp0(TGP_POP);
    }
    tgp0(TGP_POP);
}

/* ---- 0x23A80 glyph_emit: the near car (body, cabin pen, wheels), then its particles ---- */

void sr_glyph_emit(uint32_t pen, uint32_t obj, uint32_t budget) {
    const uint32_t flag = mirror16(pen + 0x52u) & 0x8888u, pen_hdr = mirror32(pen);
    const uint32_t x0 = sr_ld32(obj + 0x40), y0 = sr_ld32(obj + 0x44), z0 = sr_ld32(obj + 0x48), x1 = sr_ld32(obj + 0x4c);
    uint32_t base_ix, sum, scaled, dx, dy, remain, ix, w3, take;
    if(!sr_at(obj) || pen == 0) return;
    base_ix = mirror32(pen + (flag != 0 ? 0x10u : 0x0cu));
    if(mirror32(pen + 0x14u) != 0) base_ix += sr_ld8(obj + 0x51) & 3u;

    /* The body: suspension mean height and its pitch / roll from the four wheels. */
    sum = F(D(x0) + D(y0) + D(z0) + D(x1));
    scaled = F(D(sum) * sr_d(0u, 0x3fd00000u) - sr_d(0x47ae147bu, 0x3f947ae1u));
    tgp0(TGP_PUSH);
    tgp3(TGP_TRANSLATE, sr_ld32(obj), scaled, sr_ld32(obj + 8));
    tgp1(TGP_ROT_Y, sr_ld32(obj + 0x1c));
    dx = F((D(x0) + D(y0)) - (D(z0) + D(x1)));
    dx = F(D(dx) * sr_d(0xb4395810u, 0x3fce76c8u));
    tgp1(TGP_ROT_X, dx);
    dy = F((D(x0) + D(z0)) - (D(y0) + D(x1)));
    dy = F(D(dy) * sr_d(0x3b645a1du, 0x3fd54fdfu));
    tgp1(TGP_ROT_Z, dy);
    slot_bind();

    /* The chassis at the pose itself; the polygon count lands between yaw and pitch, as in the original. */
    ix = mirror32(pen + 4u);
    w3 = catalog_word(ix, 12);
    catalog_emit(ix, 0, w3);
    tgp0(TGP_POP);
    tgp0(TGP_PUSH);
    tgp3(TGP_TRANSLATE, sr_ld32(obj), sr_ld32(obj + 4), sr_ld32(obj + 8));
    tgp1(TGP_ROT_Y, sr_ld32(obj + 0x1c));
    placed_add(w3);
    tgp1(TGP_ROT_X, sr_ld32(obj + 0x18));
    tgp1(TGP_ROT_Z, sr_ld32(obj + 0x20));
    slot_bind();

    /* The body catalog, its count clamped to the budget left. */
    remain = budget - sr_ld32(PLACED);
    if((int32_t)remain < 1) remain = 1u;
    w3 = catalog_word(base_ix, 12);
    take = (remain != 0u && w3 >= remain) ? remain : w3;
    catalog_emit(base_ix, 0, take);
    placed_add(take);

    if(flag != 0) sr_hook_pen_push_a(sr_ld8(obj + 0x50) & 15u);
    else sr_hook_pen_push(obj, obj + 0x18u, 0x20220cu, sr_ld8(obj + 0x50) & 15u);

    if(sr_ld8(obj + 0x50) & 0x10u) {
        const uint32_t extra = mirror32(pen + 0x14u);
        if(extra != 0) {
            w3 = catalog_word(extra, 12);
            catalog_emit(extra, 0, w3);
            placed_add(w3);
        }
    }

    /* The four wheels: offsets from the pen header, suspension from obj+0x30.., steering on the front two. */
    remain = budget - sr_ld32(PLACED);
    if((int32_t)remain >= 1) {
        const uint32_t spin = fneg(sr_ld32(obj + 0x2c));
        uint32_t i, row = pen_hdr;
        for(i = 0; i < 4u; i++, row += 12u) {
            tgp0(TGP_PUSH);
            tgp3(TGP_TRANSLATE, sr_ld32(row), F(D(sr_ld32(row + 4)) + D(sr_ld32(obj + 0x30 + 4u * i))),
                 sr_ld32(row + 8));
            if(i <= 1u) tgp1(TGP_ROT_Y, sr_ld32(obj + 0x24));
            tgp1(TGP_ROT_X, spin);
            slot_bind();
            ix = mirror32(pen + 8u);
            w3 = catalog_word(ix, 12);
            catalog_emit(ix, 0, w3);
            tgp0(TGP_POP);
            placed_add(w3);
            remain = budget - sr_ld32(PLACED);
            if((int32_t)remain < 1) break;
        }
    }

    tgp0(TGP_POP);
    remain = budget - sr_ld32(PLACED);
    if((int32_t)remain > 5) sr_particle_spawn(obj);
}

/* ---- 0x24560 pen_far: the far car (pen_desc, scale over 100) ---- */

void sr_object_pen_far(uint32_t pen, uint32_t obj, uint32_t budget) {
    const uint32_t link = 0x24960u; /* the call's return address, which the bank pokes store */
    uint32_t pen_hdr, half, remain, ix, w3, take, bank, i;
    if(!sr_at(obj) || pen == 0u || obj == 0u) return;
    pen_hdr = mirror32(pen);
    if(pen_hdr == 0u) return;

    tgp0(TGP_PUSH);
    tgp3(TGP_TRANSLATE, sr_ld32(obj), sr_ld32(obj + 4), sr_ld32(obj + 8));
    tgp1(TGP_ROT_Y, sr_ld32(obj + 0x1c));
    tgp1(TGP_ROT_X, sr_ld32(obj + 0x18));
    tgp1(TGP_ROT_Z, sr_ld32(obj + 0x20));
    slot_bind();

    half = F((D(sr_ld32(obj + 0x30)) + D(sr_ld32(obj + 0x34)) + D(sr_ld32(obj + 0x38)) + D(sr_ld32(obj + 0x3c))) *
             sr_d(0u, 0x3fd00000u));
    remain = budget - sr_ld32(PLACED);
    if((int32_t)remain < 1) remain = 1u;
    ix = mirror32(pen + 0xcu);
    w3 = catalog_word(ix, 12);
    take = (remain != 0u && w3 >= remain) ? remain : w3;
    catalog_emit(ix, link, w3); /* the original word 3 goes to the GEO, the clamped one to the count */
    bank = sr_ld32(PLACED);
    tgp3(TGP_TRANSLATE, mirror32(pen_hdr), F(D(mirror32(pen_hdr + 4u)) + D(half)), mirror32(pen_hdr + 8u));
    sr_st32(PLACED, bank + take);
    slot_bind();

    ix = mirror32(pen + 8u);
    for(i = 0; i < 4u; i++) {
        bank = sr_ld32(PLACED);
        w3 = catalog_word(ix, 12);
        catalog_emit(ix, link, w3);
        sr_st32(PLACED, bank + w3);
        slot_bind();
    }
    tgp0(TGP_POP);
}

/* ---- 0x240C0 pen_mid: the mid-distance car, then its particles ---- */

void sr_object_pen_mid(uint32_t pen, uint32_t obj, uint32_t budget) {
    const uint32_t link = 0x24970u;
    uint32_t pen0, sum, y, diff, pitch, slot, wr, ix, bank, remain, cat3, cursor, ysrc, i;
    if(pen == 0u || obj == 0u) return;
    pen0 = sr_ld32(pen);

    /* The body at the suspension's mean height, pitch from the wheels. */
    tgp0(TGP_PUSH);
    sum = fadd(sr_ld32(obj + 0x40), sr_ld32(obj + 0x44));
    sum = fadd(sum, sr_ld32(obj + 0x48));
    sum = fadd(sum, sr_ld32(obj + 0x4c));
    y = F(D(sum) * sr_d(0u, 0x3fd00000u) - sr_d(0x47ae147bu, 0x3f947ae1u));
    tgp3(TGP_TRANSLATE, sr_ld32(obj), y, sr_ld32(obj + 8));
    tgp1(TGP_ROT_Y, sr_ld32(obj + 0x1c));
    diff = fadd(sr_ld32(obj + 0x40), sr_ld32(obj + 0x44));
    diff = fsub(diff, fadd(sr_ld32(obj + 0x48), sr_ld32(obj + 0x4c)));
    pitch = F(D(diff) * sr_d(0xb4395810u, 0x3fce76c8u));
    tgp1(TGP_ROT_X, pitch);
    tgp1(TGP_ROT_Z, sr_ld32(obj + 0x20));
    slot = sr_ld32(PARAM_CURSOR);
    wr = sr_hook_geo_list_ptr();
    sr_st32(slot, wr);
    tgp0(TGP_EMIT);
    sr_hook_geo_object_start(wr);
    ix = sr_ld32(pen + 4);
    bank = sr_ld32(PLACED);
    sr_st32(PARAM_CURSOR, slot + 4u);
    sr_st32(PLACED, bank + catalog_word(ix, 12));
    catalog_emit(ix, link, catalog_word(ix, 12));
    tgp0(TGP_POP);

    /* The chassis at the pose. */
    tgp0(TGP_PUSH);
    tgp3(TGP_TRANSLATE, sr_ld32(obj), sr_ld32(obj + 4), sr_ld32(obj + 8));
    tgp1(TGP_ROT_Y, sr_ld32(obj + 0x1c));
    tgp1(TGP_ROT_X, sr_ld32(obj + 0x18));
    tgp1(TGP_ROT_Z, sr_ld32(obj + 0x20));
    slot = sr_ld32(PARAM_CURSOR);
    wr = sr_hook_geo_list_ptr();
    sr_st32(slot, wr);
    tgp0(TGP_EMIT);
    bank = sr_ld32(PLACED);
    sr_st32(PARAM_CURSOR, slot + 4u);
    sr_hook_geo_object_start(wr);

    remain = budget - bank;
    if((int32_t)remain < 1) remain = 1u;
    ix = sr_ld32(pen + 0xc);
    cat3 = catalog_word(ix, 12);
    if(remain == 0u) cat3 = ix;
    else if(cat3 >= remain) cat3 = remain;
    bank = sr_ld32(PLACED);
    sr_st32(PLACED, bank + cat3);
    catalog_emit(ix, link, cat3);

    /* The four wheels. */
    cursor = pen0;
    ysrc = obj + 0x30u;
    for(i = 0; i < 4u; i++, cursor += 12u, ysrc += 4u) {
        tgp0(TGP_PUSH);
        tgp3(TGP_TRANSLATE, sr_ld32(cursor), fadd(sr_ld32(cursor + 4), sr_ld32(ysrc)), sr_ld32(cursor + 8));
        if(i <= 1u) tgp1(TGP_ROT_Y, sr_ld32(obj + 0x24));
        slot = sr_ld32(PARAM_CURSOR);
        wr = sr_hook_geo_list_ptr();
        sr_st32(slot, wr);
        tgp0(TGP_EMIT);
        sr_hook_geo_object_start(wr);
        ix = sr_ld32(pen + 8);
        bank = sr_ld32(PLACED);
        catalog_emit(ix, link, catalog_word(ix, 12));
        tgp0(TGP_POP);
        sr_st32(PARAM_CURSOR, slot + 4u);
        sr_st32(PLACED, bank + catalog_word(ix, 12));
    }

    remain = budget - sr_ld32(PLACED);
    tgp0(TGP_POP);
    if((int32_t)remain > 5) sr_particle_spawn(obj);
}

/* ---- 0x248B0 pen_desc, 0x247D0 object_pen, 0x24C50 walk ---- */

void sr_object_pen_desc(uint32_t desc) {
    uint32_t mode, obj, flags, pen;
    if(desc == 0u) return;
    mode = sr_ld32(desc + 4);
    obj = sr_ld32(desc + 8);
    if(mode == 0u || !sr_at(obj)) return;
    flags = sr_ld8(obj + 0x50);
    if(mode == 1u) {
        sr_st8(obj + 0x50, (uint8_t)(flags & 0x9fu));
        sr_glyph_emit(0x5c2780u + (((flags & 15u) * 3u) << 4), obj, 0x270fu);
        return;
    }
    sr_st8(obj + 0x50, (uint8_t)((flags & 0x9fu) | 0x20u));
    pen = 0x5c2798u + (((flags & 15u) * 3u) << 4);
    if(D(sr_ld32(desc + 0xc)) > sr_d(0u, 0x40590000u)) sr_object_pen_far(pen, obj, 0x270fu);
    else sr_object_pen_mid(pen, obj, 0x270fu);
}

void sr_object_pen(uint32_t obj, uint32_t scale, uint32_t budget) {
    uint32_t flags;
    if(obj == 0 || !sr_at(obj)) return;
    if(D(scale) > sr_d(0u, 0x407f4000u)) return; /* 500 */
    if(D(scale) >= D(sr_ld32(0x213970u))) return;
    flags = sr_ld8(obj + 0x50);
    sr_st8(obj + 0x50, (uint8_t)(flags & 0x9fu));
    sr_glyph_emit(0x5c2780u + (((flags & 15u) * 3u) << 4), obj, budget);
}

void sr_object_pen_walk(uint32_t flag_bit) {
    uint32_t node = 0x2139f0u;
    if(sr_ld32(0x2139d8u) != 0u && sr_ld32(0x2139f8u) != 0u && (sr_ld8(0x2139f0u) & 2u)) node += 0x10u;
    for(; sr_ld32(node + 8) != 0u; node += 0x10u) {
        if(sr_ld32(node + 4) == 0u) continue;
        if(flag_bit != 0u ? (sr_ld8(node) & 1u) : (sr_ld8(node) & 2u)) sr_object_pen_desc(node);
    }
}

/* ---- 0x31FC0 copro_submit, 0x32150 copro_object ---- */

static sr_particle_draw_t s_draws[SR_PARTICLE_DRAWS_MAX];
static uint32_t s_n_draws;
static float s_draw_view[12]; /* the TGP's working matrix at the submit: what 0x55 transforms the points by */

uint32_t sr_particles_draws(const sr_particle_draw_t **out) {
    *out = s_draws;
    return s_n_draws;
}

void sr_particles_view(float out[12]) {
    int i;
    for(i = 0; i < 12; i++) out[i] = s_draw_view[i];
}

uint32_t sr_particle_step(uint32_t word0, uint32_t ix) {
    const uint32_t rec = POOL + ((ix + (ix << 3)) << 2);
    if(((word0 >> 2) & 15u) == 0) {
        /* Dust: grows by its rate at +0x14 (the rate slowing by 0.02 a tick, or by 0.002 once negative) until
         * its height +0x1c goes below 0 or its 8-bit life in word 0 (bits 10..17) runs out. */
        uint32_t f, f_sum, packed, life;
        double rate;
        sr_hook_geo_bank_word(0);
        sr_hook_course_catalog(sr_ld32(0x28655c0u), sr_ld32(0x28655c4u), sr_ld32(0x28655c8u), sr_ld32(0x28655ccu));
        rate = D(sr_ld32(rec + 0x14));
        placed_add(sr_ld32(0x28655ccu));
        rate = rate > 0.0 ? rate - sr_d(0x47ae147bu, 0x3f947ae1u) : rate - sr_d(0xd2f1a9fcu, 0x3f60624du);
        f = F(rate);
        f_sum = F(D(f) + D(sr_ld32(rec + 0x1c)));
        sr_st32(rec + 0x14, f);
        sr_st32(rec + 0x1c, f_sum);
        if(D(f_sum) < 0.0) return 1u;
        packed = sr_ld32(rec);
        life = ((packed >> 10) - 1u) & 0xffu;
        sr_st32(rec, (packed & 0xfffc03ffu) | (life << 10));
        return life == 0 ? 1u : 0u;
    }
    {
        /* Debris: moves by its velocity (+0x10, +0x20, +0x18), its vertical speed +0x14 falling by 0.01 a tick,
         * until its height +0x1c goes below 0. */
        uint32_t r5, h, x, z, y;
        sr_hook_course_reg(0x800160u, 0);
        sr_hook_course_prg(0x40800000u);
        sr_hook_geo_bank_word(0);
        sr_hook_course_catalog(sr_ld32(0x28655d0u), sr_ld32(0x28655d4u), sr_ld32(0x28655d8u), sr_ld32(0x28655dcu));
        sr_hook_course_reg(0x800160u, 0);
        sr_hook_course_prg(0x48c35000u);
        r5 = F(D(sr_ld32(rec + 0x14)) - sr_d(0x47ae147bu, 0x3f847ae1u));
        h = F(D(r5) + D(sr_ld32(rec + 0x1c)));
        x = F(D(sr_ld32(rec + 4)) + D(sr_ld32(rec + 0x10)));
        z = F(D(sr_ld32(rec + 0xc)) + D(sr_ld32(rec + 0x18)));
        y = F(D(sr_ld32(rec + 8)) + D(sr_ld32(rec + 0x20)));
        placed_add(sr_ld32(0x28655dcu));
        sr_st32(rec + 4, x);
        sr_st32(rec + 0xc, z);
        sr_st32(rec + 8, y);
        sr_st32(rec + 0x14, r5);
        sr_st32(rec + 0x1c, h);
        return D(h) >= 0.0 ? 0u : 1u;
    }
}

void sr_particles_submit(void) {
    const uint32_t remain = sr_ld32(N_LIVE);
    uint32_t r6, r8;
    int32_t i;
    s_n_draws = 0;
    model2_tgp_matrix(s_draw_view);
    if((int32_t)remain <= 0) return;
    i = (int32_t)remain - 1;
    r6 = LIVE + ((uint32_t)i << 2);
    r8 = r6;
    for(; i >= 0; i--, r6 -= 4u) {
        const uint32_t ix = sr_ld32(r6), rec = POOL + ((ix + (ix << 3)) << 2);
        const uint32_t x = sr_ld32(rec + 4), y = sr_ld32(rec + 8), z = sr_ld32(rec + 0xc), size = sr_ld32(rec + 0x1c);
        const uint32_t slot = sr_ld32(PARAM_CURSOR), wr = sr_hook_geo_list_ptr();
        sr_st32(slot, wr);
        {
            const uint32_t in[10] = {x, y, z, sr_ld32(0x20220cu), x, sr_ld32(0x202214u), z, 0x3fc00000u, size,
                                     0x3f800000u};
            sr_tgp_cmd(0x2a805555u, in, 10, 0, 0);
        }
        sr_hook_geo_object_start(wr);
        sr_st32(PARAM_CURSOR, slot + 4u);
        {
            const uint32_t word0 = sr_ld32(rec), kind = ((word0 >> 2) & 15u) != 0;
            if(s_n_draws < SR_PARTICLE_DRAWS_MAX) {
                sr_particle_draw_t *d = &s_draws[s_n_draws++];
                d->pos[0] = sr_f(x), d->pos[1] = sr_f(y), d->pos[2] = sr_f(z);
                d->size = sr_f(size);
                d->oba = kind ? 0x82c96cu : 0x82c95bu;
                d->kind = kind;
            }
            if(sr_particle_step(word0, ix) == 0) continue;
        }
        /* Retired: count it off its kind, move the last pending index into its place, free the record. */
        {
            const uint32_t tag = sr_ld8(rec) & 3u, n_free = sr_ld32(N_FREE), n_live = sr_ld32(N_LIVE);
            const uint32_t counter = tag != 0 ? 0x20d7ecu : 0x20d7e8u;
            sr_st32(counter, sr_ld32(counter) - 1u);
            sr_st32(r6, sr_ld32(r8));
            r8 -= 4u;
            sr_st32(FREE + (n_free << 2), ix);
            sr_st32(N_FREE, n_free + 1u);
            sr_st32(N_LIVE, n_live - 1u);
        }
    }
}

/* ---- the race frame's parts ---- */

void sr_race_pens_before_course(void) { sr_object_pen_walk(1); }

void sr_race_pens_after_course(void) {
    sr_particles_submit();
    model2_tgp_restore_latched_view_matrix();
    if(sr_ld32(0x2139d8u) != 0u) {
        /* The followed car in the behind view, and the other one when its pen asks to be drawn on top. */
        sr_hook_course_reg(0x800030u, 0);
        sr_hook_course_prg(0x80u);
        sr_hook_course_prg(0x01f00200u);
        sr_hook_course_prg(0x00f80140u);
        sr_hook_course_prg(0x00f80140u);
        sr_hook_course_prg(0x00f80140u);
        sr_hook_course_prg(0x00f80140u);
        sr_object_pen(sr_ld32(0x213980u), 0x3f800000u, 0x270fu);
        if(sr_ld32(0x2139f8u) != 0u && (sr_ld8(0x2139f0u) & 2u) && (sr_ld8(sr_ld32(0x2139f8u) + 0x50u) & 0x80u))
            sr_object_pen_desc(0x2139f0u);
    }
}
