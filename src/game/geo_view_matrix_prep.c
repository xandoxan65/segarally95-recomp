/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_03a320_704.asm */
// @rom 0x3a320 +0x704 geo_view_matrix_prep

#include "i960_lift.h"
#include "i960_fp.h"
#include "model2_rom.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>
#include <string.h>

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret  callee r6,r8 */
/* abi: u32 arg0=g0, u32 arg1=g1, u32 arg2=g2 → u32 g0 */


/* Cam / scratch are guest workram VAs (e.g. cam @ 0x500000, scratch @ 0x214254). */
typedef struct {
    uintptr_t v[16];
} matrix_prep_local_regs;

static matrix_prep_local_regs save_matrix_prep_locals(void)
{
    matrix_prep_local_regs s = {{
        r0, r1, r2, r3, r4, r5, r6, r7,
        r8, r9, r10, r11, r12, r13, r14, r15,
    }};

    return s;
}

static void restore_matrix_prep_locals(matrix_prep_local_regs s)
{
    r0 = s.v[0];
    r1 = s.v[1];
    r2 = s.v[2];
    r3 = s.v[3];
    r4 = s.v[4];
    r5 = s.v[5];
    r6 = s.v[6];
    r7 = s.v[7];
    r8 = s.v[8];
    r9 = s.v[9];
    r10 = s.v[10];
    r11 = s.v[11];
    r12 = s.v[12];
    r13 = s.v[13];
    r14 = s.v[14];
    r15 = s.v[15];
}

static u32 cam_ld(u32 cam, u32 off)
{
    return i960_ld_u32(I960_WORKRAM, cam, off);
}

static void cam_st(u32 cam, u32 off, u32 v)
{
    i960_st_u32(I960_WORKRAM, cam, off, v);
}

static u32 scr_ld(u32 ea)
{
    return i960_ld_u32(I960_WORKRAM, ea, 0);
}

static void scr_st(u32 ea, u32 v)
{
    i960_st_u32(I960_WORKRAM, ea, 0, v);
}

