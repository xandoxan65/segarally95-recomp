/* The course section selection of the race frame (include/race/course_view.h): polygon budget, the view bind
 * with the in-front/behind flag and the section origin, and the course catalogs of the section. Every RAM write
 * and TGP command is the original's; GEO writes go to race/course_hooks.h. */
#include "race/course_view.h"
#include "race/course.h"
#include "race/course_hooks.h"
#include "race/fp.h"
#include "race/sound_cmd.h"
#include "sr_mem.h"
#include "sr_tgp.h"

#define CATALOG_BASE   0x2864b40u /* main data: 16-byte GEO catalog entries */
#define PLACED         0x20b940u  /* polygons queued this frame */
#define PARAM_CURSOR   0x20a290u  /* TGP parameter list cursor */
#define COURSE_LISTS   0x5dcac8u  /* per course (stride 12): course list head */
#define COURSE_SPANS   0x5dca00u  /* per course (stride 12): catalog span record */

static void bank_poke(void) { sr_hook_course_bank(sr_ld32(0x202278u) & 3u); }

static void placed_add(uint32_t n) { sr_st32(PLACED, sr_ld32(PLACED) + n); }

/* A catalog from main data: bank poke, its 4 words, polygons += word 3. The original adds before or after the
 * FIFO writes depending on the site; GEO and RAM do not interact, so the order is kept only for RAM. */
static void catalog_emit(uint32_t va) {
    const uint32_t w0 = sr_ld32(va), w1 = sr_ld32(va + 4), w2 = sr_ld32(va + 8), w3 = sr_ld32(va + 12);
    bank_poke();
    placed_add(w3);
    sr_hook_course_catalog(w0, w1, w2, w3);
}

/* Matrix bind: the GEO write pointer into the next parameter slot, TGP 0x05 (current matrix to GEO). */
static uint32_t matrix_bind_slot(void) {
    const uint32_t slot = sr_ld32(PARAM_CURSOR), wr = sr_hook_course_write_ptr();
    sr_st32(slot, wr);
    sr_tgp_cmd(0x02800505u, 0, 0, 0, 0);
    sr_hook_course_reg(0x801008u, wr + 0x34u);
    return slot;
}

/* A transform slot's three words (0x2172a0 / 0x217320 / 0x2173a0 + 4*slot) and 0. */
static void transform_slot_emit(uint32_t slot) {
    const uint32_t off = slot << 2;
    const uint32_t w0 = sr_ld32(0x2172a0u + off), w1 = sr_ld32(0x217320u + off), w2 = sr_ld32(0x2173a0u + off);
    bank_poke();
    sr_hook_course_catalog(w0, w1, w2, 0);
}

/* ---- 0x1C450 prg_budget ---- */

static int32_t ld_s16(uint32_t va) { return (int16_t)sr_ld16(va); }

