/* Road span basis @ 0x31320 — called from road_span 0x311E0.
 *
 * Builds weights from g1 (=t), blends span-row columns at g0, issues
 * TGP 0x20/0x25/0x54/0x27, seeds *(0x20a290), then 0x2c(ox) → g3
 * (&node+0x90) and latches 0x20cad0/0x20cad8.
 *
 * Weight path @ 0x31340: subrl is src2−src1 (MAME i960) — g5=t−1,
 * r4=−t²+t+0.5 (notbit on t²), not 1−t / t²+t+0.5.
 *
 * TGP 0x60 stores the matrix (fw @ 0x3DB; host no-op). 0x51 orients R
 * along (bz,0,by) after the nested 0x25/0x27(bx,0,cx) rebuild so 0x2c(ox)
 * yields world XZ into out.
 *
 * Uses *caller's* fp (lda 0x90(sp) only). g3 = out VA at entry.
 *
 * source: disasm/maincpu/maincpu_031320_340.asm */
// @rom 0x31320 +0x320 game_start_race_obj_road_span_basis

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_rom.h"

#include <stdio.h>
#include <string.h>

void game_start_race_obj_road_span_basis(u32 arg0, u32 arg1, u32 arg2)
{
    u32 row = arg0;
    u32 tbits = arg1;
    u32 ox = arg2;
    u32 out = (u32)g3;
    uintptr_t sp_save = sp;
    u32 g8_save = (u32)g8;
    double t = i960_u32_to_f64(tbits);
    double t2 = t * t;
    double half = 0.5;
    u32 w_g5; /* t-1  (subrl +1.0,fp0 → src2-src1) */
    u32 w_r5; /* 1-2t (subrl g6,+1.0) */
    u32 w_r4; /* -t²+t+0.5 (notbit t²; addr g1; +0.5) */
    u32 w_g4; /* t²/2-t+0.5 */
    u32 ht2;  /* t²/2 */
    u32 v00, v08, v10, v18, v20, v28;
    u32 bx, by, bz, cx;
    u32 cursor;

    sp = sp + 0x90u;
    *(u32 *)(sp - 0x20) = (u32)g9;
    *(u64 *)(sp - 0x18) = (u64)g10;
    *(u32 *)(sp - 0x10) = (u32)g12;
    g14 = 0;

    /* @0x31340–0x313D4: subrl is src2-src1 (MAME i960); notbit 31 on t². */
    ht2 = (u32)i960_f64_to_u32(t2 * half);
    w_g5 = (u32)i960_f64_to_u32(t - 1.0);
    w_r5 = (u32)i960_f64_to_u32(1.0 - (t + t));
    w_r4 = (u32)i960_f64_to_u32(-t2 + t + half);
    w_g4 = (u32)i960_f64_to_u32(t2 * half - t + half);

    v10 = i960_ld_u32(I960_ABS, row, 0x10u);
    v00 = i960_ld_u32(I960_ABS, row, 0);
    v08 = i960_ld_u32(I960_ABS, row, 8u);
    v18 = i960_ld_u32(I960_ABS, row, 0x18u);
    v20 = i960_ld_u32(I960_ABS, row, 0x20u);
    v28 = i960_ld_u32(I960_ABS, row, 0x28u);

    /* @0x313DC–0x31450 blend → bx/by/bz/cx (g8 / r14 / r12 / g10). */
    {
        double wr5 = i960_u32_to_f64(w_r5);
        double wg5 = i960_u32_to_f64(w_g5);
        double wr4 = i960_u32_to_f64(w_r4);
        double wg4 = i960_u32_to_f64(w_g4);
        double ft = t;
        double fht2 = i960_u32_to_f64(ht2);
        double f10 = i960_u32_to_f64(v10);
        double f00 = i960_u32_to_f64(v00);
        double f08 = i960_u32_to_f64(v08);
        double f18 = i960_u32_to_f64(v18);
        double f20 = i960_u32_to_f64(v20);
        double f28 = i960_u32_to_f64(v28);
        double s_r8 = wr5 * f10;
        double s_r6 = wg5 * f00;
        double s_g7 = wr4 * f10;
        double s_r9 = wg4 * f00;
        double s_g5 = wg5 * f08;
        double s_r5 = wr5 * f18;
        double s_r4 = wr4 * f18;
        double s_g4 = wg4 * f08;
        double s_g0 = ft * f20;
        double s_g6 = fht2 * f20;
        double s_g1 = ft * f28;
        double s_g7b = fht2 * f28;

        s_g5 += s_r5;
        s_r9 += s_g7;
        s_r6 += s_r8;
        s_g4 += s_r4;
        s_g5 += s_g1;
        bx = (u32)i960_f64_to_u32(s_r9 + s_g6);
        by = (u32)i960_f64_to_u32(s_g5);
        bz = (u32)i960_f64_to_u32(s_r6 + s_g0);
        cx = (u32)i960_f64_to_u32(s_g4 + s_g7b);
    }

    /* stl pairs @ fp+0x50 / fp+0x40; by @ 0x58; cx @ 0x48. */
    *(u32 *)(fp + 0x50u) = bz;
    *(u32 *)(fp + 0x54u) = 0;
    *(u32 *)(fp + 0x40u) = bx;
    *(u32 *)(fp + 0x44u) = 0;
    *(u32 *)(fp + 0x58u) = by;
    *(u32 *)(fp + 0x48u) = cx;

    {
        static int logged;

        if (!logged) {
            lift_log(
                    "lift: road_span_basis t=%.4g ox=%.4g "
                    "bx=%.4g by=%.4g bz=%.4g cx=%.4g "
                    "rowX=(%.4g,%.4g,%.4g) rowZ=(%.4g,%.4g,%.4g)\n",
                    t, i960_u32_to_f64(ox),
                    i960_u32_to_f64(bx), i960_u32_to_f64(by),
                    i960_u32_to_f64(bz), i960_u32_to_f64(cx),
                    i960_u32_to_f64(v00), i960_u32_to_f64(v10),
                    i960_u32_to_f64(v20), i960_u32_to_f64(v08),
                    i960_u32_to_f64(v18), i960_u32_to_f64(v28));
            fflush(stderr);
            logged = 1;
        }
    }

    /* @0x31470–0x314BC: 0x20; 0x25; 0x54(bz, 0, by). */
    i960_mmio_write_u32(0x884000, 0x10002020u);
    i960_mmio_write_u32(0x884000, 0x12802525u);
    i960_mmio_write_u32(0x884000, 0x2a005454u);
    i960_mmio_write_u32(0x884000, bz);
    i960_mmio_write_u32(0x884000, 0);
    i960_mmio_write_u32(0x884000, by);

    /* @0x314C4–0x314F8: 0x27(−bx, 0, −cx). */
    i960_mmio_write_u32(0x884000, 0x13802727u);
    i960_mmio_write_u32(0x884000, bx ^ 0x80000000u);
    i960_mmio_write_u32(0x884000, 0);
    i960_mmio_write_u32(0x884000, cx ^ 0x80000000u);

    /* @0x314F0–0x31540: *cursor=0; 0x60; nested 0x25/0x27(bx,0,cx); cursor+=4.
     * ldq 0x40(fp) → bx,0,cx (cx @ 0x48); not by @ 0x58. */
    cursor = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
    if (cursor != 0u)
        i960_st_u32(I960_ABS, cursor, 0, 0);
    i960_mmio_write_u32(0x884000, 0x30006060u);
    i960_mmio_write_u32(0x884000, 0x12802525u);
    i960_mmio_write_u32(0x884000, 0x13802727u);
    i960_mmio_write_u32(0x884000, bx);
    i960_mmio_write_u32(0x884000, 0);
    i960_mmio_write_u32(0x884000, cx);
    if (cursor != 0u) {
        cursor += 4u;
        i960_st_u32(I960_WORKRAM, 0x20a290, 0, cursor);
    }

    /* @0x31554–0x31588: 0x51(bz,0,by); *cursor=16. */
    i960_mmio_write_u32(0x884000, 0x28805151u);
    i960_mmio_write_u32(0x884000, bz);
    i960_mmio_write_u32(0x884000, 0);
    i960_mmio_write_u32(0x884000, by);
    if (cursor != 0u)
        i960_st_u32(I960_ABS, cursor, 0, 16u);

    /* @0x3158C–0x315E0: 0x60; 0x2c(ox,0,0); readback → out. */
    i960_mmio_write_u32(0x884000, 0x30006060u);
    i960_mmio_write_u32(0x884000, 0x16002c2cu);
    i960_mmio_write_u32(0x884000, ox);
    i960_mmio_write_u32(0x884000, 0);
    i960_mmio_write_u32(0x884000, 0);
    {
        u32 oxr = i960_mmio_read_u32(0x884000);
        u32 oyr = i960_mmio_read_u32(0x884000);
        u32 ozr = i960_mmio_read_u32(0x884000);
        static int out_logged;

        if (out != 0u) {
            i960_st_u32(I960_ABS, out, 0, oxr);
            i960_st_u32(I960_ABS, out, 8, ozr);
        }
        *(u32 *)(fp + 0x44u) = oyr;
        if (!out_logged) {
            lift_log(
                    "lift: road_span_basis 0x2c out=(%.4g,%.4g,%.4g)\n",
                    i960_u32_to_f64(oxr), i960_u32_to_f64(oyr),
                    i960_u32_to_f64(ozr));
            fflush(stderr);
            out_logged = 1;
        }
    }

    i960_mmio_write_u32(0x884000, 0x10802121u);

    /* @0x315F8–0x3161C: cursor+=4; 0x20cad8=ox; 0x20cad0=(bz,0). */
    if (cursor != 0u) {
        cursor += 4u;
        i960_st_u32(I960_WORKRAM, 0x20a290, 0, cursor);
    }
    i960_st_u32(I960_WORKRAM, 0x20cad8, 0, ox);
    i960_st_u32(I960_WORKRAM, 0x20cad0, 0, bz);
    i960_st_u32(I960_WORKRAM, 0x20cad4, 0, 0);

    g8 = g8_save;
    g9 = *(u32 *)(sp - 0x20);
    g10 = (u32)*((u64 *)(sp - 0x18));
    g12 = *(u32 *)(sp - 0x10);
    sp = sp_save;
}
