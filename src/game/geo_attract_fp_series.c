/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_033c00_1b8.asm */
// @rom 0x33c00 +0x1b8 geo_attract_fp_series

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_rom.h"

#include <string.h>

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret  callee r11 */
/* abi: u32 table_va=g0, u32 arg1=g1, u32 arg2=g2 → void (XYZ in g0/g1/g2) */
/* call site: caller 0x12054 / 0x12164 — g0 is a guest workram VA (e.g. 0x5ae400).
 * Table words live in the ROM-mirrored window; load via i960_ld so zero workram
 * cells fall back to maincpu (C-pointer into model2_workram would read zeros).
 *
 * Weight math is the cubic uniform B-spline basis (N_i,3), not Bezier:
 *   N0=(1-t)^3/6, N1=(3t^3-6t^2+4)/6, N2=(-3t^3+3t^2+3t+1)/6, N3=t^3/6
 * At t=0 the curve is (1/6)P0+(2/3)P1+(1/6)P2 — not P0. */

void geo_attract_fp_series(u32 table_va, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    /* Callee frame — disasm `call` owns 0x30(fp)/0x50(fp); do not share caller. */
    u8 frame[0x60];

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;

    g1 = (uintptr_t)arg1;
    g2 = (uintptr_t)arg2;

    r11 = g8;
    sp = sp + 0x40;
    g13 = fp + 0x30;
    *(u32 *)(sp - 0x20) = (u32)g9;
    /* Disasm: stl g10,0xffffffe8(sp) — g10:g11 pair. */
    *(u64 *)(sp - 0x18) = ((u64)g11 << 32) | (u32)g10;
    g9 = i960_f64_to_u32((i960_u32_to_f64(arg1)) * (i960_u32_to_f64(arg1)));
    fp3 = i960_u32_to_f64(arg1);
    i960_rifl_write(&g4, &g5, fp3);
    i960_rifl_write(&g10, &g11, (1.0) - (i960_rifl_read(g4, g5)));
    fp1 = i960_rifl_read(g10, g11);
    fp2 = i960_u32_to_f64(g9);
    i960_rifl_write(&g8, &g9, (fp1) * (fp1));
    r6 = i960_f64_to_u32((i960_u32_to_f64(arg1)) * (fp2));
    g10 = i960_f64_to_u32(fp2);
    fp2 = i960_u32_to_f64(g10);
    /* Disasm: stl g8,0x50(fp) — saves g8:g9 (double (1-t)^2). */
    *(u64 *)(fp + 0x50) = ((u64)g9 << 32) | (u32)g8;
    fp3 = i960_u32_to_f64(r6);
    {
        u64 tmp = *(u64 *)(fp + 0x50);

        g10 = (u32)tmp;
        g11 = (u32)(tmp >> 32);
    }
    i960_rifl_write(&r6, &r7, fp3);
    fp3 = i960_rifl_read(g10, g11);
    i960_rifl_write(&g10, &g11, (fp1) * (fp3));
    g8 = 0;
    g9 = 0 | (1u << 30);
    fp0 = i960_rifl_read(g8, g9);
    *(u64 *)(fp + 0x50) = ((u64)g11 << 32) | (u32)g10;
    i960_rifl_write(&g10, &g11, (fp2) / (fp0));
    fp1 = i960_rifl_read(g10, g11);
    {
        u64 tmp = *(u64 *)(fp + 0x50);

        g10 = (u32)tmp;
        g11 = (u32)(tmp >> 32);
    }
    g8 = 0;
    g9 = 0x40180000u;
    i960_rifl_write(&g4, &g5, (i960_rifl_read(g4, g5)) / (fp0));
    i960_rifl_write(&g6, &g7, (i960_rifl_read(r6, r7)) / (fp0));
    fp0 = i960_rifl_read(g8, g9);
    fp3 = i960_rifl_read(g10, g11);
    i960_rifl_write(&g10, &g11, (fp3) / (fp0));
    *(u64 *)(fp + 0x50) = ((u64)g11 << 32) | (u32)g10;
    i960_rifl_write(&g10, &g11, (i960_rifl_read(g6, g7)) - (fp2));
    i960_rifl_write(&g6, &g7, (fp1) - (i960_rifl_read(g6, g7)));
    g8 = 0x55555555u;
    g9 = 0x3fe55555u;
    i960_rifl_write(&r6, &r7, (i960_rifl_read(r6, r7)) / (fp0));
    fp0 = i960_rifl_read(g8, g9);
    fp2 = i960_rifl_read(g10, g11);
    i960_rifl_write(&g10, &g11, (fp0) + (fp2));
    fp1 = i960_rifl_read(g6, g7);
    i960_rifl_write(&g4, &g5, (i960_rifl_read(g4, g5)) + (fp1));
    fp2 = i960_rifl_read(g10, g11);
    {
        u64 tmp = *(u64 *)(fp + 0x50);

        g10 = (u32)tmp;
        g11 = (u32)(tmp >> 32);
    }
    g6 = i960_ld_u32(I960_WORKRAM, table_va, 0);
    g8 = 0x55555555u;
    g9 = 0x3fc55555u;
    fp0 = i960_rifl_read(g8, g9);
    fp1 = i960_rifl_read(g4, g5);
    g5 = i960_ld_u32(I960_WORKRAM, table_va, 0xc);
    fp3 = i960_rifl_read(g10, g11);
    i960_rifl_write(&g10, &g11, (fp0) + (fp1));
    g3 = i960_f64_to_u32(fp2);
    g1 = i960_ld_u32(I960_WORKRAM, table_va, 0x4);
    r5 = i960_ld_u32(I960_WORKRAM, table_va, 0x8);
    r10 = i960_f64_to_u32((i960_u32_to_f64(g5)) * (i960_u32_to_f64(g3)));
    g5 = i960_ld_u32(I960_WORKRAM, table_va, 0x10);
    fp1 = i960_rifl_read(g10, g11);
    g2 = i960_ld_u32(I960_WORKRAM, table_va, 0x18);
    g7 = i960_f64_to_u32(fp1);
    g4 = i960_f64_to_u32(fp3);
    fp3 = i960_rifl_read(r6, r7);
    r4 = i960_f64_to_u32((i960_u32_to_f64(g6)) * (i960_u32_to_f64(g4)));
    g6 = i960_ld_u32(I960_WORKRAM, table_va, 0x14);
    r6 = i960_f64_to_u32(fp3);
    r9 = i960_f64_to_u32((i960_u32_to_f64(g2)) * (i960_u32_to_f64(g7)));
    g2 = i960_ld_u32(I960_WORKRAM, table_va, 0x24);
    r8 = i960_f64_to_u32((i960_u32_to_f64(g5)) * (i960_u32_to_f64(g3)));
    g5 = i960_ld_u32(I960_WORKRAM, table_va, 0x1c);
    g3 = i960_f64_to_u32((i960_u32_to_f64(g6)) * (i960_u32_to_f64(g3)));
    g6 = i960_ld_u32(I960_WORKRAM, table_va, 0x20);
    g1 = i960_f64_to_u32((i960_u32_to_f64(g1)) * (i960_u32_to_f64(g4)));
    g4 = i960_f64_to_u32((i960_u32_to_f64(r5)) * (i960_u32_to_f64(g4)));
    r5 = i960_f64_to_u32((i960_u32_to_f64(g2)) * (i960_u32_to_f64(r6)));
    g2 = i960_f64_to_u32((i960_u32_to_f64(g5)) * (i960_u32_to_f64(g7)));
    r4 = i960_f64_to_u32((i960_u32_to_f64(r10)) + (i960_u32_to_f64(r4)));
    g5 = i960_ld_u32(I960_WORKRAM, table_va, 0x28);
    g7 = i960_f64_to_u32((i960_u32_to_f64(g6)) * (i960_u32_to_f64(g7)));
    g6 = i960_ld_u32(I960_WORKRAM, table_va, 0x2c);
    g0 = i960_f64_to_u32((i960_u32_to_f64(g5)) * (i960_u32_to_f64(r6)));
    g5 = i960_f64_to_u32((i960_u32_to_f64(g6)) * (i960_u32_to_f64(r6)));
    g1 = i960_f64_to_u32((i960_u32_to_f64(r8)) + (i960_u32_to_f64(g1)));
    g4 = i960_f64_to_u32((i960_u32_to_f64(g3)) + (i960_u32_to_f64(g4)));
    r4 = i960_f64_to_u32((i960_u32_to_f64(r9)) + (i960_u32_to_f64(r4)));
    g1 = i960_f64_to_u32((i960_u32_to_f64(g2)) + (i960_u32_to_f64(g1)));
    g4 = i960_f64_to_u32((i960_u32_to_f64(g7)) + (i960_u32_to_f64(g4)));
    r12 = i960_f64_to_u32((i960_u32_to_f64(r5)) + (i960_u32_to_f64(r4)));
    g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) + (i960_u32_to_f64(g4)));
    r13 = i960_f64_to_u32((i960_u32_to_f64(g0)) + (i960_u32_to_f64(g1)));
    *(u32 *)(g13 + 0x8) = (u32)g4;
    r14 = g4;
    /* Disasm: stl r12,(g13) then ldq 0x30(fp),g0 — X/Y/Z(/pad) return. */
    *(u32 *)g13 = (u32)r12;
    *(u32 *)((uintptr_t)g13 + 4) = (u32)r13;
    g8 = r11;
    g0 = *(u32 *)(fp + 0x30);
    g1 = *(u32 *)(fp + 0x34);
    g2 = *(u32 *)(fp + 0x38);
    g3 = *(u32 *)(fp + 0x3c);
    g9 = *(u32 *)(sp - 0x20);
    {
        u64 saved = *(u64 *)(sp - 0x18);
        g10 = (u32)saved;
        g11 = (u32)(saved >> 32);
    }
    sp = sp_save;
    fp = fp_save;
    return;
}