void sr_race_prg_budget(void) {
    const uint32_t zoom = sr_ld32(0x2139d8u);
    const uint32_t row = ((uint32_t)((int32_t)sr_ld32(0x2140c8u) / 10)) << 1;
    const uint32_t r5 = sr_ld32(0x213970u);
    uint32_t tab, near_n = 0, far_n = zoom != 0u ? 1u : 0u, wf, wn, total, cap;
    int32_t base, extra;

    tab = sr_ld32((sr_ld32(0x2020a4u) == 0u ? 0x5dc9a0u : 0x5dc9c0u) + (sr_race_course_index() << 2));
    base = ld_s16(tab + row);
    extra = zoom == 0u ? ld_s16(sr_ld32(0x5dc9e0u + (sr_race_course_index() << 2)) + row) : 0;

    /* Depth list 0x2139f0 (16-byte entries: kind, state, object, depth): state 0 beyond 400, 2 nearer than
     * 0x213970 counted in near_n, else 1 counted in far_n. Kind 3 counts double without zoom. */
    if(sr_ld32(0x2139f8u) != 0u) {
        uint32_t e = 0x2139f0u;
        for(;;) {
            if(sr_ld32(e + 4) != 0u) {
                const uint32_t add = (zoom == 0u && sr_ld32(e) == 3u) ? 2u : 1u;
                const uint32_t depth = sr_ld32(e + 12);
                if(D(depth) > 400.0) {
                    sr_st32(e + 4, 0);
                } else if(D(sr_ld32(e + 12)) > D(r5)) {
                    sr_st32(e + 4, 2);
                    near_n += add;
                } else {
                    far_n += add;
                    sr_st32(e + 4, 1);
                }
            }
            e += 16;
            if(sr_ld32(e + 8) == 0u) break;
        }
    }

    if(sr_ld32(0x202230u) == 0u) {
        wf = far_n == 0u ? 0u : (zoom != 0u ? 0x2eeu : 0x177u) + far_n * 0x177u - 0x177u;
        wn = (near_n * 5u) << 4;
    } else {
        wf = far_n * 0x2eeu;
        wn = (near_n * 5u) << 5;
    }
    total = wn + (wf + (uint32_t)(base + extra));
    sr_st32(0x20aac8u, near_n);
    sr_st32(0x214348u, (uint32_t)extra);
    cap = 0xed8u; /* 3800 */
    if(sr_ld32(0x202230u) == 0u && zoom != 0u && sr_race_course_index() == 0u) {
        const int32_t c4 = (int32_t)sr_ld32(0x2140c4u);
        if(c4 > -5 && c4 <= 9) cap = 0xaf0u; /* 2800 */
    }
    if(!(total < cap)) {
        /* Over budget: keep at most 1 (zoom) or 2 far objects, demote the rest to state 2. */
        const uint32_t keep_max = zoom != 0u ? 1u : 2u;
        uint32_t keep;
        total -= wf;
        keep = (int32_t)far_n >= (int32_t)keep_max ? keep_max : far_n;
        far_n = keep;
        if(sr_ld32(0x2139f8u) != 0u) {
            uint32_t st = 0x2139f4u, next = 0x2139f8u;
            for(;;) {
                if(sr_ld32(st) == 1u) {
                    if(keep == 0u) sr_st32(st, 2);
                    else keep--;
                }
                next += 16;
                st += 16;
                if(sr_ld32(next) == 0u) break;
            }
        }
        total += far_n * 0x2eeu;
        if(!(total <= cap)) base -= (int32_t)(total - cap);
    }
    sr_st32(0x20aac4u, far_n);
    sr_st32(0x214344u, (uint32_t)base);
}

/* ---- race frame: budget accumulate and the view bind ---- */

uint32_t sr_course_budget_accumulate(void) {
    uint32_t acc = sr_ld32(0x214344u) + sr_ld32(PLACED);
    if(sr_ld32(0x2139d8u) != 0u && acc > 0xbb8u) acc = 0xbb8u;
    sr_st32(0x214344u, acc);
    return acc;
}

void sr_course_view_frame(uint32_t course) {
    const uint32_t acc = sr_course_budget_accumulate();
    sr_course_view_bind(course, sr_ld32(0x2140c8u), acc);
}

/* ---- 0x3BE30 view bind ---- */

void sr_course_view_bind(uint32_t course, uint32_t pos, uint32_t budget) {
    const uint32_t list3 = course * 3u;
    const uint32_t list = sr_ld32(COURSE_LISTS + (list3 << 2));
    uint32_t dir_va, m[12], in[12], p[3], i, slot, wr;
    int32_t rec;
    float side;

    if(list == 0u) return;
    dir_va = sr_ld32(list + 0xc);
    if((int32_t)pos < 0) {
        rec = 0;
    } else {
        rec = (int16_t)sr_ld16(sr_ld32(list + 8) + ((pos * 3u) << 2));
        dir_va += pos * 8u;
    }

    sr_tgp_cmd(0x10002020u, 0, 0, 0, 0); /* push */
    {
        const uint32_t x = sr_ld32(dir_va), z = sr_ld32(dir_va + 4);
        sr_st32(0x214360u, x);
        sr_st32(0x214364u, 0);
        sr_st32(0x214368u, z);
        /* Rotation of the current matrix with a zero translation: is the section's direction in front? */
        sr_tgp_cmd(0x13002626u, 0, 0, m, 12);
        for(i = 0; i < 9; i++) in[i] = F(D(m[i]));
        in[9] = in[10] = in[11] = 0;
        sr_tgp_cmd(0x11002222u, in, 12, 0, 0);
        in[0] = x;
        in[1] = 0;
        in[2] = z;
        sr_tgp_cmd(0x16002c2cu, in, 3, p, 3);
        side = sr_f(p[2]);
    }
    sr_tgp_cmd(0x10802121u, 0, 0, 0, 0); /* pop */
    sr_st32(0x2020a4u, side <= 0.0f ? 1u : 0u);

    sr_hook_course_reg(0x800160u, 0);
    sr_hook_course_prg(0x48c35000u); /* far Z 400000 */

    /* The reference stores the write pointer here only into a work RAM slot; the list is in buffer RAM. */
    slot = sr_ld32(PARAM_CURSOR);
    wr = sr_hook_course_write_ptr();
    if(slot != 0u && (slot & 3u) == 0u && slot - SR_WORK_BASE < SR_WORK_SIZE) sr_st32(slot, wr);
    sr_tgp_cmd(0x02800505u, 0, 0, 0, 0);
    sr_st32(PARAM_CURSOR, slot + 4u);
    sr_hook_course_reg(0x801008u, wr + 0x34u);

    sr_course_prg_mode(course, (uint32_t)rec);
    sr_course_catalog_span(COURSE_SPANS + (list3 << 2), (uint32_t)rec, side <= 0.0f ? 0xffffffffu : 1u, budget, 1u);
}

