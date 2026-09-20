/* Semantic C from MAME disasm @ 0x3b200 — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_03b200_5c0.asm */
// @rom 0x3b200 +0x5c0 geo_attract_course_slot_call

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_rom.h"

#include <stdio.h>
#include <string.h>

/*
 * g0 = course slot record *vaddr* (passed as (void *)(uintptr_t)va from carousel).
 * g1 = host pointer to attract object (fp shadow).
 * g2 = span / style word from caller.
 *
 * Loop walks slot candidates, emits copro marker streams, picks nearest index,
 * then emits tail markers + stores selection @ 0x214340 / 0x214360.
 *
 * Copro FIFO @ 0x884000: host implements marker arity + formulas from this
 * disasm (0x2e XZ edge test, 0x2c distance, matrix push/pop) in model2_hw_fifo.c.
 */

static u32 slot_ld_u32(u32 ea)
{
    return i960_ld_u32(I960_WORKRAM, ea, 0);
}

static u16 slot_ld_u16(u32 ea)
{
    return i960_ld_u16(I960_WORKRAM, ea, 0);
}

static void slot_ldl(u32 ea, u32 *lo, u32 *hi)
{
    *lo = slot_ld_u32(ea);
    *hi = slot_ld_u32(ea + 4u);
}

/* lda (ix)[ix*2] → ix*3; lda (base)[that*4] → base + ix*12. */
static u32 slot_scaled12(u32 base, u32 ix)
{
    u32 trip = ix + (ix << 1);

    return base + (trip << 2);
}