u32 geo_view_matrix_prep(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    u8 frame[0x50];
    u32 cam;
    u32 scratch;

    (void)arg2;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    sp = sp + 16;
    cam = arg0 != 0u ? arg0 : (u32)g0;
    scratch = arg1 != 0u ? arg1 : (u32)g1;
    g4 = i960_ld_u8(I960_WORKRAM, 0x202051, 0);
    g3 = 0xff;
    g4 = g3 & g4;
    g4 = g4 - 0x30;
    r5 = i960_f64_to_u32((double)(i32)(u32)(g4));
    fp0 = i960_u32_to_f64(r5);
    r6 = cam;
    r8 = scratch;
    if (fp0 >= 0.0)
        goto L_0003a35c;
    fp0 = 0.0;
    goto L_0003a360;

    L_0003a35c:
        fp0 = i960_u32_to_f64(r5);

    L_0003a360:
        g2 = 0;
        g3 = 0x4061e000;
        fp3 = i960_rifl_read(g2, g3);
        if (fp0 >= fp3)
            goto L_0003a390;
        fp0 = i960_u32_to_f64(r5);
        g1 = r5;
        if (fp0 >= 0.0)
            goto L_0003a398;
        g1 = 0;
        goto L_0003a398;

    L_0003a390:
        g1 = 0x430f0000u;

    L_0003a398:
        g6 = i960_ld_u32(I960_WORKRAM, 0x2142fc, 0);
        g4 = 0x430f0000u;
        r5 = i960_f64_to_u32((i960_u32_to_f64(g1)) / (i960_u32_to_f64(g4)));
        if (g6 > 0) {
            g6 = g6 - 1;
            r5 = 0x3e99999au;
        }
    g5 = i960_ld_u32(I960_WORKRAM, 0x214300, 0);
    i960_st_u32(I960_WORKRAM, 0x2142fc, 0, (u32)g6);
    if (0 > (signed char)g5) {
        g5 = g5 + 1;
    }
    g4 = i960_ld_u32(I960_WORKRAM, 0x214120, 0);
    i960_st_u32(I960_WORKRAM, 0x214300, 0, (u32)g5);
    if (1 < (signed char)g4) {
        r5 = 0;
    }
    g4 = i960_ld_u32(I960_WORKRAM, 0x2139d0, 0);
    if ((unsigned char)g4 != 0)
        goto L_0003a408;
    g1 = r5;
    g0 = geo_view_matrix_slot_init(cam, (u32)g1, (u32)g2);
    cam_st(cam, 0x78, (u32)g0);
    goto L_0003a418;

    L_0003a408:
        r4 = cam + 0x78u;
        g0 = cam_ld(cam, 0x78u);
        g0 = geo_view_matrix_slot_step((u32)g0, (u32)g1, (u32)g2);
    cam_st(cam, 0x78u, (u32)g0);

    L_0003a418:
        g4 = i960_ld_u32(I960_WORKRAM, 0x214120, 0);
        if ((unsigned char)g4 == 0)
            goto L_0003a430;
        g4 = i960_ld_u32(I960_WORKRAM, 0x2142fc, 0);
        if ((unsigned char)g4 == 0)
            goto L_0003a438;

    L_0003a430:
        r9 = 0;
        goto L_0003a43c;

    L_0003a438:
        r9 = cam_ld(cam, 0x78);

    L_0003a43c:
        g4 = i960_ld_u32(I960_WORKRAM, 0x202008, 0);
        if (g4 & 1)
            goto L_0003a454;
        g0 = cam_ld(cam, 0x7c);
        g1 = r5;
        {
            matrix_prep_local_regs caller_r = save_matrix_prep_locals();

            geo_view_matrix_slot_fill((u32)g0, (u32)g1, (u32)g2);
            restore_matrix_prep_locals(caller_r);
        }

    L_0003a454:
        g6 = i960_ld_u32(I960_WORKRAM, 0x5d9264, (u32)(r9 << 2));
        g4 = i960_ld_u32(I960_WORKRAM, 0x5d9254, 0);
        g6 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(g6)));
        g2 = 0x10002020u;
        i960_mmio_write_u32(0x884000, (u32)g2); /* copro_fifo */;
        g3 = 0x12802525u;
        i960_mmio_write_u32(0x884000, (u32)g3); /* copro_fifo */;
        i960_st_u32(I960_WORKRAM, 0x2142ec, 0, (u32)g6);
        g5 = cam_ld(cam, 0x3c);
        g4 = cam_ld(cam, 0x74);
        g5 = i960_f64_to_u32((i960_u32_to_f64(g4)) + (i960_u32_to_f64(g5)));
        g13 = 0x15002a2au;
        i960_mmio_write_u32(0x884000, (u32)g13); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
        g2 = 0x16002c2cu;
        i960_mmio_write_u32(0x884000, (u32)g2); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
        fp3 = 1.0;
        g3 = i960_f64_to_u32(fp3);
        i960_mmio_write_u32(0x884000, (u32)g3); /* copro_fifo */;
        g0 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        g5 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        g2 = 0x2c805959u;
        i960_mmio_write_u32(0x884000, (u32)g2); /* copro_fifo */;
        *(u32 *)(fp + 0x44) = (u32)g4;
        g4 = cam_ld(cam, 0x2c);
        g7 = i960_ld_u32(I960_WORKRAM, 0x5d9304, 0);
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g0); /* copro_fifo */;
        *(u32 *)(fp + 0x48) = (u32)g5;
        g5 = cam_ld(cam, 0x30);
        g4 = *(u32 *)(fp + 0x44);
        g7 = i960_f64_to_u32((i960_u32_to_f64(g7)) / (i960_u32_to_f64(g6)));
        i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        g4 = cam_ld(cam, 0x34);
        g5 = *(u32 *)(fp + 0x48);
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
        r10 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        fp0 = i960_u32_to_f64(r10);
        g4 = i960_ld_u32(I960_WORKRAM, 0x5d9300, 0);
        g3 = 0x10802121u;
        i960_mmio_write_u32(0x884000, (u32)g3); /* copro_fifo */;
        *(u32 *)(fp + 0x40) = (u32)g0;
        r11 = i960_f64_to_u32((i960_u32_to_f64(g4)) + (i960_u32_to_f64(g7)));
        if (fp0 >= 0.0)
            goto L_0003a5a0;
        r7 = 0;
        goto L_0003a5f4;

    L_0003a5a0:
        g4 = i960_ld_u32(I960_WORKRAM, 0x20b0b4, 0);
        g5 = 0x42700000u;
        g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) * (i960_u32_to_f64(g4)));
        g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) * (i960_u32_to_f64(g4)));
        fp1 = i960_u32_to_f64(g4);
        fp0 = 1.8849555921538759;
        i960_rifl_write(&g2, &g3, (fp1) / (fp0));
        fp1 = i960_rifl_read(g2, g3);
        g3 = i960_ld_u32(I960_WORKRAM, 0x2142ec, 0);
        fp0 = i960_u32_to_f64(g3);
        fp0 = i960_u32_to_f64(g3);
        i960_rifl_write(&g2, &g3, (fp0) * (fp1));
        fp1 = i960_rifl_read(g2, g3);
        r7 = i960_f64_to_u32(fp1);

    L_0003a5f4:
        if ((unsigned char)r9 == 0)
            goto L_0003a7a8;
        g4 = i960_ld_u32(I960_WORKRAM, 0x2142e4, 0);
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) + (i960_u32_to_f64(r7)));
        fp0 = i960_u32_to_f64(g4);
        fp1 = i960_u32_to_f64(g4);
        if (fp0 >= 0.0)
            goto L_0003a61c;
        fp0 = 0.0;
        goto L_0003a624;

    L_0003a61c:
        g3 = i960_f64_to_u32(fp1);
        fp0 = i960_u32_to_f64(g3);

    L_0003a624:
        g2 = 0;
        g3 = 0x40c13000u;
        fp3 = i960_rifl_read(g2, g3);
        if (fp0 >= fp3)
            goto L_0003a65c;
        g2 = i960_f64_to_u32(fp1);
        fp0 = i960_u32_to_f64(g2);
        if (fp0 >= 0.0)
            goto L_0003a654;
        g4 = 0;
        goto L_0003a664;

    L_0003a654:
        g4 = i960_f64_to_u32(fp1);
        goto L_0003a664;

    L_0003a65c:
        g4 = 0x46098000u;

    L_0003a664:
        i960_st_u32(I960_WORKRAM, 0x2142e8, 0, (u32)g4);
        g6 = cam + 0x7cu;
        g5 = cam_ld(cam, 0x7cu);
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) - (i960_u32_to_f64(g5)));
        g3 = g4 & ~(1u << 31);
        fp0 = i960_u32_to_f64(g3);
        fp0 = i960_u32_to_f64(g3);
        g2 = 0;
        g3 = 0x40690000;
        fp3 = i960_rifl_read(g2, g3);
        if (fp0 <= fp3)
            goto L_0003a6b4;
        /* |delta| > 200: rate-limit, keeping the sign of the original delta. */
        if (i960_u32_to_f64(g4) >= i960_u32_to_f64(+0.0))
            g4 = 0x43480000u;
        else
            g4 = 0xc3480000u;

    L_0003a6b4:
        g5 = i960_f64_to_u32((i960_u32_to_f64(g5)) + (i960_u32_to_f64(g4)));
        cam_st(cam, 0x7cu, (u32)g5);
        g4 = i960_ld_u32(I960_WORKRAM, 0x2020b4, 0);
        g2 = cam_ld(cam, 0x7c);
        g5 = scr_ld(scratch);
        fp0 = i960_u32_to_f64(g2);
        if (g4 == 0)
            goto L_0003a70c;
        g4 = i960_ld_u32(I960_WORKRAM, 0x5d9318, 0);
        g5 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(g5)));
        fp1 = i960_u32_to_f64(g5);
        fp1 = i960_u32_to_f64(g5);
        i960_rifl_write(&g2, &g3, (1.0) + (fp1));
        fp1 = i960_rifl_read(g2, g3);
        g3 = i960_f64_to_u32(fp0);
        fp0 = i960_u32_to_f64(g3);
        i960_rifl_write(&g2, &g3, (fp1) * (fp0));
        fp0 = i960_rifl_read(g2, g3);
        g5 = i960_f64_to_u32(fp0);
        goto L_0003a710;

    L_0003a70c:
        g5 = i960_f64_to_u32(fp0);

    L_0003a710:
        g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) - (i960_u32_to_f64(r7)));
        g5 = i960_f64_to_u32((i960_u32_to_f64(g4)) / (i960_u32_to_f64(g5)));
        if (i960_u32_to_f64(g5) < i960_u32_to_f64(+0.0)) {
            g5 = g5 ^ (1u << 31);
        }
    fp0 = i960_u32_to_f64(g5);
    if (fp0 <= 1.0)
        goto L_0003a73c;
    g4 = 0x3f800000u;
    goto L_0003a740;

    L_0003a73c:
        g4 = g5;

    L_0003a740:
        scr_st(scratch, (u32)g4);
        g3 = cam_ld(cam, 0x7c);
        fp1 = i960_u32_to_f64(g3);
        fp1 = i960_u32_to_f64(g3);
        fp0 = (2.0 * 3.141592653589793);
        i960_rifl_write(&g2, &g3, (fp0) * (fp1));
        fp1 = i960_rifl_read(g2, g3);
        g3 = i960_ld_u32(I960_WORKRAM, 0x2142ec, 0);
        fp0 = i960_u32_to_f64(g3);
        fp0 = i960_u32_to_f64(g3);
        i960_rifl_write(&g2, &g3, (fp1) / (fp0));
        fp1 = i960_rifl_read(g2, g3);
        g2 = 0;
        g3 = 0x40ac2000u;
        fp0 = i960_rifl_read(g2, g3);
        i960_rifl_write(&g2, &g3, (fp1) / (fp0));
        fp1 = i960_rifl_read(g2, g3);
        g4 = i960_f64_to_u32(fp1);
        cam_st(cam, 0x80, (u32)g4);
        goto L_0003a7ec;

    L_0003a7a8:
        g3 = i960_ld_u32(I960_WORKRAM, 0x2142e8, 0);
        cam_st(cam, 0x7c, (u32)g3);
        g13 = i960_ld_u32(I960_WORKRAM, 0x20b0b4, 0);
        fp0 = i960_u32_to_f64(g13);
        fp0 = i960_u32_to_f64(g13);
        fp1 = 0.3;
        i960_rifl_write(&g2, &g3, (fp0) / (fp1));
        fp0 = i960_rifl_read(g2, g3);
        g4 = i960_f64_to_u32(fp0);
        cam_st(cam, 0x80, (u32)g4);
        scr_st(scratch, (u32)g14);

    L_0003a7ec:
        g3 = i960_ld_u32(I960_WORKRAM, 0x20b0b4, 0);
        fp1 = i960_u32_to_f64(g3);
        fp1 = i960_u32_to_f64(g3);
        fp0 = 0.3;
        i960_rifl_write(&g2, &g3, (fp1) / (fp0));
        fp1 = i960_rifl_read(g2, g3);
        g3 = cam_ld(cam, 0x7c);
        g6 = cam + 0x84u;
        g4 = cam_ld(cam, 0x84u);
        g5 = i960_f64_to_u32(fp1);
        fp0 = i960_u32_to_f64(g3);
        fp0 = i960_u32_to_f64(g3);
        g2 = 0;
        g3 = 0x408f4000u;
        fp3 = i960_rifl_read(g2, g3);
        g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) + (i960_u32_to_f64(g4)));
        cam_st(cam, 0x80, (u32)g5);
        cam_st(cam, 0x84u, (u32)g4);
        if (fp0 <= fp3)
            goto L_0003a860;
        fp0 = i960_u32_to_f64(r10);
        if (fp0 > 0.0)
            goto L_0003a864;

    L_0003a860:
        scr_st(scratch, (u32)g14);

    L_0003a864:
        g2 = cam_ld(cam, 0x7c);
        fp2 = i960_u32_to_f64(g2);
        fp0 = i960_u32_to_f64(g2);
        g2 = 0;
        g3 = 0x40b77000u;
        fp3 = i960_rifl_read(g2, g3);
        if (fp2 >= fp3)
            goto L_0003a8e0;
        fp0 = i960_u32_to_f64(r5);
        fp3 = 0.8;
        if (fp0 <= fp3)
            goto L_0003a8e0;
        g2 = 0;
        g3 = 0x40b77000u;
        fp1 = i960_rifl_read(g2, g3);
        i960_rifl_write(&g2, &g3, (fp1) - (fp2));
        fp0 = i960_rifl_read(g2, g3);
        i960_rifl_write(&g2, &g3, (fp0) / (fp1));
        fp0 = i960_rifl_read(g2, g3);
        i960_rifl_write(&g2, &g3, (1.0) - (fp0));
        fp0 = i960_rifl_read(g2, g3);
        g4 = i960_f64_to_u32(fp0);
        i960_st_u32(I960_WORKRAM, 0x2142f4, 0, (u32)g4);
        goto L_0003a8f0;

    L_0003a8e0:
        fp3 = 1.0;
        g3 = i960_f64_to_u32(fp3);
        i960_st_u32(I960_WORKRAM, 0x2142f4, 0, (u32)g3);

    L_0003a8f0:
        r8 = scr_ld(scratch);
        i960_st_u32(I960_WORKRAM, 0x2142f0, 0, (u32)r8);
        r4 = cam + 0x7cu;
        g0 = cam_ld(cam, 0x7cu);
        g1 = r5;
        g0 = geo_view_angle_frac((u32)g0, (u32)g1, (u32)g2);
    r5 = g0;
    {
        matrix_prep_local_regs caller_r = save_matrix_prep_locals();

        geo_view_matrix_mode_gate((u32)r5, (u32)g1, (u32)g2);
        restore_matrix_prep_locals(caller_r);
    }
    g4 = i960_ld_u32(I960_WORKRAM, 0x5d9308, 0);
    g4 = i960_f64_to_u32((i960_u32_to_f64(r5)) - (i960_u32_to_f64(g4)));
    g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) / (i960_u32_to_f64(r11)));
    g5 = cam_ld(cam, 0x7cu);
    g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) + (i960_u32_to_f64(g4)));
    fp0 = i960_u32_to_f64(g4);
    fp1 = i960_u32_to_f64(g4);
    if (fp0 >= 0.0)
        goto L_0003a944;
    fp0 = 0.0;
    goto L_0003a94c;

    L_0003a944:
        g2 = i960_f64_to_u32(fp1);
        fp0 = i960_u32_to_f64(g2);

    L_0003a94c:
        g2 = 0;
        g3 = 0x40c13000u;
        fp3 = i960_rifl_read(g2, g3);
        if (fp0 >= fp3)
            goto L_0003a984;
        g2 = i960_f64_to_u32(fp1);
        fp0 = i960_u32_to_f64(g2);
        if (fp0 >= 0.0)
            goto L_0003a97c;
        g5 = 0;
        goto L_0003a98c;

    L_0003a97c:
        g5 = i960_f64_to_u32(fp1);
        goto L_0003a98c;

    L_0003a984:
        g5 = 0x46098000u;

    L_0003a98c:
        g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) - (i960_u32_to_f64(r7)));
        i960_st_u32(I960_WORKRAM, 0x2142e8, 0, (u32)g5);
        r5 = i960_f64_to_u32((i960_u32_to_f64(r11)) * (i960_u32_to_f64(g4)));
        if (r9 == 0)
            goto L_0003a9e8;
        g4 = i960_ld_u32(I960_WORKRAM, 0x214120, 0);
        if (1 < (signed char)g4)
            goto L_0003a9e8;
        g4 = i960_ld_u32(I960_WORKRAM, 0x2142ec, 0);
        r5 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(r5)));
        fp0 = i960_u32_to_f64(r5);
        fp0 = i960_u32_to_f64(r5);
        fp1 = 0.3;
        i960_rifl_write(&g2, &g3, (fp0) / (fp1));
        fp0 = i960_rifl_read(g2, g3);
        r4 = i960_f64_to_u32(fp0);
        goto L_0003a9ec;

    L_0003a9e8:
        r4 = 0;

    L_0003a9ec:
        g0 = geo_view_control_byte_apply(cam, (u32)g1, (u32)g2);
    /* movr g0,fp0 — control_byte return bits as float, not cam VA. */
    fp1 = i960_u32_to_f64(r4);
    fp0 = i960_u32_to_f64((u32)g0);
    if (i960_u32_to_f64(r10) < i960_u32_to_f64(+0.0)) {
        i960_rifl_write(&g2, &g3, fp0);
        g3 = g3 ^ (1u << 31);
        fp0 = i960_rifl_read(g2, g3);
    }
    i960_rifl_write(&g2, &g3, (fp1) - (fp0));
    fp0 = i960_rifl_read(g2, g3);
    r4 = i960_f64_to_u32(fp0);
    {
        static unsigned force_log;

        force_log++;
        if ((force_log % 60u) == 1u) {
            fprintf(stderr,
                    "lift: matrix_prep drive=%.3g brake=%.3g net=%.3g "
                    "needle=%.3g r7=%.3g r9=%u r10=%.3g "
                    "an=0x%02x/%02x/%02x 214120=%d\n",
                    fp1, i960_u32_to_f64((u32)g0), fp0,
                    i960_u32_to_f64(cam_ld(cam, 0x7c)),
                    i960_u32_to_f64(r7), (unsigned)r9,
                    i960_u32_to_f64(r10),
                    (unsigned)i960_ld_u8(I960_WORKRAM, 0x202050, 0),
                    (unsigned)i960_ld_u8(I960_WORKRAM, 0x202051, 0),
                    (unsigned)i960_ld_u8(I960_WORKRAM, 0x202052, 0),
                    (int)(i32)i960_ld_u32(I960_WORKRAM, 0x214120, 0));
            fflush(stderr);
        }
    }
    g0 = r4;
    sp = sp_save;
    fp = fp_save;
    return (u32)g0;
}