/* ---- 0x3B9B0 extra catalogs by course and section ---- */

void sr_course_prg_mode(uint32_t course, uint32_t span) {
    switch(course) {
    case 0:
    case 4:
        if(!(span > 25u && span <= 55u)) catalog_emit(0x28689a0u);
        if(span > 36u || span <= 7u) {
            bank_poke();
            sr_hook_course_catalog(0x000c2858u, sr_ld32(0x217340u), 0x00921862u, 0x0000006cu);
        }
        return;
    case 2:
        if(sr_ld32(0x2020a4u) == 0u) {
            if(span == 22u) catalog_emit(0x2869b30u);
            if(span == 23u) catalog_emit(0x2869b40u);
        }
        if(span == 23u) {
            const uint32_t w0 = sr_ld32(0x2172c8u), w1 = sr_ld32(0x217348u), w2 = sr_ld32(0x2173c8u);
            bank_poke();
            sr_hook_course_catalog(w0, w1, w2, 0);
        }
        return;
    case 3:
        if(span > 36u && span <= 55u) catalog_emit(0x286a540u);
        return;
    default:
        return;
    }
}

/* ---- 0x3B7C0 the section's catalog run ---- */

void sr_course_catalog_span(uint32_t rec_va, uint32_t span_a, uint32_t span_b, uint32_t budget, uint32_t flag) {
    const uint32_t rec = sr_ld32(rec_va);
    uint32_t cursor = rec + 8u, idx_ptr = rec + 4u, cat, wr_slot, wr, list_base, list_len, ix, start, remain;
    int32_t diff;
    const uint32_t course = sr_race_course_index();

    /* Head catalog: the first threshold at or above the section. */
    for(;;) {
        const uint32_t threshold = sr_ld32(cursor);
        cat = sr_ld32(idx_ptr);
        idx_ptr += 8u;
        if((int32_t)span_a <= (int32_t)threshold) break;
        cursor += 8u;
    }
    catalog_emit(CATALOG_BASE + (cat << 4));
    if((course == 0u || course == 4u) && flag != 0u) sr_course_marker_burst();

    wr_slot = sr_ld32(PARAM_CURSOR);
    wr = sr_hook_course_write_ptr();
    sr_st32(PARAM_CURSOR, wr_slot + 4u);
    sr_st32(wr_slot, wr);
    list_base = sr_ld32(rec_va + 4);
    sr_tgp_cmd(0x02800505u, 0, 0, 0, 0);
    sr_hook_course_reg(0x801008u, wr + 0x34u);

    diff = (int32_t)span_a - (int32_t)span_b;
    list_len = sr_ld32(list_base);
    if(diff < 0) ix = list_len - 1u;
    else if((uint32_t)diff < list_len) ix = (uint32_t)diff;
    else ix = 0u;
    start = ix;
    sr_hook_course_reg(0x800160u, 0);
    remain = budget - sr_ld32(PLACED);
    sr_hook_course_prg(sr_ld32(0x5da1f0u));
    if((int32_t)remain > 0) {
        for(;;) {
            const uint32_t entry = sr_ld32(sr_ld32(rec_va + 8) + (ix << 2));
            const uint32_t va = CATALOG_BASE + (entry << 4);
            const uint32_t w0 = sr_ld32(va), w1 = sr_ld32(va + 4), w2 = sr_ld32(va + 8), w3 = sr_ld32(va + 12);
            const uint32_t take = (remain == 0u || w3 < remain) ? w3 : remain;
            uint32_t next;
            bank_poke();
            sr_hook_course_catalog(w0, w1, w2, w3);
            placed_add(take);
            next = ix + span_b;
            if((int32_t)next < 0) next = list_len - 1u;
            else if(next >= list_len) next = 0u;
            ix = next;
            if(ix == start) break;
            remain = budget - sr_ld32(PLACED);
            if((int32_t)remain <= 0) break;
        }
    }
    sr_course_span_table(course, span_b, list_len, start, ix);
}

