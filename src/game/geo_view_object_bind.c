/* Object bind @ 0x33E90 — multi-car near-plane depth into 0x214124.
 *
 * Disasm: lda 0x20(sp),sp only — keeps the *caller's* fp. Temps at
 * 0x40(fp)/0x50(fp)/0x58(fp) live on scene_frame's private host frame.
 * Solo (0x2139c0 <= 1) skips the loop but still writes 0x214124.
 *
 * 0x2c payload: ldq (r7),g0 loads primary XYZ from 0x40(fp); ldl (g5),g4
 * loads the other car. FIFO order is g0,g4,g1,g5,g2,g6 — not caller g1/g2.
 *
 * source: disasm/maincpu/maincpu_033e90_1d4.asm */
// @rom 0x33e90 +0x1d4 geo_view_object_bind

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

#include <string.h>

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret */
/* abi: void * arg0=g0 → void  (g1/g2 overwritten by ldq on multi-car path) */

void geo_view_object_bind(void *arg0, u32 arg1, u32 arg2)
{
    uintptr_t sp_save = sp;
    u32 base = (u32)(uintptr_t)arg0;
    u32 gx, gy, gz;
    u32 ox, oy, oz;

    (void)arg1;
    (void)arg2;

    /* @0x33E90: lda 0x20(sp),sp — do not rebind fp. */
    sp = sp + 0x20;

    /* ldl (g0),r12; stl r12,0x40(fp); ld 0x8(g0); st 0x48(fp). */
    gx = i960_ld_u32(I960_WORKRAM, base, 0);
    gy = i960_ld_u32(I960_WORKRAM, base, 4);
    gz = i960_ld_u32(I960_WORKRAM, base, 8);
    memcpy((void *)(fp + 0x40), &gx, 4);
    memcpy((void *)(fp + 0x44), &gy, 4);
    memcpy((void *)(fp + 0x48), &gz, 4);

    g5 = i960_ld_u32(I960_WORKRAM, 0x2139c0, 0);
    fp2 = 0.0;
    r4 = 1;
    r8 = i960_ld_u16(I960_WORKRAM, base + 0x56u, 0);
    if ((i32)g5 <= 1)
        goto L_00033fa8;

    r7 = fp + 0x40;
    r6 = fp + 0x50;
    r5 = fp + 0x58;
    g13 = 0x214134u;
    r13 = 0x2c005858u;

L_00033edc:
    g5 = i960_ld_u32(I960_WORKRAM, (u32)g13, 0);
    g4 = i960_ld_u16(I960_WORKRAM, (u32)g5 + 0x56u, 0);
    g7 = r8 - g4;
    fp1 = (double)(i32)(u32)g7;
    /* ldq (r7),g0 — primary XYZ from caller frame. */
    memcpy(&g0, (void *)r7, 4);
    memcpy(&g1, (void *)(r7 + 4), 4);
    memcpy(&g2, (void *)(r7 + 8), 4);
    oz = i960_ld_u32(I960_WORKRAM, (u32)g5 + 8u, 0);
    ox = i960_ld_u32(I960_WORKRAM, (u32)g5, 0);
    oy = i960_ld_u32(I960_WORKRAM, (u32)g5 + 4u, 0);
    g6 = oz;
    g4 = ox;
    /* st other Z @ 0x58(fp); stl other XY @ 0x50(fp). */
    memcpy((void *)r5, &oz, 4);
    memcpy((void *)r6, &ox, 4);
    memcpy((void *)(r6 + 4), &oy, 4);
    /* ldl (g5),g4 overwrites g4/g5 with other XY. */
    g4 = ox;
    g5 = oy;
    i960_mmio_write_u32(0x884000, (u32)r13);
    r10 = 0;
    r11 = 0x40240000u;
    i960_mmio_write_u32(0x884000, (u32)g0);
    fp3 = i960_rifl_read(r10, r11);
    i960_rifl_write(&r10, &r11, fp3 * fp1);
    i960_mmio_write_u32(0x884000, (u32)g4);
    i960_mmio_write_u32(0x884000, (u32)g1);
    i960_mmio_write_u32(0x884000, (u32)g5);
    i960_mmio_write_u32(0x884000, (u32)g2);
    i960_mmio_write_u32(0x884000, (u32)g6);
    r12 = i960_mmio_read_u32(0x884000);
    fp0 = i960_u32_to_f64(r12);
    fp1 = i960_rifl_read(r10, r11);
    g4 = i960_f64_to_u32(fp1);
    if ((i32)g7 < 0) {
        i960_rifl_write(&r10, &r11, fp0);
        r11 = r11 ^ (1u << 31);
        fp0 = i960_rifl_read(r10, r11);
    }
    g5 = i960_f64_to_u32(fp0);
    if (i960_u32_to_f64((u32)g4) <= i960_u32_to_f64((u32)g5))
        goto L_00033f88;
    g4 = g5;
L_00033f88:
    if (fp2 > i960_u32_to_f64((u32)g4))
        fp2 = i960_u32_to_f64((u32)g4);
    g4 = i960_ld_u32(I960_WORKRAM, 0x2139c0, 0);
    g13 = g13 + 4;
    r4 = r4 + 1;
    if ((i32)(signed char)r4 < (i32)(signed char)g4)
        goto L_00033edc;

L_00033fa8:
    r11 = i960_f64_to_u32(fp2);
    fp0 = i960_u32_to_f64(r11);
    r12 = 0;
    r13 = 0x40240000u;
    fp1 = i960_rifl_read(r12, r13);
    i960_rifl_write(&r10, &r11, fp1 * fp0);
    fp0 = i960_rifl_read(r10, r11);
    r11 = i960_f64_to_u32(fp0);
    fp0 = i960_u32_to_f64(r11);
    r12 = 0;
    r13 = 0x40ab5800u;
    fp1 = i960_rifl_read(r12, r13);
    fp2 = i960_u32_to_f64(r11);
    i960_rifl_write(&r10, &r11, fp1 + fp0);
    r12 = 0;
    r13 = 0x40a45000u;
    fp3 = i960_rifl_read(r12, r13);
    fp0 = i960_rifl_read(r10, r11);
    if (fp0 >= fp3)
        goto L_00034018;
    i960_st_u32(I960_WORKRAM, 0x214124, 0, 0x3b7745bau);
    goto L_done;

L_00034018:
    r13 = i960_f64_to_u32(fp2);
    fp1 = i960_u32_to_f64(r13);
    r10 = 0;
    r11 = 0x40ab5800u;
    fp0 = i960_rifl_read(r10, r11);
    r10 = 0x51eb851fu;
    r11 = 0x40239eb8u;
    i960_rifl_write(&r12, &r13, fp0 + fp1);
    fp0 = i960_rifl_read(r10, r11);
    fp1 = i960_rifl_read(r12, r13);
    i960_rifl_write(&r12, &r13, fp0 / fp1);
    fp0 = i960_rifl_read(r12, r13);
    g4 = i960_f64_to_u32(fp0);
    i960_st_u32(I960_WORKRAM, 0x214124, 0, (u32)g4);

L_done:
    sp = sp_save;
}