void geo_attract_course_slot_call(void * arg0, void * arg1, u32 arg2)
{
    u32 rec_va = (u32)(uintptr_t)arg0;
    u32 *obj = (u32 *)arg1;
    u8 frame[0x110];
    uintptr_t fp_save = fp;
    u32 count;
    u32 tab8;
    u32 base4;
    u32 tab_c;
    u32 cursor;
    u32 ix;
    u32 loop_i;
    u32 best_ix;
    u32 g4, g5, g6, g7, g12;
    u32 g0w, g2w, g3w;
    u32 r4lo, r4hi, r5w, r6w, r8w, r9w, r11w;
    u32 entry;
    u32 rec7;
    double fp2_max;
    double fp3_two;
    double fp0, fp1;

    (void)arg2;
    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;

    if (rec_va == 0) {
        g0 = 0;
        fp = fp_save;
        return;
    }

    /* @0x3B214–0x3B234 */
    count = slot_ld_u32(rec_va);
    tab8 = slot_ld_u32(rec_va + 8u);
    base4 = slot_ld_u32(rec_va + 4u);
    *(u32 *)(frame + 0x90) = tab8;
    cursor = i960_ld_u32(I960_WORKRAM, 0x214340, 0);
    tab_c = slot_ld_u32(rec_va + 0xcu);
    *(u32 *)(frame + 0xa0) = tab_c;
    if (cursor >= count)
        cursor = 0;

    /* @0x3B240: lda 0x7f7fffff → max float for nearest tracking.
     * Disasm stores g14 into fp+0x100/0x80; in this fn g14 is float 0
     * (also used as the 0x2c origin triple). Host link must not leak here. */
    fp2_max = i960_u32_to_f64(0x7f7fffffu);
    *(u32 *)(frame + 0x100) = 0;
    *(u32 *)(frame + 0x80) = 0;
    loop_i = 0;

    /* @0x3B250: cmpi count,0 / be → epilogue. */
    if (count == 0)
        goto epilogue;

    /* @0x3B260–0x3B28C: cursor+count, scratch ptrs, fp3 = 2.0. */
    cursor = cursor + count;
    *(u32 *)(frame + 0xb0) = (u32)(uintptr_t)(frame + 0x70);
    *(u32 *)(frame + 0xc0) = (u32)(uintptr_t)(frame + 0x78);
    *(u32 *)(frame + 0xf0) = cursor;
    fp3_two = i960_rifl_read(0, 0x40100000u);

loop_body:
    /* @0x3B290: bbc 0,r12 — bit0 clear → subtract path. */
    g4 = *(u32 *)(frame + 0xf0);
    if ((loop_i & 1u) != 0) {
        /* @0x3B294–0x3B2A0: lda 0x1(g13)[g4>>1] */
        g4 = g4 + 1u + (loop_i >> 1);
    } else {
        /* @0x3B2A8–0x3B2B0 */
        g4 = g4 - (loop_i >> 1);
    }
    /* @0x3B2B4: remo count, g4 */
    if (count != 0)
        g4 = g4 % count;
    else
        g4 = 0;
    *(u32 *)(frame + 0x100) = g4;

    /* @0x3B2BC–0x3B2C8: record = tab8[ix*12] */
    ix = *(u32 *)(frame + 0x100);
    tab8 = *(u32 *)(frame + 0x90);
    rec7 = slot_scaled12(tab8, ix);

    /* @0x3B2CC–0x3B330: four u16 indices → base4 triples → xyz + extras */
    g4 = (u32)slot_ld_u16(rec7 + 4u);
    g3w = obj ? obj[0] : 0;
    g6 = (u32)slot_ld_u16(rec7 + 6u);
    g2w = obj ? obj[2] : 0; /* +0x8 */
    {
        u32 ea = slot_scaled12(base4, g4);
        slot_ldl(ea, &r4lo, &r4hi);
        g0w = slot_ld_u32(ea + 8u);
        *(u32 *)(frame + 0x40) = r4lo;
        *(u32 *)(frame + 0x44) = r4hi;
        *(u32 *)(frame + 0x48) = g0w;
    }
    {
        u32 ea = slot_scaled12(base4, g6);
        u32 lo, hi;
        slot_ldl(ea, &lo, &hi);
        *(u32 *)(frame + 0x4c) = lo;
        *(u32 *)(frame + 0x50) = hi;
        g6 = slot_ld_u32(ea + 8u);
        *(u32 *)(frame + 0x54) = g6;
    }
    g5 = (u32)slot_ld_u16(rec7 + 8u);
    {
        u32 ea = slot_scaled12(base4, g5);
        u32 lo, hi;
        slot_ldl(ea, &lo, &hi);
        *(u32 *)(frame + 0x58) = lo;
        *(u32 *)(frame + 0x5c) = hi;
        g5 = slot_ld_u32(ea + 8u);
        *(u32 *)(frame + 0x60) = g5;
    }
    g4 = (u32)slot_ld_u16(rec7 + 0xau);
    {
        u32 ea = slot_scaled12(base4, g4);
        u32 lo, hi;
        slot_ldl(ea, &lo, &hi);
        *(u32 *)(frame + 0x64) = lo;
        *(u32 *)(frame + 0x68) = hi;
        g4 = slot_ld_u32(ea + 8u);
        *(u32 *)(frame + 0x6c) = g4;
    }

    /* @0x3B334–0x3B384: first copro burst (marker 0x2e005c5c). */
    i960_mmio_write_u32(0x884000, 0x2e005c5cu);
    i960_mmio_write_u32(0x884000, g2w);
    i960_mmio_write_u32(0x884000, g0w);
    i960_mmio_write_u32(0x884000, *(u32 *)(frame + 0x4c));
    i960_mmio_write_u32(0x884000, g3w);
    i960_mmio_write_u32(0x884000, r4lo);
    i960_mmio_write_u32(0x884000, g6);

    /* @0x3B388: ld FIFO — 0x2e XZ edge side-test result. */
    g2w = i960_mmio_read_u32(0x884000);
    r4lo = *(u32 *)(frame + 0x50);
    r5w = *(u32 *)(frame + 0x54);
    r6w = *(u32 *)(frame + 0x58);
    r8w = *(u32 *)(frame + 0x60);
    r9w = *(u32 *)(frame + 0x64);
    r11w = *(u32 *)(frame + 0x6c);
    g5 = obj ? obj[0] : 0;
    g4 = obj ? obj[2] : 0;

    /*
     * @0x3B3A0–0x3B3E0: second 0x2e after ldq 0x50(fp)→r4..r7.
     * Payload: obj.z, r5=[54]=v1.z, r6=[58]=v2.x, obj.x, g7, g6.
     * g7 is still v1.x from the pre-burst `ld 0x4c(fp)` (not [5c]=v2.y);
     * g6 was reloaded from 0x60 (=v2.z) before the FIFO readback.
     */
    i960_mmio_write_u32(0x884000, 0x2e005c5cu);
    i960_mmio_write_u32(0x884000, g4);
    i960_mmio_write_u32(0x884000, r5w);
    i960_mmio_write_u32(0x884000, r6w);
    i960_mmio_write_u32(0x884000, g5);
    i960_mmio_write_u32(0x884000, *(u32 *)(frame + 0x4c));
    i960_mmio_write_u32(0x884000, *(u32 *)(frame + 0x60));
    g7 = i960_mmio_read_u32(0x884000);

    /* @0x3B3F0–0x3B424: third — ldq 0x60 → 60/64/68/6c; g0 was [58]. */
    g5 = obj ? obj[0] : 0;
    g4 = obj ? obj[2] : 0;
    i960_mmio_write_u32(0x884000, 0x2e005c5cu);
    i960_mmio_write_u32(0x884000, g4);
    i960_mmio_write_u32(0x884000, r8w);
    i960_mmio_write_u32(0x884000, r9w);
    i960_mmio_write_u32(0x884000, g5);
    i960_mmio_write_u32(0x884000, *(u32 *)(frame + 0x58));
    i960_mmio_write_u32(0x884000, r11w);
    g6 = i960_mmio_read_u32(0x884000);

    /* @0x3B440–0x3B47C: fourth — ldq 0x40 → 40/44/48/4c; last word is [48]. */
    g5 = obj ? obj[0] : 0;
    g4 = obj ? obj[2] : 0;
    i960_mmio_write_u32(0x884000, 0x2e005c5cu);
    i960_mmio_write_u32(0x884000, g4);
    i960_mmio_write_u32(0x884000, r11w);
    i960_mmio_write_u32(0x884000, *(u32 *)(frame + 0x40));
    i960_mmio_write_u32(0x884000, g5);
    i960_mmio_write_u32(0x884000, r9w);
    i960_mmio_write_u32(0x884000, *(u32 *)(frame + 0x48));
    g4 = i960_mmio_read_u32(0x884000);

    /*
     * @0x3B458–0x3B4BC: visibility on four 0x2e results. Continue when signs
     * are mixed (not all ≤0, not all >0).
     */
    if (!(i960_u32_to_f64(g2w) > 0.0 || i960_u32_to_f64(g7) > 0.0
          || i960_u32_to_f64(g6) > 0.0 || i960_u32_to_f64(g4) > 0.0))
        goto epilogue;
    if (i960_u32_to_f64(g2w) > 0.0 && i960_u32_to_f64(g7) > 0.0
        && i960_u32_to_f64(g6) > 0.0 && i960_u32_to_f64(g4) > 0.0)
        goto epilogue;

    /*
     * @0x3B4C0–0x3B53C: centroid of four ground verts, then delta to obj.
     * Quads are xyz @ 40/4c/58/64 (X) and 48/54/60/6c (Z). Sum / 2.0
     * (fp3); Y delta forced to 0. Result at 0x70/74/78 for 0x2c distance.
     */
    {
        u32 x0 = *(u32 *)(frame + 0x40);
        u32 x1 = *(u32 *)(frame + 0x4c);
        u32 x2 = *(u32 *)(frame + 0x58);
        u32 x3 = *(u32 *)(frame + 0x64);
        u32 z0 = *(u32 *)(frame + 0x48);
        u32 z1 = *(u32 *)(frame + 0x54);
        u32 z2 = *(u32 *)(frame + 0x60);
        u32 z3 = *(u32 *)(frame + 0x6c);
        double sx, sz;

        sx = i960_u32_to_f64(x0) + i960_u32_to_f64(x1)
            + i960_u32_to_f64(x2) + i960_u32_to_f64(x3);
        sz = i960_u32_to_f64(z0) + i960_u32_to_f64(z1)
            + i960_u32_to_f64(z2) + i960_u32_to_f64(z3);
        fp0 = sx / fp3_two;
        fp1 = sz / fp3_two;
        g7 = i960_f64_to_u32(fp0);
        g5 = i960_f64_to_u32(fp1);
        g4 = obj ? obj[0] : 0;
        g12 = i960_f64_to_u32(i960_u32_to_f64(g7) - i960_u32_to_f64(g4));
        g4 = obj ? obj[2] : 0;
        g5 = i960_f64_to_u32(i960_u32_to_f64(g5) - i960_u32_to_f64(g4));
        *(u32 *)(frame + 0x70) = g12;
        *(u32 *)(frame + 0x74) = 0;
        *(u32 *)(frame + 0x78) = g5;
    }

    /* @0x3B540–0x3B58C: marker 0x2c005858 + distance triple.
     * First point is (0,0,0) — ROM emits g14 thrice (zero in this callee). */
    i960_mmio_write_u32(0x884000, 0x2c005858u);
    i960_mmio_write_u32(0x884000, 0);
    i960_mmio_write_u32(0x884000, 0);
    i960_mmio_write_u32(0x884000, 0);
    i960_mmio_write_u32(0x884000, *(u32 *)(frame + 0x70));
    i960_mmio_write_u32(0x884000, *(u32 *)(frame + 0x74));
    i960_mmio_write_u32(0x884000, *(u32 *)(frame + 0x78));
    g4 = i960_mmio_read_u32(0x884000);

    /* @0x3B594–0x3B5A4: track nearest (min) into fp2 / fp+0x80. */
    if (i960_u32_to_f64(g4) < fp2_max) {
        fp2_max = i960_u32_to_f64(g4);
        *(u32 *)(frame + 0x80) = *(u32 *)(frame + 0x100);
    }

    loop_i++;
    if (loop_i < count)
        goto loop_body;

epilogue:
    /* @0x3B5B0: if loop exhausted, prefer nearest index from fp+0x80. */
    if (loop_i == count)
        *(u32 *)(frame + 0x100) = *(u32 *)(frame + 0x80);

    best_ix = *(u32 *)(frame + 0x100);
    tab_c = *(u32 *)(frame + 0xa0);

    /* @0x3B5C4: lda (tab_c)[best*8] → address; ld pair → 0x214360. */
    if (tab_c != 0) {
        entry = tab_c + (best_ix << 3);
        g12 = slot_ld_u32(entry);
        g7 = slot_ld_u32(entry + 4u);
        i960_st_u32(I960_WORKRAM, 0x214360, 0, g12);
        i960_st_u32(I960_WORKRAM, 0x214364, 0, 0); /* ROM st g14 — float 0 */
        i960_st_u32(I960_WORKRAM, 0x214368, 0, g7);
    } else {
        g7 = 0;
        if (base4 != 0) {
            g12 = slot_ld_u32(base4);
            i960_st_u32(I960_WORKRAM, 0x214360, 0, g12);
            i960_st_u32(I960_WORKRAM, 0x214364, 0, 0);
            i960_st_u32(I960_WORKRAM, 0x214368, 0, 0);
        }
    }

    i960_st_u32(I960_WORKRAM, 0x214340, 0, best_ix);

    /* @0x3B5E4–0x3B710: push, readback view matrix, clear T (ROM g14=0),
     * reload R-only, probe 0x214360, pop. Host must not write link into T. */
    i960_mmio_write_u32(0x884000, 0x10002020u);
    i960_mmio_write_u32(0x884000, 0x13002626u);
    for (g4 = 0; g4 < 12u; g4++)
        ((u32 *)(frame + 0x40))[g4] = i960_mmio_read_u32(0x884000);
    /* fp+0x64/68/6c = matrix T — zero before 0x11002222 reload. */
    *(u32 *)(frame + 0x64) = 0;
    *(u32 *)(frame + 0x68) = 0;
    *(u32 *)(frame + 0x6c) = 0;

    i960_mmio_write_u32(0x884000, 0x11002222u);
    i960_mmio_write_u32(0x884000, *(u32 *)(frame + 0x40));
    i960_mmio_write_u32(0x884000, *(u32 *)(frame + 0x44));
    i960_mmio_write_u32(0x884000, *(u32 *)(frame + 0x48));
    i960_mmio_write_u32(0x884000, *(u32 *)(frame + 0x4c));
    /* stq @ 0x884010 / 0x884020 — host FIFO only decodes 0x884000; emit words. */
    i960_mmio_write_u32(0x884000, *(u32 *)(frame + 0x50));
    i960_mmio_write_u32(0x884000, *(u32 *)(frame + 0x54));
    i960_mmio_write_u32(0x884000, *(u32 *)(frame + 0x58));
    i960_mmio_write_u32(0x884000, *(u32 *)(frame + 0x5c));
    i960_mmio_write_u32(0x884000, *(u32 *)(frame + 0x60));
    i960_mmio_write_u32(0x884000, *(u32 *)(frame + 0x64));
    i960_mmio_write_u32(0x884000, *(u32 *)(frame + 0x68));
    i960_mmio_write_u32(0x884000, *(u32 *)(frame + 0x6c));

    i960_mmio_write_u32(0x884000, 0x16002c2cu);
    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_WORKRAM, 0x214360, 0));
    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_WORKRAM, 0x214364, 0));
    i960_mmio_write_u32(0x884000, i960_ld_u32(I960_WORKRAM, 0x214368, 0));
    g5 = i960_mmio_read_u32(0x884000);
    g6 = i960_mmio_read_u32(0x884000);
    g4 = i960_mmio_read_u32(0x884000);
    *(u32 *)(frame + 0x70) = g5;
    *(u32 *)(frame + 0x74) = g6;
    *(u32 *)(frame + 0x78) = g4;

    i960_mmio_write_u32(0x884000, 0x10802121u);

    /* @0x3B760: cmprl fp0,+0 → return best+1 or ~best. */
    fp0 = i960_u32_to_f64(*(u32 *)(frame + 0x78));
    if (fp0 > 0.0)
        g0 = best_ix + 1u;
    else
        g0 = (u32)(0u - 1u) - best_ix;

    {
        static unsigned s_slot_log;

        if (s_slot_log < 6u && obj) {
            lift_log(
                    "lift: slot_call best_ix=%u g0=%#x query_xz=(%.3g,%.3g) "
                    "probe=(%.3g,%.3g,%.3g)\n",
                    best_ix, (unsigned)g0,
                    (float)i960_u32_to_f64(obj[0]),
                    (float)i960_u32_to_f64(obj[2]),
                    (float)i960_u32_to_f64(*(u32 *)(frame + 0x70)),
                    (float)i960_u32_to_f64(*(u32 *)(frame + 0x74)),
                    (float)i960_u32_to_f64(*(u32 *)(frame + 0x78)));
            s_slot_log++;
        }
    }

    fp = fp_save;
}