/* ---- 0x32410 two marker models (desert): own matrices pushed, bound and emitted ---- */

void sr_course_marker_burst(void) {
    static const uint32_t scale[3] = {0x42200000u, 0x3f800000u, 0x42200000u};
    uint32_t slot;
    {
        static const uint32_t t[3] = {0x43a34937u, 0xc23a3021u, 0x43bd05e3u};
        const uint32_t rz = 0x3dee4baeu;
        sr_tgp_cmd(0x10002020u, 0, 0, 0, 0);
        sr_tgp_cmd(0x13802727u, t, 3, 0, 0);
        sr_tgp_cmd(0x15802b2bu, &rz, 1, 0, 0);
        sr_tgp_cmd(0x14002828u, scale, 3, 0, 0);
    }
    slot = matrix_bind_slot();
    transform_slot_emit(0);
    sr_tgp_cmd(0x10802121u, 0, 0, 0, 0);
    sr_tgp_cmd(0x10002020u, 0, 0, 0, 0);
    {
        /* The original stores the advanced cursor in the middle of the translate's arguments. */
        static const uint32_t t[3] = {0x42931062u, 0xc1458937u, 0xc1c41687u};
        sr_st32(PARAM_CURSOR, slot + 4u);
        sr_tgp_cmd(0x13802727u, t, 3, 0, 0);
        sr_tgp_cmd(0x14002828u, scale, 3, 0, 0);
    }
    slot = matrix_bind_slot();
    transform_slot_emit(0);
    sr_tgp_cmd(0x10802121u, 0, 0, 0, 0);
    sr_st32(PARAM_CURSOR, slot + 4u);
}

/* ---- 0x47920 the section table's extra pieces ---- */

void sr_course_span_table(uint32_t course, uint32_t span_b, uint32_t list_len, uint32_t start_ix, uint32_t ix) {
    const int32_t r7 = (int32_t)list_len;
    int32_t r6 = (int32_t)start_ix, r5 = (int32_t)ix;
    uint32_t rec;
    int32_t a, b;
    if((int32_t)span_b < 0) {
        r6 = (int32_t)ix;
        r5 = (int32_t)start_ix;
    }
    rec = sr_ld32(0x5e66d0u + (course << 2));
    sr_text_cursor(1, 20);
    a = (int32_t)sr_ld32(rec);
    b = (int32_t)sr_ld32(rec + 4);
    /* Rows (a, b, slot) until a < 0: emit the slot when its span range [a, b] meets the drawn range. */
    if(r6 > r5) {
        while(a >= 0) {
            const uint32_t slot = sr_ld32(rec + 8);
            int emit;
            if(!(r6 > a) && r7 > a) emit = 1;
            else if(!(r6 > b) && r7 > b) emit = 1;
            else if(!(0 > a) && r5 >= a) emit = 1;
            else emit = !(0 > b) && !(r5 < b);
            if(emit) transform_slot_emit(slot);
            rec += 12u;
            a = (int32_t)sr_ld32(rec);
            b = (int32_t)sr_ld32(rec + 4);
        }
    } else {
        while(a >= 0) {
            const uint32_t slot = sr_ld32(rec + 8);
            int emit;
            if(!(r6 > a) && r5 >= a) emit = 1;
            else emit = !(r6 > b) && !(r5 < b);
            if(emit) transform_slot_emit(slot);
            rec += 12u;
            a = (int32_t)sr_ld32(rec);
            b = (int32_t)sr_ld32(rec + 4);
        }
    }
}
