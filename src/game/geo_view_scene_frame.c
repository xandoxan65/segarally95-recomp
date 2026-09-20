/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_034f40_22b4.asm */
// @rom 0x34f40 +0x22b4 geo_view_scene_frame

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "i960_host.h"
#include "model2_rom.h" /* model2_ram_mut for gw32 */

#include "lift_syms.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* Guest VA → host word/halfword (pool node, workram, crx). */
static u32 *gw32(u32 va)
{
    u8 *p = model2_ram_mut(va);
    static u32 zero;
    return p ? (u32 *)(void *)p : &zero;
}

static u16 *gw16(u32 va)
{
    u8 *p = model2_ram_mut(va);
    static u16 zero;
    return p ? (u16 *)(void *)p : &zero;
}

/* i960 invalid-op leaves dest; host IEEE would store NaN into follow xyz. */
static u32 fadd_leave(u32 dest, u32 addend)
{
    double s = i960_u32_to_f64(dest) + i960_u32_to_f64(addend);

    if (!isfinite(s))
        return dest;
    return (u32)i960_f64_to_u32(s);
}

static u32 fdiv_leave(u32 dest, u32 num, u32 den)
{
    double s = i960_u32_to_f64(num) / i960_u32_to_f64(den);

    if (!isfinite(s))
        return dest;
    return (u32)i960_f64_to_u32(s);
}

typedef struct {
    uintptr_t v[16];
    uintptr_t g14v;
} scene_local_regs;

static scene_local_regs save_scene_local_regs(void)
{
    scene_local_regs s = {{
        r0, r1, r2, r3, r4, r5, r6, r7,
        r8, r9, r10, r11, r12, r13, r14, r15,
    }, g14};

    return s;
}

static void restore_scene_local_regs(scene_local_regs s)
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
    /* i960 call/ret restores caller g14. Host C helpers leave the link. */
    g14 = s.g14v;
}


/* convention: kind=leaf_ret  args g0  link g14 ret */
/* abi: void * arg0=g0 → void  (large 0x1f0 frame; calls geo_view_frame_update) */

void geo_view_scene_frame(void * arg0, void * arg1, void * arg2)
{
    /*
     * List-walk callx passes the pool node guest VA in g0 (e.g. 0x500000).
     * Use a private host frame — auto-lift's fp=sp leaked into the caller.
     */
    u32 arg0_p = (u32)(uintptr_t)arg0;
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    u32 save_g8 = (u32)g8;
    u32 save_g9 = (u32)g9;
    u32 save_g10 = (u32)g10;
    u32 save_g11 = (u32)g11;
    u32 save_g12 = (u32)g12;
    u8 frame[0x220];
    u8 *node;
    /*
     * Host EA of fp+0x170. Disasm spills lda 0x170(fp) into 0x1e0(fp) then
     * st (g8) on the velocity-reproject path (@ 0x363A4 / 0x36410). A u32
     * spill truncates the private-frame pointer; *gw32 then misses X so
     * fp+0x170 stays at the first 0x2c readback. Frame 1 skips that block
     * (fp3==0); Frame 2 enters it and poisons fp150/fp158 → follow yaw.
     */
    uintptr_t fp170_host = 0;

    (void)arg1;
    (void)arg2;

    node = model2_ram_mut(arg0_p);
    if (!node) {
        g8 = save_g8;
        g9 = save_g9;
        g10 = save_g10;
        g11 = save_g11;
        g12 = save_g12;
        sp = sp_save;
        fp = fp_save;
        return;
    }

    /*
     * Callx leaves g14 as the link; many paths then `st g14` for 0 (vel
     * clears @ 0x36D90, fp temps, 0x2139d8). A dirty g14 from a prior list
     * node turns those into nonzero writes — Y still freefalls from gravity
     * while X/Z/angle temps poison frame_update rates (post-countdown pan).
     */
    g14 = 0;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    *(u32 *)(frame + 0x1f0) = arg0_p;
    g6 = arg0_p + 16;
    g5 = i960_ld_u8(I960_ABS, 0x2139d8, 0);
    g4 = i960_ld_u8(I960_ABS, (u32)g6, 0);
    g9 = 0x7f;
    g5 = g5 << 7;
    g4 = g9 & g4;
    g4 = g4 | g5;
    i960_st_u8(I960_ABS, (u32)g6, 0, (u8)g4);
    g4 = i960_ld_u32(I960_ABS, 0x2020a8, 0);
    r4 = arg0_p + 0x74;
    if ((unsigned char)g4 == 9)
        goto L_00034f94;
    {
        scene_local_regs caller_r = save_scene_local_regs();

        geo_view_pitch_smooth((u32)g0, (u32)g1, (u32)g2);
        restore_scene_local_regs(caller_r);
    }
    goto L_00034f98;

    L_00034f94:
        g0 = 0;

    L_00034f98:
        i960_st_u32(I960_ABS, (u32)r4, 0, (u32)g0);
        g4 = i960_ld_u32(I960_ABS, 0x20a560, 0);
        /* @0x34FA4: cmpibge 0,g4,0x34fb4 — Intel src1>=src2 → skip when timer <= 0. */
        if ((i32)g4 <= 0)
            goto L_00034fb4;
        g10 = *(u32 *)(frame + 0x1f0);
        g0 = i960_ld_u32(I960_ABS, g10 + 0x8cu, 0);
        {
            scene_local_regs caller_r = save_scene_local_regs();

            geo_view_object_bind((void *)(uintptr_t)g0, (u32)g1, (u32)g2);
            restore_scene_local_regs(caller_r);
        }

    L_00034fb4:
        g11 = *(u32 *)(frame + 0x1f0);
        r4 = g11 + 0x88;
        g0 = g11 + 20;
        g1 = r4;
        {
            scene_local_regs caller_r = save_scene_local_regs();

            g0 = geo_view_slot_word_pack((u32)g0, (u32)g1, (u32)g2);
            restore_scene_local_regs(caller_r);
        }
    g4 = i960_ld_u16(I960_ABS, (u32)r4, 0);
    if ((int)(short)(u16)g4 < 0)
        i960_st_u16(I960_ABS, (u32)r4, 0, (u16)g14);
    g4 = i960_ld_u16(I960_ABS, (u32)r4, 0);
    g12 = *(u32 *)(frame + 0x1f0);
    i960_st_u32(I960_ABS, 0x2140c8, 0, (u32)g4);
    g4 = i960_ld_u16(I960_ABS, g12 + 0x8au, 0);
    i960_st_u32(I960_ABS, 0x2140c4, 0, (u32)g4);
    g4 = i960_ld_u32(I960_ABS, g12 + 0x24u, 0);
    if (i960_u32_to_f64(g4) < i960_u32_to_f64(+0.0)) {
        g4 = g4 ^ (1u << 31);
    }
    fp0 = i960_u32_to_f64(g4);
    fp2 = 0.1;
    if (fp0 <= fp2)
        goto L_00035048;
    g8 = *(u32 *)(frame + 0x1f0);
    g5 = g8 + 0x24;
    g4 = 0x3dcccccdu;
    if (i960_u32_to_f64(i960_ld_u32(I960_ABS, (u32)g5, 0))
        < i960_u32_to_f64(+0.0)) {
        g4 = 0xbdcccccdu;
    }
    i960_st_u32(I960_ABS, (u32)g5, 0, (u32)g4);

    L_00035048:
        g9 = 0x2c005858u;
        i960_mmio_write_u32(0x884000, (u32)g9); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
        g10 = *(u32 *)(fp + 0x1f0);
        i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
        g1 = g10 + 0x20;
        g4 = *gw32((u32)(uintptr_t)(g1));
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        g2 = g10 + 0x24;
        g4 = *gw32((u32)(uintptr_t)(g2));
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        g0 = g10 + 0x28;
        g4 = *gw32((u32)(uintptr_t)(g0));
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        g11 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        fp0 = i960_u32_to_f64(g11);
        fp0 = i960_u32_to_f64(g11);
        fp2 = i960_rifl_read(0xd2f1a9fcu, 0x3f60624du);
        if (fp0 <= fp2)
            goto L_000351a8;
        g8 = 15;
        i960_st_u32(I960_WORKRAM, 0x213974, 0, (u32)g8);
        g4 = *gw32((u32)(uintptr_t)(g1));
        g7 = 0x3f19999au;
        g4 = i960_f64_to_u32((i960_u32_to_f64(g7)) * (i960_u32_to_f64(g4)));
        g6 = g10 + 0x2c;
        g5 = *gw32((u32)(uintptr_t)(g6));
        g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) + (i960_u32_to_f64(g4)));
        *gw32((u32)(uintptr_t)(g6)) = (u32)g4;
        g4 = *gw32((u32)(uintptr_t)(g0));
        g4 = i960_f64_to_u32((i960_u32_to_f64(g7)) * (i960_u32_to_f64(g4)));
        g6 = g10 + 0x34;
        g5 = *gw32((u32)(uintptr_t)(g6));
        g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) + (i960_u32_to_f64(g4)));
        *gw32((u32)(uintptr_t)(g6)) = (u32)g4;
        g9 = *gw32((u32)(uintptr_t)(g1));
        fp0 = i960_u32_to_f64(g9);
        fp0 = i960_u32_to_f64(g9);
        g10 = 0 | (1u << 31);
        g11 = 0x3fd99999u;
        fp1 = i960_rifl_read(g10, g11);
        i960_rifl_write(&r14, &r15, (fp1) * (fp0));
        fp0 = i960_rifl_read(r14, r15);
        g4 = i960_f64_to_u32(fp0);
        *gw32((u32)(uintptr_t)(g1)) = (u32)g4;
        r15 = *gw32((u32)(uintptr_t)(g0));
        fp0 = i960_u32_to_f64(r15);
        fp0 = i960_u32_to_f64(r15);
        i960_rifl_write(&g8, &g9, (fp1) * (fp0));
        fp0 = i960_rifl_read(g8, g9);
        g4 = i960_f64_to_u32(fp0);
        *gw32((u32)(uintptr_t)(g0)) = (u32)g4;
        g9 = *(u32 *)(fp + 0x1f0);
        g5 = *gw32((u32)(uintptr_t)(g1));
        g6 = g9 + 20;
        g4 = *gw32((u32)(uintptr_t)(g6));
        g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) + (i960_u32_to_f64(g4)));
        *gw32((u32)(uintptr_t)(g6)) = (u32)g4;
        g6 = g9 + 24;
        g4 = *gw32((u32)(uintptr_t)(g6));
        g5 = *gw32((u32)(uintptr_t)(g2));
        g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) + (i960_u32_to_f64(g4)));
        *gw32((u32)(uintptr_t)(g6)) = (u32)g4;
        g6 = g9 + 28;
        g4 = *gw32((u32)(uintptr_t)(g6));
        g5 = *gw32((u32)(uintptr_t)(g0));
        g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) + (i960_u32_to_f64(g4)));
        *gw32((u32)(uintptr_t)(g6)) = (u32)g4;
        *gw32((u32)(uintptr_t)(g1)) = (u32)g14;
        *gw32((u32)(uintptr_t)(g2)) = (u32)g14;
        *gw32((u32)(uintptr_t)(g0)) = (u32)g14;

    L_000351a8:
        g10 = *(u32 *)(fp + 0x1f0);
        g4 = *gw32((u32)(uintptr_t)(g10 + 0x14));
        g5 = *gw32((u32)(uintptr_t)(g10 + 0x2c));
        g4 = fadd_leave((u32)g4, (u32)g5);
        g1 = fp + 0x130;
        *(u32 *)(fp + 0x130) = (u32)g4;
        g4 = *gw32((u32)(uintptr_t)(g10 + 0x18));
        g5 = *gw32((u32)(uintptr_t)(g10 + 0x30));
        g4 = fadd_leave((u32)g4, (u32)g5);
        *(u32 *)(fp + 0x134) = (u32)g4;
        g4 = *gw32((u32)(uintptr_t)(g10 + 0x1c));
        g5 = *gw32((u32)(uintptr_t)(g10 + 0x34));
        g4 = fadd_leave((u32)g4, (u32)g5);
        *(u32 *)(fp + 0x138) = (u32)g4;
        g4 = *gw32((u32)(uintptr_t)(g10 + 0x38));
        g5 = *gw32((u32)(uintptr_t)(g10 + 0x44));
        g4 = fadd_leave((u32)g4, (u32)g5);
        g2 = fp + 0x140;
        *(u32 *)(fp + 0x140) = (u32)g4;
        g4 = *gw32((u32)(uintptr_t)(g10 + 0x3c));
        g5 = *gw32((u32)(uintptr_t)(g10 + 0x48));
        g4 = fadd_leave((u32)g4, (u32)g5);
        *(u32 *)(fp + 0x144) = (u32)g4;
        g5 = *gw32((u32)(uintptr_t)(g10 + 0x40));
        g4 = *gw32((u32)(uintptr_t)(g10 + 0x4c));
        g5 = fadd_leave((u32)g5, (u32)g4);
        g0 = *(u32 *)(fp + 0x1f0);
        *(u32 *)(fp + 0x148) = (u32)g5;
        {
            scene_local_regs caller_r = save_scene_local_regs();

            /* ROM `st g14` at prep 0x48/213960 is 0 (cleared at frame entry). */
            g14 = 0;
            g0 = (uintptr_t)geo_view_scene_prep((void *)(uintptr_t)g0,
                                               (void *)(uintptr_t)g1,
                                               (void *)(uintptr_t)g2);
            restore_scene_local_regs(caller_r);
        }
    g11 = 0x10002020u;
    i960_mmio_write_u32(0x884000, (u32)g11); /* copro_fifo */;
    g12 = 0x12802525u;
    i960_mmio_write_u32(0x884000, (u32)g12); /* copro_fifo */;
    r14 = 0x13802727u;
    i960_mmio_write_u32(0x884000, (u32)r14); /* copro_fifo */;
    g4 = *(u32 *)(fp + 0x130);
    g5 = *(u32 *)(fp + 0x134);
    g6 = *(u32 *)(fp + 0x138);
    g7 = *(u32 *)(fp + 0x144);
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g6); /* copro_fifo */;
    r15 = 0x15002a2au;
    i960_mmio_write_u32(0x884000, (u32)r15); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g7); /* copro_fifo */;
    g8 = 0x14802929u;
    i960_mmio_write_u32(0x884000, (u32)g8); /* copro_fifo */;
    g4 = *(u32 *)(fp + 0x140);
    g5 = *(u32 *)(fp + 0x148);
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g9 = 0x15802b2bu;
    i960_mmio_write_u32(0x884000, (u32)g9); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
    g10 = 0x16002c2cu;
    i960_mmio_write_u32(0x884000, (u32)g10); /* copro_fifo */;
    g11 = 0xbf228f5cu;
    i960_mmio_write_u32(0x884000, (u32)g11); /* copro_fifo */;
    g12 = 0xbecccccdu;
    i960_mmio_write_u32(0x884000, (u32)g12); /* copro_fifo */;
    r14 = 0x3fa00000u;
    i960_mmio_write_u32(0x884000, (u32)r14); /* copro_fifo */;
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    g5 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    g6 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    r15 = g10;
    i960_mmio_write_u32(0x884000, (u32)r15); /* copro_fifo */;
    g8 = 0x3f228f5cu;
    i960_mmio_write_u32(0x884000, (u32)g8); /* copro_fifo */;
    g9 = 0xbecccccdu;
    i960_mmio_write_u32(0x884000, (u32)g9); /* copro_fifo */;
    g10 = 0x3fa00000u;
    i960_mmio_write_u32(0x884000, (u32)g10); /* copro_fifo */;
    *(u32 *)(fp + 0xf0) = (u32)g4;
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    *(u32 *)(fp + 0xf4) = (u32)g5;
    *(u32 *)(fp + 0xf8) = (u32)g6;
    *(u32 *)(fp + 0xfc) = (u32)g4;
    g3 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    g13 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    g11 = r15;
    i960_mmio_write_u32(0x884000, (u32)g11); /* copro_fifo */;
    g12 = 0xbf28f5c3u;
    i960_mmio_write_u32(0x884000, (u32)g12); /* copro_fifo */;
    r14 = 0xbecccccdu;
    i960_mmio_write_u32(0x884000, (u32)r14); /* copro_fifo */;
    r15 = 0xbfa00000u;
    i960_mmio_write_u32(0x884000, (u32)r15); /* copro_fifo */;
    g7 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    g1 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    g2 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    g8 = g11;
    i960_mmio_write_u32(0x884000, (u32)g8); /* copro_fifo */;
    g9 = 0x3f28f5c3u;
    i960_mmio_write_u32(0x884000, (u32)g9); /* copro_fifo */;
    g10 = 0xbecccccdu;
    i960_mmio_write_u32(0x884000, (u32)g10); /* copro_fifo */;
    g11 = 0xbfa00000u;
    i960_mmio_write_u32(0x884000, (u32)g11); /* copro_fifo */;
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    g5 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    g6 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    g12 = 0x10802121u;
    i960_mmio_write_u32(0x884000, (u32)g12); /* copro_fifo */;
    r14 = 0x10002020u;
    i960_mmio_write_u32(0x884000, (u32)r14); /* copro_fifo */;
    r15 = 0x12802525u;
    i960_mmio_write_u32(0x884000, (u32)r15); /* copro_fifo */;
    *(u32 *)(fp + 0x100) = (u32)g3;
    *(u32 *)(fp + 0x104) = (u32)g13;
    *(u32 *)(fp + 0x108) = (u32)g7;
    *(u32 *)(fp + 0x10c) = (u32)g1;
    *(u32 *)(fp + 0x110) = (u32)g2;
    *(u32 *)(fp + 0x114) = (u32)g4;
    *(u32 *)(fp + 0x118) = (u32)g5;
    *(u32 *)(fp + 0x11c) = (u32)g6;
    /* ROM `st g14` — caller g14 is 0; host TGP/helpers may have left a link. */
    g14 = 0;
    *(u32 *)(fp + 0x150) = (u32)g14;
    *(u32 *)(fp + 0x154) = (u32)g14;
    *(u32 *)(fp + 0x158) = (u32)g14;
    *(u32 *)(fp + 0x120) = (u32)g14;
    *(u32 *)(fp + 0x124) = (u32)g14;
    g8 = *(u32 *)(fp + 0x1f0);
    *(u32 *)(fp + 0x128) = (u32)g14;
    g4 = *gw32((u32)(uintptr_t)(g8 + 0x14));
    g5 = *gw32((u32)(uintptr_t)(g8 + 0x18));
    g6 = *gw32((u32)(uintptr_t)(g8 + 0x1c));
    g9 = 0x13802727u;
    i960_mmio_write_u32(0x884000, (u32)g9); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g6); /* copro_fifo */;
    g4 = *gw32((u32)(uintptr_t)(g8 + 0x3c));
    g10 = 0x15002a2au;
    i960_mmio_write_u32(0x884000, (u32)g10); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g4 = *gw32((u32)(uintptr_t)(g8 + 0x38));
    g11 = 0x14802929u;
    i960_mmio_write_u32(0x884000, (u32)g11); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g4 = *gw32((u32)(uintptr_t)(g8 + 0x40));
    g12 = 0x15802b2bu;
    i960_mmio_write_u32(0x884000, (u32)g12); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    r14 = 0x16002c2cu;
    i960_mmio_write_u32(0x884000, (u32)r14); /* copro_fifo */;
    r15 = 0xbf228f5cu;
    i960_mmio_write_u32(0x884000, (u32)r15); /* copro_fifo */;
    g8 = 0xbecccccdu;
    i960_mmio_write_u32(0x884000, (u32)g8); /* copro_fifo */;
    g9 = 0x3fa00000u;
    i960_mmio_write_u32(0x884000, (u32)g9); /* copro_fifo */;
    g7 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    g1 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    g2 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    g10 = r14;
    i960_mmio_write_u32(0x884000, (u32)g10); /* copro_fifo */;
    g11 = 0x3f228f5cu;
    i960_mmio_write_u32(0x884000, (u32)g11); /* copro_fifo */;
    g12 = 0xbecccccdu;
    i960_mmio_write_u32(0x884000, (u32)g12); /* copro_fifo */;
    r14 = 0x3fa00000u;
    i960_mmio_write_u32(0x884000, (u32)r14); /* copro_fifo */;
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    g5 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    g6 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    r15 = g10;
    i960_mmio_write_u32(0x884000, (u32)r15); /* copro_fifo */;
    *(u32 *)(fp + 0x40) = (u32)g7;
    *(u32 *)(fp + 0x44) = (u32)g1;
    *(u32 *)(fp + 0x48) = (u32)g2;
    *(u32 *)(fp + 0x4c) = (u32)g4;
    *(u32 *)(fp + 0x50) = (u32)g5;
    *(u32 *)(fp + 0x54) = (u32)g6;
    g8 = 0xbf28f5c3u;
    i960_mmio_write_u32(0x884000, (u32)g8); /* copro_fifo */;
    g9 = 0xbecccccdu;
    i960_mmio_write_u32(0x884000, (u32)g9); /* copro_fifo */;
    g10 = 0xbfa00000u;
    i960_mmio_write_u32(0x884000, (u32)g10); /* copro_fifo */;
    g7 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    g1 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    g2 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    g11 = r15;
    i960_mmio_write_u32(0x884000, (u32)g11); /* copro_fifo */;
    g12 = 0x3f28f5c3u;
    i960_mmio_write_u32(0x884000, (u32)g12); /* copro_fifo */;
    r14 = 0xbecccccdu;
    i960_mmio_write_u32(0x884000, (u32)r14); /* copro_fifo */;
    r15 = 0xbfa00000u;
    i960_mmio_write_u32(0x884000, (u32)r15); /* copro_fifo */;
    g3 = i960_ld_u32(I960_WORKRAM, 0x213960, 0);
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    g5 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    g6 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    g8 = 0x10802121u;
    i960_mmio_write_u32(0x884000, (u32)g8); /* copro_fifo */;
    *(u32 *)(fp + 0x58) = (u32)g7;
    *(u32 *)(fp + 0x5c) = (u32)g1;
    *(u32 *)(fp + 0x60) = (u32)g2;
    *(u32 *)(fp + 0x64) = (u32)g4;
    *(u32 *)(fp + 0x68) = (u32)g5;
    *(u32 *)(fp + 0x6c) = (u32)g6;
    *(u32 *)(fp + 0x1a0) = (u32)g0;
    if (g3 == 0)
        goto L_00035738;

    r5 = 0x2138d0u;
    r6 = 0;
    g9 = 0x21393cu;

    L_000356d8:
        g4 = i960_ld_u8(I960_WORKRAM, 0x213960, 0);
        if (g4 & 1u) {
            u8 *xyz = frame + 0xf0 + (u32)r6;

            g1 = i960_ld_u32(I960_WORKRAM, 0x2140c8, 0);
            g0 = i960_host_race_course_index();
            g3 = 0;
            /*
             * Disasm 0x356F4 forms a private-frame EA.  Keep that host
             * pointer full-width; r5 remains the guest out VA.
             */
            {
                scene_local_regs caller_r = save_scene_local_regs();

                geo_view_table_index_a((void *)(uintptr_t)g0, (u32)g1, xyz);
                restore_scene_local_regs(caller_r);
            }
            {
                scene_local_regs caller_r = save_scene_local_regs();

                geo_view_table_index_b(xyz, (void *)(uintptr_t)r5, 0);
                restore_scene_local_regs(caller_r);
            }
        }

        g4 = i960_ld_u32(I960_WORKRAM, 0x213960, 0);
        r5 = r5 + 0x24;
        r6 = r6 + 12;
        g4 = (uintptr_t)((i32)g4 >> 1);
        i960_st_u32(I960_WORKRAM, 0x213960, 0, (u32)g4);
        if (r5 <= g9)
            goto L_000356d8;

    L_00035738:
        g7 = *(u32 *)(fp + 0xf0);
        g4 = *(u32 *)(fp + 0x40);
        g0 = *(u32 *)(fp + 0xf4);
        r7 = *(u32 *)(fp + 0x44);
        g1 = *(u32 *)(fp + 0xf8);
        r9 = *(u32 *)(fp + 0x48);
        g2 = *(u32 *)(fp + 0xfc);
        r10 = *(u32 *)(fp + 0x4c);
        g3 = *(u32 *)(fp + 0x100);
        r11 = *(u32 *)(fp + 0x50);
        g13 = *(u32 *)(fp + 0x104);
        r12 = *(u32 *)(fp + 0x54);
        r4 = *(u32 *)(fp + 0x108);
        r5 = *(u32 *)(fp + 0x10c);
        r6 = *(u32 *)(fp + 0x110);
        g6 = *(u32 *)(fp + 0x114);
        g5 = *(u32 *)(fp + 0x118);
        r8 = *(u32 *)(fp + 0x68);
        g7 = i960_f64_to_u32((i960_u32_to_f64(g7)) - (i960_u32_to_f64(g4)));
        g4 = *(u32 *)(fp + 0x11c);
        g0 = i960_f64_to_u32((i960_u32_to_f64(g0)) - (i960_u32_to_f64(r7)));
        r7 = *(u32 *)(fp + 0x6c);
        g10 = *(u32 *)(fp + 0x58);
        g12 = *(u32 *)(fp + 0x5c);
        r14 = *(u32 *)(fp + 0x60);
        g1 = i960_f64_to_u32((i960_u32_to_f64(g1)) - (i960_u32_to_f64(r9)));
        g8 = *(u32 *)(fp + 0x64);
        g2 = i960_f64_to_u32((i960_u32_to_f64(g2)) - (i960_u32_to_f64(r10)));
        *(u32 *)(fp + 0x40) = (u32)g7;
        g3 = i960_f64_to_u32((i960_u32_to_f64(g3)) - (i960_u32_to_f64(r11)));
        *(u32 *)(fp + 0x44) = (u32)g0;
        g13 = i960_f64_to_u32((i960_u32_to_f64(g13)) - (i960_u32_to_f64(r12)));
        *(u32 *)(fp + 0x48) = (u32)g1;
        g11 = g10;
        r4 = i960_f64_to_u32((i960_u32_to_f64(r4)) - (i960_u32_to_f64(g11)));
        *(u32 *)(fp + 0x4c) = (u32)g2;
        fp3 = i960_u32_to_f64(g12);
        r5 = i960_f64_to_u32((i960_u32_to_f64(r5)) - (fp3));
        *(u32 *)(fp + 0x50) = (u32)g3;
        r15 = r14;
        r6 = i960_f64_to_u32((i960_u32_to_f64(r6)) - (i960_u32_to_f64(r15)));
        *(u32 *)(fp + 0x54) = (u32)g13;
        g9 = g8;
        g6 = i960_f64_to_u32((i960_u32_to_f64(g6)) - (i960_u32_to_f64(g9)));
        *(u32 *)(fp + 0x58) = (u32)r4;
        *(u32 *)(fp + 0x5c) = (u32)r5;
        g5 = i960_f64_to_u32((i960_u32_to_f64(g5)) - (i960_u32_to_f64(r8)));
        *(u32 *)(fp + 0x60) = (u32)r6;
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) - (i960_u32_to_f64(r7)));
        *(u32 *)(fp + 0x64) = (u32)g6;
        g11 = *(u32 *)(fp + 0x1f0);
        *(u32 *)(fp + 0x68) = (u32)g5;
        *(u32 *)(fp + 0x6c) = (u32)g4;
        g10 = 0x10002020u;
        i960_mmio_write_u32(0x884000, (u32)g10); /* copro_fifo */;
        g12 = 0x17802f2fu;
        i960_mmio_write_u32(0x884000, (u32)g12); /* copro_fifo */;
        g1 = g11 + 0x2c;
        g4 = *gw32((u32)(uintptr_t)(g1));
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        g0 = g11 + 0x30;
        g4 = *gw32((u32)(uintptr_t)(g0));
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        g7 = g11 + 0x34;
        g4 = *gw32((u32)(uintptr_t)(g7));
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        g13 = fp + 0x160;
        *(u32 *)(fp + 0x160) = (u32)g4;
        g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        g3 = fp + 0x164;
        *(u32 *)(fp + 0x164) = (u32)g4;
        g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        g2 = fp + 0x168;
        *(u32 *)(fp + 0x168) = (u32)g4;
        r14 = 0x12802525u;
        i960_mmio_write_u32(0x884000, (u32)r14); /* copro_fifo */;
        g4 = *gw32((u32)(uintptr_t)(g11 + 0x3c));
        r15 = 0x15002a2au;
        i960_mmio_write_u32(0x884000, (u32)r15); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        g8 = 0x16002c2cu;
        i960_mmio_write_u32(0x884000, (u32)g8); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
        fp3 = 1.0;
        g9 = i960_f64_to_u32(fp3);
        i960_mmio_write_u32(0x884000, (u32)g9); /* copro_fifo */;
        g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        g5 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        g6 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        r14 = g8 << 1;
        i960_mmio_write_u32(0x884000, (u32)r14); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
        *(u32 *)(fp + 0xac) = (u32)g4;
        *(u32 *)(fp + 0xb0) = (u32)g5;
        *(u32 *)(fp + 0xb4) = (u32)g6;
        i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
        g4 = *gw32((u32)(uintptr_t)(g1));
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        g4 = *gw32((u32)(uintptr_t)(g0));
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        g4 = *gw32((u32)(uintptr_t)(g7));
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        r15 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        fp0 = i960_u32_to_f64(r15);
        fp0 = i960_u32_to_f64(r15);
        fp3 = 0.01;
        if (fp0 <= fp3)
            goto L_00035ac4;
        g4 = *(u32 *)(fp + 0xb0);
        g5 = *(u32 *)(fp + 0x168);
        g6 = *(u32 *)(fp + 0xb4);
        g7 = *(u32 *)(fp + 0x164);
        g0 = *(u32 *)(fp + 0x160);
        r14 = 0x2d805b5bu;
        i960_mmio_write_u32(0x884000, (u32)r14); /* copro_fifo */;
        g1 = *(u32 *)(fp + 0xac);
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g6); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g7); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g0); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g1); /* copro_fifo */;
        g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        *(u32 *)(fp + 0x160) = (u32)g4;
        g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        *(u32 *)(fp + 0x164) = (u32)g4;
        g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        *(u32 *)(fp + 0x168) = (u32)g4;
        r15 = *(u32 *)(fp + 0x164);
        /* Disasm @ 0x35A00–0x35A0C: cpysre fp0,+0.0,fp0 → |r15|; movr r15,fp1. */
        fp0 = fabs(i960_u32_to_f64(r15));
        fp1 = i960_u32_to_f64(r15);
        if (fp0 <= 1.0)
            goto L_00035a1c;
        g5 = 0;
        goto L_00035a54;

    L_00035a1c:
        g4 = i960_f64_to_u32((fp1) * (fp1));
        g4 = i960_f64_to_u32((1.0) - (i960_u32_to_f64(g4)));
        g4 = i960_f64_to_u32(sqrt(i960_u32_to_f64(g4)));
        if (i960_u32_to_f64(g4) == i960_u32_to_f64(+0.0))
            goto L_00035a3c;
        /* divr g4,fp1,g4 → fp1/g4; atanr +1.0,g4,g5 → atan2(g4, 1.0). */
        g4 = i960_f64_to_u32((fp1) / (i960_u32_to_f64(g4)));
        g5 = i960_f64_to_u32(atan(i960_u32_to_f64(g4)));
        goto L_00035a54;

    L_00035a3c:
        g5 = 0xbfc90fdbu;
        if (fp1 > 0.0) {
            g5 = 0x3fc90fdbu;
        }

    L_00035a54:
        fp0 = i960_u32_to_f64(g5);
        fp3 = 1.5707963267948966;
        if (fp0 <= fp3)
            goto L_00035a80;
        g5 = 0x3fc90fdbu;
        goto L_00035aa4;

    L_00035a80:
        fp2 = i960_rifl_read(0x54442d18u, 0xbff921fbu);
        if (fp0 < fp2) {
            g5 = 0xbfc90fdbu;
        }

    L_00035aa4:
        g4 = i960_ld_u32(I960_WORKRAM, 0x214200, 0);
        g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) - (i960_u32_to_f64(g4)));
        i960_st_u32(I960_WORKRAM, 0x214200, 0, (u32)g5);
        i960_st_u32(I960_WORKRAM, 0x214204, 0, (u32)g4);
        goto L_00035ad4;

    L_00035ac4:
        i960_st_u32(I960_WORKRAM, 0x214204, 0, (u32)g14);
        i960_st_u32(I960_WORKRAM, 0x214200, 0, (u32)g14);

    L_00035ad4:
        g8 = *(u32 *)(fp + 0x1f0);
        g4 = *gw32((u32)(uintptr_t)(g8 + 0x74));
        g9 = 0x15002a2au;
        i960_mmio_write_u32(0x884000, (u32)g9); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        g10 = 0x16002c2cu;
        i960_mmio_write_u32(0x884000, (u32)g10); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
        fp3 = 1.0;
        g11 = i960_f64_to_u32(fp3);
        i960_mmio_write_u32(0x884000, (u32)g11); /* copro_fifo */;
        g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        r4 = fp + 0xa0;
        *(u32 *)(fp + 0xa0) = (u32)g4;
        i960_st_u32(I960_WORKRAM, 0x214208, 0, (u32)g14);
        g13 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        g7 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        r14 = 0x10802121u;
        i960_mmio_write_u32(0x884000, (u32)r14); /* copro_fifo */;

        /* @0x35B5C: four geo_view_copro_vec_push with private-frame EAs. */
        g1 = fp + 0x40;
        g2 = fp + 0xf0;
        g3 = r4;
        g0 = *(u32 *)(fp + 0x1f0);
        g4 = 0x3e800000u;
        g5 = fp + 0x70;
        g6 = 0;
        *(u32 *)(fp + 0xa4) = (u32)g13;
        *(u32 *)(fp + 0xa8) = (u32)g7;
        {
            scene_local_regs caller_r = save_scene_local_regs();

            geo_view_copro_vec_push((u32)g0, (u32)g1, (u32)g2);
            restore_scene_local_regs(caller_r);
        }
    g3 = r4;
    g4 = 0x3e800000u;
    i960_st_u32(I960_WORKRAM, 0x213974, 0, (u32)g0);
    g0 = *(u32 *)(fp + 0x1f0);
    g6 = 1;
    g1 = fp + 0x4c;
    g2 = fp + 0xfc;
    g5 = fp + 0x7c;
    {
        scene_local_regs caller_r = save_scene_local_regs();

        geo_view_copro_vec_push((u32)g0, (u32)g1, (u32)g2);
        restore_scene_local_regs(caller_r);
    }
    g4 = i960_ld_u32(I960_WORKRAM, 0x213974, 0);
    g5 = g4 + (u32)(g0 << 1);
    g4 = 0x3e800000u;
    g6 = 2;
    r4 = fp + 0xac;
    g0 = *(u32 *)(fp + 0x1f0);
    g1 = fp + 0x58;
    g2 = fp + 0x108;
    i960_st_u32(I960_WORKRAM, 0x213974, 0, (u32)g5);
    g3 = r4;
    g5 = fp + 0x88;
    {
        scene_local_regs caller_r = save_scene_local_regs();

        geo_view_copro_vec_push((u32)g0, (u32)g1, (u32)g2);
        restore_scene_local_regs(caller_r);
    }
    g4 = i960_ld_u32(I960_WORKRAM, 0x213974, 0);
    g5 = g4 + (u32)(g0 << 2);
    g4 = 0x3e800000u;
    g6 = 3;
    g0 = *(u32 *)(fp + 0x1f0);
    g3 = r4;
    g1 = fp + 0x64;
    i960_st_u32(I960_WORKRAM, 0x213974, 0, (u32)g5);
    g2 = fp + 0x114;
    g5 = fp + 0x94;
    {
        scene_local_regs caller_r = save_scene_local_regs();

        geo_view_copro_vec_push((u32)g0, (u32)g1, (u32)g2);
        restore_scene_local_regs(caller_r);
    }
    g4 = i960_ld_u8(I960_WORKRAM, 0x214208, 0);
    g5 = i960_ld_u32(I960_WORKRAM, 0x213974, 0);
    r15 = 31 + 17;
    g4 = r15 & g4;
    g0 = g5 + (u32)(g0 << 3);
    i960_st_u32(I960_WORKRAM, 0x213974, 0, (u32)g0);
    if (g4 != 0)
        goto L_00035c68;
    i960_st_u32(I960_WORKRAM, 0x2138c0, 0, (u32)g14);
    goto L_00035c88;

    L_00035c60:
        g7 = 1;
        goto L_00035cac;

    L_00035c68:
        g4 = i960_ld_u32(I960_WORKRAM, 0x2138c0, 0);
        if ((unsigned char)g4 != 0)
            goto L_00035c88;
        g0 = 0x7b;
        {
            scene_local_regs caller_r = save_scene_local_regs();

            comm_palette_index_call((u32)g0);
            restore_scene_local_regs(caller_r);
        }
    g8 = 1;
    i960_st_u32(I960_WORKRAM, 0x2138c0, 0, (u32)g8);

    L_00035c88:
        g9 = *(u32 *)(fp + 0x1f0);
        g7 = 0;
        g5 = 0;
        g6 = *gw16((u32)(uintptr_t)(g9 + 0x12));

    L_00035c98:
        g4 = g6 & 15;
        if ((unsigned char)g4 == 6)
            goto L_00035c60;
        g6 = g6 >> 4;
        g5 = g5 + 0x1;
        if (3 >= (signed char)g5)
            goto L_00035c98;

    L_00035cac:
        g4 = i960_ld_u32(I960_WORKRAM, 0x21396c, 0);
        if ((unsigned char)g4 != 0)
            goto L_00035d00;
        if ((unsigned char)g7 == 0)
            goto L_00035ce4;
        g4 = i960_ld_u32(I960_WORKRAM, 0x2142d4, 0);
        if ((unsigned char)g4 != 0)
            goto L_00035ce4;
        g10 = 2;
        g11 = 5;
        i960_st_u32(I960_WORKRAM, 0x2142c8, 0, (u32)g10);
        i960_st_u32(I960_WORKRAM, 0x2142d0, 0, (u32)g11);
        goto L_00035d00;

    L_00035ce4:
        if ((unsigned char)g7 != 0)
            goto L_00035d00;
        g4 = i960_ld_u32(I960_WORKRAM, 0x2142d4, 0);
        if ((unsigned char)g4 != 0) {
            g12 = 3;
            i960_st_u32(I960_WORKRAM, 0x2142c8, 0, (u32)g12);
        }

    L_00035d00:
        g3 = i960_ld_u32(I960_WORKRAM, 0x213880, 0);
        g1 = i960_ld_u32(I960_WORKRAM, 0x213884, 0);
        g2 = i960_ld_u32(I960_WORKRAM, 0x213890, 0);
        g0 = i960_ld_u32(I960_WORKRAM, 0x213894, 0);
        g6 = i960_ld_u32(I960_WORKRAM, 0x213888, 0);
        g7 = i960_ld_u32(I960_WORKRAM, 0x21388c, 0);
        g5 = i960_ld_u32(I960_WORKRAM, 0x213898, 0);
        g3 = i960_f64_to_u32((i960_u32_to_f64(g1)) + (i960_u32_to_f64(g3)));
        g4 = i960_ld_u32(I960_WORKRAM, 0x21389c, 0);
        g2 = i960_f64_to_u32((i960_u32_to_f64(g0)) + (i960_u32_to_f64(g2)));
        g6 = i960_f64_to_u32((i960_u32_to_f64(g7)) + (i960_u32_to_f64(g6)));
        g5 = i960_f64_to_u32((i960_u32_to_f64(g4)) + (i960_u32_to_f64(g5)));
        g4 = i960_ld_u32(I960_WORKRAM, 0x213974, 0);
        i960_st_u32(I960_WORKRAM, 0x21420c, 0, (u32)g3);
        g4 = 3 & g4;
        i960_st_u32(I960_WORKRAM, 0x214210, 0, (u32)g2);
        i960_st_u32(I960_WORKRAM, 0x214214, 0, (u32)g6);
        i960_st_u32(I960_WORKRAM, 0x214218, 0, (u32)g5);
        if (g4 == 3) {
            fp0 = i960_u32_to_f64(g3);
            fp1 = i960_u32_to_f64(g2);
            r14 = 0;
            r15 = 0x3fe00000u;
            fp2 = i960_rifl_read(r14, r15);
            i960_rifl_write(&g8, &g9, (fp2) * (fp0));
            i960_rifl_write(&g10, &g11, (fp2) * (fp1));
            fp0 = i960_rifl_read(g8, g9);
            fp1 = i960_rifl_read(g10, g11);
            g4 = i960_f64_to_u32(fp0);
            g5 = i960_f64_to_u32(fp1);
            i960_st_u32(I960_WORKRAM, 0x21420c, 0, (u32)g4);
            i960_st_u32(I960_WORKRAM, 0x214210, 0, (u32)g5);
        }
    /* @0x35DC4: if (0x213974 & 12) == 12, half-scale 0x214214/218. */
    g4 = i960_ld_u32(I960_WORKRAM, 0x213974, 0);
    g4 = g4 & 12;
    if ((unsigned char)g4 == 12) {
        g11 = i960_ld_u32(I960_WORKRAM, 0x214214, 0);
        g12 = i960_ld_u32(I960_WORKRAM, 0x214218, 0);
        fp0 = i960_u32_to_f64(g11);
        fp1 = i960_u32_to_f64(g12);
        r14 = 0;
        r15 = 0x3fe00000u;
        fp2 = i960_rifl_read(r14, r15);
        i960_rifl_write(&g8, &g9, (fp2) * (fp0));
        i960_rifl_write(&g10, &g11, (fp2) * (fp1));
        fp0 = i960_rifl_read(g8, g9);
        fp1 = i960_rifl_read(g10, g11);
        g4 = i960_f64_to_u32(fp0);
        g5 = i960_f64_to_u32(fp1);
        i960_st_u32(I960_WORKRAM, 0x214214, 0, (u32)g4);
        i960_st_u32(I960_WORKRAM, 0x214218, 0, (u32)g5);
    }
    /* @0x35E2C: always — push negated node+0x40 / +0x38. */
    g11 = 0x10002020u;
    r14 = *(u32 *)(fp + 0x1f0);
    i960_mmio_write_u32(0x884000, (u32)g11); /* copro_fifo */;
    g12 = 0x12802525u;
    i960_mmio_write_u32(0x884000, (u32)g12); /* copro_fifo */;
    g4 = *gw32((u32)(uintptr_t)(r14 + 0x40));
    g4 = g4 ^ (1u << 31);
    r15 = 0x15802b2bu;
    i960_mmio_write_u32(0x884000, (u32)r15); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g4 = *gw32((u32)(uintptr_t)(r14 + 0x38));
    g4 = g4 ^ (1u << 31);
    g8 = 0x14802929u;
    i960_mmio_write_u32(0x884000, (u32)g8); /* copro_fifo */;
    r14 = r14 + 0x3c;
    *(u32 *)(fp + 0x1b0) = (u32)r14;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g4 = *gw32((u32)(uintptr_t)(r14));
    g4 = g4 ^ (1u << 31);
    g13 = *(u32 *)(fp + 0x70);
    r15 = 0x15002a2au;
    i960_mmio_write_u32(0x884000, (u32)r15); /* copro_fifo */;
    g2 = *(u32 *)(fp + 0x74);
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g8 = 0x16002c2cu;
    i960_mmio_write_u32(0x884000, (u32)g8); /* copro_fifo */;
    g3 = *(u32 *)(fp + 0x78);
    i960_mmio_write_u32(0x884000, (u32)g13); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g2); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g3); /* copro_fifo */;
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    g9 = fp + 0x170;
    fp170_host = (uintptr_t)g9;
    *(u32 *)(fp + 0x1e0) = (u32)g9; /* disasm spill; host uses fp170_host */
    *(u32 *)(fp + 0x170) = (u32)g4;
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    r3 = fp + 0x174;
    *(u32 *)(fp + 0x174) = (u32)g4;
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    r13 = fp + 0x178;
    *(u32 *)(fp + 0x178) = (u32)g4;
    g10 = g8;
    i960_mmio_write_u32(0x884000, (u32)g10); /* copro_fifo */;
    g11 = *(u32 *)(fp + 0x7c);
    g12 = *(u32 *)(fp + 0x80);
    r14 = *(u32 *)(fp + 0x84);
    i960_mmio_write_u32(0x884000, (u32)g11); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g12); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)r14); /* copro_fifo */;
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    *(u32 *)(fp + 0x17c) = (u32)g4;
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    *(u32 *)(fp + 0x180) = (u32)g4;
    r10 = *(u32 *)(fp + 0x174);
    r5 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    r15 = g10;
    i960_mmio_write_u32(0x884000, (u32)r15); /* copro_fifo */;
    g8 = *(u32 *)(fp + 0x88);
    g9 = *(u32 *)(fp + 0x8c);
    g10 = *(u32 *)(fp + 0x90);
    i960_mmio_write_u32(0x884000, (u32)g8); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g9); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g10); /* copro_fifo */;
    g7 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    r4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    g11 = r15;
    i960_mmio_write_u32(0x884000, (u32)g11); /* copro_fifo */;
    g12 = *(u32 *)(fp + 0x94);
    r14 = *(u32 *)(fp + 0x98);
    r15 = *(u32 *)(fp + 0x9c);
    i960_mmio_write_u32(0x884000, (u32)g12); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)r14); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)r15); /* copro_fifo */;
    g5 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    *(u32 *)(fp + 0x18c) = (u32)g4;
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    r8 = *(u32 *)(fp + 0x180);
    *(u32 *)(fp + 0x198) = (u32)g4;
    g0 = *(u32 *)(fp + 0x18c);
    g6 = *(u32 *)(fp + 0x198);
    g8 = i960_f64_to_u32((i960_u32_to_f64(r8)) + (i960_u32_to_f64(r10)));
    g9 = i960_f64_to_u32((i960_u32_to_f64(g6)) + (i960_u32_to_f64(g0)));
    fp1 = i960_u32_to_f64(g8);
    fp1 = i960_u32_to_f64(g8);
    fp0 = i960_u32_to_f64(g9);
    fp0 = i960_u32_to_f64(g9);
    i960_rifl_write(&g10, &g11, (fp1) - (fp0));
    fp2 = 1.35;
    fp1 = i960_rifl_read(g10, g11);
    i960_rifl_write(&r14, &r15, (fp2) * (fp1));
    r6 = *(u32 *)(fp + 0x150);
    fp3 = i960_u32_to_f64(r6);
    *(u32 *)(fp + 0x184) = (u32)r5;
    i960_rifl_write(&r6, &r7, fp3);
    fp1 = i960_rifl_read(r14, r15);
    *(u32 *)(fp + 0x188) = (u32)g7;
    i960_rifl_write(&r6, &r7, (fp1) + (i960_rifl_read(r6, r7)));
    *(u32 *)(fp + 0x190) = (u32)r4;
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    *(u32 *)(fp + 0x194) = (u32)g5;
    fp2 = i960_rifl_read(r6, r7);
    r6 = i960_f64_to_u32(fp2);
    *(u32 *)(fp + 0x19c) = (u32)g4;
    *(u32 *)(fp + 0x150) = (u32)r6;
    r9 = *(u32 *)(fp + 0x188);
    r12 = *(u32 *)(fp + 0x194);
    g8 = i960_f64_to_u32((i960_u32_to_f64(r12)) + (i960_u32_to_f64(r9)));
    g5 = *(u32 *)(fp + 0x170);
    g4 = *(u32 *)(fp + 0x17c);
    g5 = i960_f64_to_u32((i960_u32_to_f64(g4)) + (i960_u32_to_f64(g5)));
    r15 = g8;
    fp2 = i960_u32_to_f64(r15);
    r10 = i960_f64_to_u32((i960_u32_to_f64(r10)) - (i960_u32_to_f64(r8)));
    *(u32 *)(fp + 0x1d0) = (u32)g8;
    i960_rifl_write(&r14, &r15, fp2);
    *(u64 *)(fp + 0x1d0) = ((u64)r15 << 32) | (u32)r14;
    { u64 _p = *(u64 *)(fp + 0x1d0); r14 = (u32)_p; r15 = (u32)(_p >> 32); }
    fp0 = i960_u32_to_f64(g5);
    g0 = i960_f64_to_u32((i960_u32_to_f64(g0)) - (i960_u32_to_f64(g6)));
    fp2 = i960_rifl_read(r14, r15);
    i960_rifl_write(&r14, &r15, (fp2) - (fp0));
    g9 = *(u32 *)(fp + 0x184);
    g7 = *(u32 *)(fp + 0x178);
    fp3 = i960_u32_to_f64(r10);
    g10 = g9;
    g8 = 0x51eb851fu;
    g9 = 0x3fe51eb8u;
    g4 = i960_f64_to_u32((i960_u32_to_f64(g10)) + (i960_u32_to_f64(g7)));
    i960_rifl_write(&r10, &r11, fp3);
    fp3 = i960_u32_to_f64(g0);
    *(u64 *)(fp + 0x1d0) = ((u64)r15 << 32) | (u32)r14;
    g7 = i960_f64_to_u32((i960_u32_to_f64(g10)) - (i960_u32_to_f64(g7)));
    g10 = 0x851eb852u;
    g11 = 0x3fe451ebu;
    fp1 = i960_rifl_read(g8, g9);
    { u64 _p = *(u64 *)(fp + 0x1d0); r14 = (u32)_p; r15 = (u32)(_p >> 32); }
    g8 = 0x9999999au;
    g9 = 0x3ff59999u;
    g6 = *(u32 *)(fp + 0x190);
    i960_rifl_write(&g0, &g1, fp3);
    fp3 = i960_rifl_read(g10, g11);
    r8 = *(u32 *)(fp + 0x19c);
    fp2 = i960_rifl_read(g8, g9);
    i960_rifl_write(&r10, &r11, (fp3) * (i960_rifl_read(r10, r11)));
    fp3 = i960_rifl_read(r14, r15);
    g9 = g7;
    i960_rifl_write(&r14, &r15, (fp2) * (fp3));
    fp3 = i960_u32_to_f64(g9);
    r4 = i960_f64_to_u32((i960_u32_to_f64(r8)) - (i960_u32_to_f64(g6)));
    g4 = i960_f64_to_u32((i960_u32_to_f64(g6)) + (i960_u32_to_f64(g4)));
    *(u64 *)(fp + 0x1d0) = ((u64)r15 << 32) | (u32)r14;
    r14 = 0x851eb852u;
    r15 = 0x3fe451ebu;
    g5 = i960_f64_to_u32((i960_u32_to_f64(r9)) + (i960_u32_to_f64(g5)));
    i960_rifl_write(&g8, &g9, fp3);
    fp3 = i960_rifl_read(r14, r15);
    r14 = *(u32 *)(fp + 0x158);
    *(u32 *)(fp + 0x200) = (u32)g7;
    fp2 = i960_u32_to_f64(r4);
    *(u64 *)(fp + 0x200) = ((u64)g9 << 32) | (u32)g8;
    g4 = i960_f64_to_u32((i960_u32_to_f64(r8)) + (i960_u32_to_f64(g4)));
    g8 = *(u32 *)(fp + 0x154);
    { u64 _p = *(u64 *)(fp + 0x200); g10 = (u32)_p; g11 = (u32)(_p >> 32); }
    g5 = i960_f64_to_u32((i960_u32_to_f64(r12)) + (i960_u32_to_f64(g5)));
    i960_rifl_write(&g0, &g1, (fp1) * (i960_rifl_read(g0, g1)));
    i960_rifl_write(&r4, &r5, fp2);
    i960_rifl_write(&r4, &r5, (fp1) * (i960_rifl_read(r4, r5)));
    fp1 = i960_u32_to_f64(r14);
    fp1 = i960_u32_to_f64(r14);
    fp0 = i960_u32_to_f64(g8);
    fp2 = i960_rifl_read(g10, g11);
    i960_rifl_write(&g10, &g11, (fp3) * (fp2));
    fp2 = i960_u32_to_f64(g4);
    { u64 _p = *(u64 *)(fp + 0x1d0); r14 = (u32)_p; r15 = (u32)(_p >> 32); }
    fp0 = i960_u32_to_f64(g8);
    fp3 = i960_u32_to_f64(g5);
    i960_rifl_write(&r8, &r9, fp2);
    fp2 = i960_rifl_read(r14, r15);
    i960_rifl_write(&r14, &r15, (fp2) + (fp0));
    g8 = 0x9999999au;
    g9 = 0x3fc99999u;
    i960_rifl_write(&r10, &r11, (i960_rifl_read(g0, g1)) + (i960_rifl_read(r10, r11)));
    i960_rifl_write(&g0, &g1, fp3);
    fp3 = i960_rifl_read(g8, g9);
    g8 = *(u32 *)(fp + 0x80);
    fp0 = i960_rifl_read(r14, r15);
    r14 = *(u32 *)(fp + 0x7c);
    g9 = g8;
    *(u64 *)(fp + 0x200) = ((u64)g11 << 32) | (u32)g10;
    g2 = i960_f64_to_u32((i960_u32_to_f64(g9)) + (i960_u32_to_f64(g2)));
    { u64 _p = *(u64 *)(fp + 0x200); g8 = (u32)_p; g9 = (u32)(_p >> 32); }
    i960_rifl_write(&r10, &r11, (i960_rifl_read(r10, r11)) + (fp1));
    r15 = r14;
    g13 = i960_f64_to_u32((i960_u32_to_f64(r15)) + (i960_u32_to_f64(g13)));
    i960_rifl_write(&r8, &r9, (fp3) * (i960_rifl_read(r8, r9)));
    fp3 = i960_rifl_read(g8, g9);
    i960_rifl_write(&r4, &r5, (i960_rifl_read(r4, r5)) + (fp3));
    fp2 = 0.2;
    r15 = *(u32 *)(fp + 0x88);
    g10 = *(u32 *)(fp + 0x84);
    g8 = *(u32 *)(fp + 0x8c);
    i960_rifl_write(&g0, &g1, (fp2) * (i960_rifl_read(g0, g1)));
    fp1 = i960_rifl_read(r10, r11);
    g12 = i960_f64_to_u32(fp1);
    fp2 = i960_u32_to_f64(r15);
    r14 = i960_f64_to_u32(fp0);
    g11 = g10;
    g10 = *(u32 *)(fp + 0x90);
    g13 = i960_f64_to_u32((fp2) + (i960_u32_to_f64(g13)));
    fp2 = i960_u32_to_f64(r6);
    *(u64 *)(fp + 0x200) = ((u64)r5 << 32) | (u32)r4;
    fp1 = i960_u32_to_f64(g12);
    fp0 = i960_u32_to_f64(r14);
    fp0 = i960_u32_to_f64(r14);
    i960_rifl_write(&r14, &r15, fp2);
    *(u64 *)(fp + 0x1d0) = ((u64)r15 << 32) | (u32)r14;
    i960_rifl_write(&r14, &r15, fp1);
    g9 = g8;
    g2 = i960_f64_to_u32((i960_u32_to_f64(g9)) + (i960_u32_to_f64(g2)));
    fp3 = i960_rifl_read(r4, r5);
    i960_rifl_write(&g8, &g9, (fp3) + (fp0));
    *(u64 *)(fp + 0x200) = ((u64)r15 << 32) | (u32)r14;
    r15 = *(u32 *)(fp + 0x94);
    g7 = *(u32 *)(fp + 0x120);
    fp0 = i960_rifl_read(g8, g9);
    g8 = *(u32 *)(fp + 0x98);
    r14 = *(u32 *)(fp + 0x9c);
    g3 = i960_f64_to_u32((i960_u32_to_f64(g11)) + (i960_u32_to_f64(g3)));
    g11 = g10;
    g3 = i960_f64_to_u32((i960_u32_to_f64(g11)) + (i960_u32_to_f64(g3)));
    { u64 _p = *(u64 *)(fp + 0x1d0); g10 = (u32)_p; g11 = (u32)(_p >> 32); }
    g5 = *(u32 *)(fp + 0x124);
    fp2 = i960_u32_to_f64(r15);
    r15 = r14;
    g3 = i960_f64_to_u32((i960_u32_to_f64(r15)) + (i960_u32_to_f64(g3)));
    r15 = *(u32 *)(fp + 0x1a0);
    g6 = *(u32 *)(fp + 0x128);
    g9 = g8;
    g13 = i960_f64_to_u32((fp2) + (i960_u32_to_f64(g13)));
    fp2 = i960_u32_to_f64(r15);
    g2 = i960_f64_to_u32((i960_u32_to_f64(g9)) + (i960_u32_to_f64(g2)));
    g13 = i960_f64_to_u32((i960_u32_to_f64(g7)) + (i960_u32_to_f64(g13)));
    { u64 _p = *(u64 *)(fp + 0x200); g8 = (u32)_p; g9 = (u32)(_p >> 32); }
    i960_rifl_write(&r14, &r15, fp2);
    g2 = i960_f64_to_u32((i960_u32_to_f64(g5)) + (i960_u32_to_f64(g2)));
    *(u64 *)(fp + 0x1c0) = ((u64)r15 << 32) | (u32)r14;
    fp3 = i960_rifl_read(g10, g11);
    g3 = i960_f64_to_u32((i960_u32_to_f64(g6)) + (i960_u32_to_f64(g3)));
    i960_rifl_write(&r8, &r9, (i960_rifl_read(r8, r9)) + (fp3));
    fp3 = i960_rifl_read(g8, g9);
    { u64 _p = *(u64 *)(fp + 0x1c0); g8 = (u32)_p; g9 = (u32)(_p >> 32); }
    *(u32 *)(fp + 0x120) = (u32)g13;
    *(u32 *)(fp + 0x124) = (u32)g2;
    *(u32 *)(fp + 0x128) = (u32)g3;
    i960_rifl_write(&g0, &g1, (fp3) - (i960_rifl_read(g0, g1)));
    fp3 = i960_rifl_read(r8, r9);
    *(u64 *)(fp + 0x200) = ((u64)g1 << 32) | (u32)g0;
    g0 = i960_f64_to_u32(fp3);
    { u64 _p = *(u64 *)(fp + 0x200); r14 = (u32)_p; r15 = (u32)(_p >> 32); }
    fp3 = i960_rifl_read(g8, g9);
    *(u32 *)(fp + 0x158) = (u32)g12;
    *(u64 *)(fp + 0x1d0) = ((u64)r9 << 32) | (u32)r8;
    g4 = i960_f64_to_u32(fp0);
    fp2 = i960_rifl_read(r14, r15);
    g1 = i960_f64_to_u32(fp2);
    *(u32 *)(fp + 0x150) = (u32)g0;
    *(u32 *)(fp + 0x154) = (u32)g4;
    *(u32 *)(fp + 0x158) = (u32)g1;
    if (fp3 == 0.0)
        goto L_00036508;
    r14 = *(u32 *)(fp + 0x1f0);
    r15 = 0x16002c2cu;
    i960_mmio_write_u32(0x884000, (u32)r15); /* copro_fifo */;
    g7 = r14 + 0x2c;
    g4 = *gw32((u32)(uintptr_t)(g7));
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g6 = r14 + 0x30;
    g4 = *gw32((u32)(uintptr_t)(g6));
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g5 = r14 + 0x34;
    g4 = *gw32((u32)(uintptr_t)(g5));
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g8 = *(u32 *)(fp + 0x1e0);
    (void)g8;
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    /* @0x363A4: st g4,(g8) with g8 = fp+0x170 — full host EA. */
    *(u32 *)fp170_host = (u32)g4;
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    *(u32 *)(fp + 0x174) = (u32)g4;
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    *(u32 *)(fp + 0x178) = (u32)g4;
    g9 = r15;
    i960_mmio_write_u32(0x884000, (u32)g9); /* copro_fifo */;
    g4 = *gw32((u32)(uintptr_t)(g7));
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g4 = *gw32((u32)(uintptr_t)(g6));
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g4 = *gw32((u32)(uintptr_t)(g5));
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    fp0 = 0.1;
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    i960_rifl_write(&r14, &r15, (fp0) * (fp3));
    /* @0x36410: second st g4,(g8) into fp+0x170. */
    *(u32 *)fp170_host = (u32)g4;
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    *(u32 *)(fp + 0x174) = (u32)g4;
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    *(u32 *)(fp + 0x178) = (u32)g4;
    fp1 = i960_rifl_read(r14, r15);
    r15 = *(u32 *)(fp + 0x1f0);
    g5 = r15 + 0x50;
    g8 = *gw32((u32)(uintptr_t)(g5));
    fp0 = i960_u32_to_f64(g8);
    fp0 = i960_u32_to_f64(g8);
    i960_rifl_write(&g10, &g11, (fp0) * (fp1));
    fp1 = i960_rifl_read(g10, g11);
    g11 = *(u32 *)(fp + 0x178);
    fp0 = i960_u32_to_f64(g11);
    fp0 = i960_u32_to_f64(g11);
    i960_rifl_write(&r14, &r15, (fp0) * (fp1));
    fp3 = 0.2;
    fp1 = i960_rifl_read(r14, r15);
    i960_rifl_write(&g8, &g9, (fp3) * (fp1));
    fp0 = i960_u32_to_f64(g0);
    { u64 _p = *(u64 *)(fp + 0x1c0); g10 = (u32)_p; g11 = (u32)(_p >> 32); }
    fp1 = i960_rifl_read(g8, g9);
    g8 = 0;
    g9 = 0x3fe00000u;
    i960_rifl_write(&r14, &r15, (fp0) - (fp1));
    fp1 = i960_rifl_read(g8, g9);
    fp3 = i960_rifl_read(g10, g11);
    i960_rifl_write(&g10, &g11, (fp1) * (fp3));
    fp0 = i960_rifl_read(r14, r15);
    g4 = i960_f64_to_u32(fp0);
    *(u32 *)(fp + 0x150) = (u32)g4;
    g5 = *gw32((u32)(uintptr_t)(g5));
    fp0 = i960_u32_to_f64(g5);
    fp0 = i960_u32_to_f64(g5);
    fp1 = i960_rifl_read(g10, g11);
    i960_rifl_write(&r14, &r15, (fp0) * (fp1));
    fp1 = i960_rifl_read(r14, r15);
    r15 = *(u32 *)(fp + 0x170);
    fp0 = i960_u32_to_f64(r15);
    fp0 = i960_u32_to_f64(r15);
    i960_rifl_write(&g8, &g9, (fp0) * (fp1));
    fp3 = 0.2;
    fp1 = i960_rifl_read(g8, g9);
    i960_rifl_write(&g10, &g11, (fp3) * (fp1));
    fp0 = i960_u32_to_f64(g1);
    fp1 = i960_rifl_read(g10, g11);
    i960_rifl_write(&r14, &r15, (fp1) + (fp0));
    fp0 = i960_rifl_read(r14, r15);
    g4 = i960_f64_to_u32(fp0);
    *(u32 *)(fp + 0x158) = (u32)g4;

    L_00036508:
        g4 = i960_ld_u32(I960_WORKRAM, 0x2020b4, 0);
        r15 = 0x10802121u;
        i960_mmio_write_u32(0x884000, (u32)r15); /* copro_fifo */;
        if ((unsigned char)g4 == 0)
            goto L_000365e8;
        g8 = 0x10002020u;
        g10 = *(u32 *)(fp + 0x1b0);
        i960_mmio_write_u32(0x884000, (u32)g8); /* copro_fifo */;
        g9 = 0x12802525u;
        i960_mmio_write_u32(0x884000, (u32)g9); /* copro_fifo */;
        g4 = *gw32((u32)(uintptr_t)(g10));
        g4 = g4 ^ (1u << 31);
        g11 = 0x15002a2au;
        i960_mmio_write_u32(0x884000, (u32)g11); /* copro_fifo */;
        r14 = *(u32 *)(fp + 0x1f0);
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        g12 = 0x16002c2cu;
        i960_mmio_write_u32(0x884000, (u32)g12); /* copro_fifo */;
        g4 = *gw32((u32)(uintptr_t)(r14 + 0x2c));
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        g4 = *gw32((u32)(uintptr_t)(r14 + 0x30));
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        g4 = *gw32((u32)(uintptr_t)(r14 + 0x34));
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        g5 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        g6 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        *(u32 *)(fp + 0x168) = (u32)g4;
        g4 = *(u32 *)(fp + 0x168);
        *(u32 *)(fp + 0x160) = (u32)g5;
        *(u32 *)(fp + 0x164) = (u32)g6;
        if (i960_u32_to_f64(g4) < i960_u32_to_f64(+0.0)) {
            g4 = g4 ^ (1u << 31);
        }
    i960_st_u32(I960_WORKRAM, 0x20b0b4, 0, (u32)g4);
    i960_mmio_write_u32(0x884000, (u32)r15); /* copro_fifo */;
    goto L_0003670c;

    L_000365e8:
        g4 = i960_ld_u32(I960_WORKRAM, 0x20a560, 0);
        /*
         * @0x365F0: cmpibge 0,g4,0x366ac — Intel: branch when timer <= 0.
         * Countdown keeps bare |vel| in 0x20b0b4; after GO the TGP 0x11
         * arm (table inv @ firmware 0x1A7) scales by k/0x214124.
         */
        if ((i32)g4 <= 0)
            goto L_000366ac;
        g8 = i960_ld_u32(I960_WORKRAM, 0x214124, 0);
        fp0 = i960_u32_to_f64(g8);
        fp0 = i960_u32_to_f64(g8);
        fp1 = 0.002802857142857143;
        i960_rifl_write(&r14, &r15, (fp1) / (fp0));
        fp1 = i960_rifl_read(r14, r15);
        r15 = 0x2c005858u;
        i960_mmio_write_u32(0x884000, (u32)r15); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
        g8 = *(u32 *)(fp + 0x1f0);
        i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
        g4 = *gw32((u32)(uintptr_t)(g8 + 0x2c));
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        g4 = *gw32((u32)(uintptr_t)(g8 + 0x30));
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        g4 = *gw32((u32)(uintptr_t)(g8 + 0x34));
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        g4 = i960_f64_to_u32(fp1);
        g5 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        g9 = 0x8801111u;
        i960_mmio_write_u32(0x884000, (u32)g9); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        g5 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(g5)));
        i960_st_u32(I960_WORKRAM, 0x20b0b4, 0, (u32)g5);
        goto L_0003670c;

    L_000366ac:
        g10 = 0x2c005858u;
        i960_mmio_write_u32(0x884000, (u32)g10); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
        g11 = *(u32 *)(fp + 0x1f0);
        i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
        g4 = *gw32((u32)(uintptr_t)(g11 + 0x2c));
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        g4 = *gw32((u32)(uintptr_t)(g11 + 0x30));
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        g4 = *gw32((u32)(uintptr_t)(g11 + 0x34));
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        i960_st_u32(I960_WORKRAM, 0x20b0b4, 0, (u32)g4);

    L_0003670c:
        g4 = i960_ld_u32(I960_WORKRAM, 0x2138a4, 0);
        g5 = i960_ld_u32(I960_WORKRAM, 0x2138a0, 0);
        g12 = *(u32 *)(fp + 0x1f0);
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) - (i960_u32_to_f64(g5)));
        g5 = *gw32((u32)(uintptr_t)(g12 + 0x50));
        g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) * (i960_u32_to_f64(g4)));
        fp0 = i960_u32_to_f64(g4);
        fp1 = 0.005;
        i960_rifl_write(&g8, &g9, (fp1) * (fp0));
        fp0 = i960_rifl_read(g8, g9);
        g0 = *(u32 *)(fp + 0x1f0);
        g3 = i960_f64_to_u32(fp0);
        g1 = fp + 0x120;
        g2 = fp + 0x150;
        {
            scene_local_regs caller_r = save_scene_local_regs();

            geo_view_frame_update((void *)(uintptr_t)g0,
                                  (void *)(uintptr_t)g1,
                                  (void *)(uintptr_t)g2);
            restore_scene_local_regs(caller_r);
        }
    /* Reload node — disasm keeps g12 via callee-save; host may still clobber. */
    g12 = *(u32 *)(fp + 0x1f0);
    g5 = *gw32((u32)(uintptr_t)(g12 + 0x54));
    g4 = *(u32 *)(fp + 0x150);
    g7 = g12 + 0x44;
    /* Locals for post-clamp pitch_rate log (avoid fp scratch). */
    u32 pitch_log_cam54 = (u32)g5;
    u32 pitch_log_fp150 = *(u32 *)(fp + 0x150);
    u32 pitch_log_g3 = (u32)g3;
    g4 = fdiv_leave(0, (u32)g4, (u32)g5);
    g5 = *gw32((u32)(uintptr_t)(g7));
    *gw32((u32)(uintptr_t)(g7)) = fadd_leave((u32)g5, (u32)g4);
    g4 = *(u32 *)(fp + 0x154);
    g5 = *gw32((u32)(uintptr_t)(g12 + 0x58));
    g6 = g12 + 0x48;
    g4 = fdiv_leave(0, (u32)g4, (u32)g5);
    g5 = *gw32((u32)(uintptr_t)(g6));
    *gw32((u32)(uintptr_t)(g6)) = fadd_leave((u32)g5, (u32)g4);
    g4 = *(u32 *)(fp + 0x158);
    g5 = *gw32((u32)(uintptr_t)(g12 + 0x5c));
    g6 = g12 + 0x4c;
    g4 = fdiv_leave(0, (u32)g4, (u32)g5);
    g5 = *gw32((u32)(uintptr_t)(g6));
    *gw32((u32)(uintptr_t)(g6)) = fadd_leave((u32)g5, (u32)g4);
    g4 = *gw32((u32)(uintptr_t)(g7));
    if (i960_u32_to_f64(g4) < i960_u32_to_f64(+0.0)) {
        g4 = g4 ^ (1u << 31);
    }
    fp0 = i960_u32_to_f64(g4);
    fp3 = 0.1;
    if (fp0 <= fp3)
        goto L_0003680c;
    r14 = *(u32 *)(fp + 0x1f0);
    g5 = r14 + 0x44;
    /* Clamp to ±0.1 keeping the sign of the live value (disasm @ 0x367E4). */
    g4 = 0x3dcccccdu;
    if (i960_u32_to_f64(*gw32((u32)(uintptr_t)(g5))) < i960_u32_to_f64(+0.0))
        g4 = 0xbdcccccdu;
    *gw32((u32)(uintptr_t)(g5)) = (u32)g4;

    L_0003680c:
    {
        static int pitch_rate_logs;
        double pitch_now =
            i960_u32_to_f64(*gw32((u32)(uintptr_t)(
                *(u32 *)(fp + 0x1f0) + 0x38)));

        /*
         * Log after ±0.1 clamp. First 12 always; then keep sampling while
         * |pitch| is in the stuck band so chase isn't lost after desert.
         * flag974==0xf ⇒ all four vec_push main-paths → dampers skipped
         * (disasm cmpibe 15); pitch then nulls height a4−a0 only.
         */
        if (pitch_rate_logs < 12 ||
            (pitch_rate_logs < 40 &&
             (pitch_now > 0.4 || pitch_now < -0.4))) {
            u32 node = *(u32 *)(fp + 0x1f0);

            lift_log(
                    "lift: pitch_rate node=%#x cam54=%.4g a0=%.4g a4=%.4g g3=%.4g "
                    "fp150=%.4g rate=%.4g pitch=%.4g flag974=%#x\n",
                    (unsigned)node,
                    i960_u32_to_f64(pitch_log_cam54),
                    i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x2138a0, 0)),
                    i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x2138a4, 0)),
                    i960_u32_to_f64(pitch_log_g3),
                    i960_u32_to_f64(pitch_log_fp150),
                    i960_u32_to_f64(*gw32((u32)(uintptr_t)(node + 0x44))),
                    pitch_now,
                    (unsigned)(i960_ld_u32(I960_WORKRAM, 0x213974, 0) & 0xffu));
            fflush(stderr);
            pitch_rate_logs++;
        }
    }
        r15 = *(u32 *)(fp + 0x1f0);
        g4 = *gw32((u32)(uintptr_t)(r15 + 0x48));
        if (i960_u32_to_f64(g4) < i960_u32_to_f64(+0.0)) {
            g4 = g4 ^ (1u << 31);
        }
    fp0 = i960_u32_to_f64(g4);
    fp3 = 0.1;
    if (fp0 <= fp3)
        goto L_00036868;
    r14 = *(u32 *)(fp + 0x1f0);
    g5 = r14 + 0x48;
    g4 = 0x3dcccccdu;
    if (i960_u32_to_f64(*gw32((u32)(uintptr_t)(g5))) < i960_u32_to_f64(+0.0))
        g4 = 0xbdcccccdu;
    *gw32((u32)(uintptr_t)(g5)) = (u32)g4;

    L_00036868:
        r15 = *(u32 *)(fp + 0x1f0);
        g4 = *gw32((u32)(uintptr_t)(r15 + 0x4c));
        if (i960_u32_to_f64(g4) < i960_u32_to_f64(+0.0)) {
            g4 = g4 ^ (1u << 31);
        }
    fp0 = i960_u32_to_f64(g4);
    fp3 = 0.1;
    if (fp0 <= fp3)
        goto L_000368c4;
    r14 = *(u32 *)(fp + 0x1f0);
    g5 = r14 + 0x4c;
    g4 = 0x3dcccccdu;
    if (i960_u32_to_f64(*gw32((u32)(uintptr_t)(g5))) < i960_u32_to_f64(+0.0))
        g4 = 0xbdcccccdu;
    *gw32((u32)(uintptr_t)(g5)) = (u32)g4;

    L_000368c4:
        g0 = 2;
        g1 = 25;
        tile_cursor_seed((u32)g0, (u32)g1);
    g4 = i960_ld_u32(I960_WORKRAM, 0x213974, 0);
    if ((unsigned char)g4 == 15)
        goto L_000369b4;
    r15 = *(u32 *)(fp + 0x1f0);
    g4 = *gw32((u32)(uintptr_t)(r15 + 0x4c));
    fp3 = 1.0;
    i960_rifl_write(&g8, &g9, fp3);
    *(u64 *)(fp + 0x200) = ((u64)g9 << 32) | (u32)g8;
    if (i960_u32_to_f64(g4) < i960_u32_to_f64(+0.0)) {
        r14 = 0;
        r15 = 0xbff00000u;
        *(u64 *)(fp + 0x200) = ((u64)r15 << 32) | (u32)r14;
    }
    r15 = *(u32 *)(fp + 0x1f0);
    g5 = r15 + 0x40;
    g8 = *gw32((u32)(uintptr_t)(g5));
    fp1 = i960_u32_to_f64(g8);
    fp0 = 1.0;
    /* Disasm @ 0x36918: cmpr fp1,+0.0 — fp1 is the live angle, not float bits. */
    if (fp1 < 0.0) {
        g10 = 0;
        g11 = 0xbff00000u;
        fp0 = i960_rifl_read(g10, g11);
    }
    { u64 _p = *(u64 *)(fp + 0x200); r14 = (u32)_p; r15 = (u32)(_p >> 32); }
    fp2 = i960_rifl_read(r14, r15);
    if (fp2 != fp0)
        goto L_000369b4;
    g4 = i960_f64_to_u32((fp1) * (fp1));
    g8 = i960_ld_u32(I960_WORKRAM, 0x5d3e3c, 0);
    i960_rifl_write(&g10, &g11, fp1);
    fp0 = i960_u32_to_f64(g8);
    fp0 = i960_u32_to_f64(g8);
    *(u64 *)(fp + 0x200) = ((u64)g11 << 32) | (u32)g10;
    /*
     * Disasm @ 0x36954–0x36968: cmpr angle,+0.0 then movr ang²→fp1 then bge.
     * Condition is the angle sign (CC preserved), not ang² (always ≥0).
     */
    {
        double ang = i960_rifl_read(g10, g11);

        fp1 = i960_u32_to_f64(g4);
        if (ang >= 0.0)
            goto L_00036994;
    }
    i960_rifl_write(&r14, &r15, fp0);
    r15 = r15 ^ (1u << 31);
    fp0 = i960_rifl_read(r14, r15);
    i960_rifl_write(&g8, &g9, (fp1) * (fp0));
    { u64 _p = *(u64 *)(fp + 0x200); g10 = (u32)_p; g11 = (u32)(_p >> 32); }
    fp3 = i960_rifl_read(g10, g11);
    fp0 = i960_rifl_read(g8, g9);
    i960_rifl_write(&g10, &g11, (fp3) - (fp0));
    fp0 = i960_rifl_read(g10, g11);
    goto L_000369ac;

    L_00036994:
        i960_rifl_write(&r14, &r15, (fp1) * (fp0));
        { u64 _p = *(u64 *)(fp + 0x200); g8 = (u32)_p; g9 = (u32)(_p >> 32); }
        fp3 = i960_rifl_read(g8, g9);
        fp0 = i960_rifl_read(r14, r15);
        i960_rifl_write(&g8, &g9, (fp3) - (fp0));
        fp0 = i960_rifl_read(g8, g9);

    L_000369ac:
        g4 = i960_f64_to_u32(fp0);
        *gw32((u32)(uintptr_t)(g5)) = (u32)g4;

    L_000369b4:
        g0 = 2;
        g1 = 26;
        tile_cursor_seed((u32)g0, (u32)g1);
    g4 = i960_ld_u32(I960_WORKRAM, 0x213974, 0);
    if ((unsigned char)g4 == 15)
        goto L_00036a5c;
    r14 = *(u32 *)(fp + 0x1f0);
    g5 = r14 + 0x38;
    r15 = *gw32((u32)(uintptr_t)(g5));
    fp0 = i960_u32_to_f64(r15);
    g4 = i960_f64_to_u32((fp0) * (fp0));
    g8 = i960_ld_u32(I960_WORKRAM, 0x5d3e3c, 0);
    fp1 = i960_u32_to_f64(g8);
    fp1 = i960_u32_to_f64(g8);
    i960_rifl_write(&r14, &r15, fp0);
    fp2 = i960_rifl_read(0x66666666u, 0x3ff66666u);
    i960_rifl_write(&g6, &g7, (fp2) * (fp1));
    *(u64 *)(fp + 0x1d0) = ((u64)r15 << 32) | (u32)r14;
    /*
     * Disasm @ 0x369E8 cmpr pitch,+0.0 … 0x36A18 bge after movr pitch²→fp1.
     * Branch on pitch sign (CC), not on pitch² / u32_to_f64(double).
     */
    {
        double pitch = i960_rifl_read(r14, r15);

        fp1 = i960_u32_to_f64(g4);
        if (pitch >= 0.0)
            goto L_00036a3c;
    }
    g7 = g7 ^ (1u << 31);
    fp0 = i960_rifl_read(g6, g7);
    i960_rifl_write(&g8, &g9, (fp1) * (fp0));
    fp3 = i960_rifl_read(r14, r15);
    fp0 = i960_rifl_read(g8, g9);
    i960_rifl_write(&g10, &g11, (fp3) - (fp0));
    fp0 = i960_rifl_read(g10, g11);
    goto L_00036a54;

    L_00036a3c:
        i960_rifl_write(&g6, &g7, (fp1) * (i960_rifl_read(g6, g7)));
        { u64 _p = *(u64 *)(fp + 0x1d0); r14 = (u32)_p; r15 = (u32)(_p >> 32); }
        fp2 = i960_rifl_read(r14, r15);
        fp0 = i960_rifl_read(g6, g7);
        i960_rifl_write(&r14, &r15, (fp2) - (fp0));
        fp0 = i960_rifl_read(r14, r15);

    L_00036a54:
        g4 = i960_f64_to_u32(fp0);
        *gw32((u32)(uintptr_t)(g5)) = (u32)g4;

    L_00036a5c:
        g6 = i960_ld_u32(I960_WORKRAM, 0x2141cc, 0);
        g8 = *(u32 *)(fp + 0x1f0);
        g7 = i960_ld_u32(I960_WORKRAM, 0x214204, 0);
        g4 = i960_ld_u32(I960_WORKRAM, 0x2141d0, 0);
        g0 = g8 + 0x48;
        g5 = *gw32((u32)(uintptr_t)(g0));
        g6 = i960_f64_to_u32((i960_u32_to_f64(g7)) * (i960_u32_to_f64(g6)));
        g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) * (i960_u32_to_f64(g4)));
        g6 = i960_f64_to_u32((i960_u32_to_f64(g4)) + (i960_u32_to_f64(g6)));
        g5 = i960_f64_to_u32((i960_u32_to_f64(g5)) - (i960_u32_to_f64(g6)));
        *gw32((u32)(uintptr_t)(g0)) = (u32)g5;
        g4 = i960_ld_u32(I960_WORKRAM, 0x202008, 0);
        g9 = 0xff;
        g4 = g4 + 4;
        g4 = g4 & 7;
        g4 = g4 & g9;
        if ((unsigned char)g4 != 0)
            goto L_00036b98;
        g4 = i960_ld_u32(I960_WORKRAM, 0x213974, 0);
        if ((unsigned char)g4 != 0)
            goto L_00036ac4;
        r4 = 0;
        goto L_00036afc;

    L_00036ac4:
        g10 = i960_ld_u32(I960_WORKRAM, 0x20b0b4, 0);
        fp0 = i960_u32_to_f64(g10);
        fp0 = i960_u32_to_f64(g10);
        r14 = 0;
        r15 = 0x405f4000;
        fp1 = i960_rifl_read(r14, r15);
        i960_rifl_write(&g8, &g9, (fp1) * (fp0));
        fp0 = i960_rifl_read(g8, g9);
        r4 = (uintptr_t)(i32)(fp0);
        g9 = 0x7f;
        if ((signed char)r4 > (signed char)g9) {
            r4 = 0x7f;
        }

    L_00036afc:
        g4 = i960_ld_u8(I960_WORKRAM, 0x214208, 0);
        /* Disasm: lda 0xcc,g10 — mask immediate, not a ROM host pointer. */
        g10 = 0xcc;
        g4 = g10 & g4;
        if ((unsigned char)g4 == 0)
            goto L_00036b54;
        g4 = i960_ld_u32(I960_WORKRAM, 0x2138c4, 0);
        if ((unsigned char)g4 != 0)
            goto L_00036b34;
        g0 = 0xb7;
        g1 = 0;
        {
            scene_local_regs caller_r = save_scene_local_regs();

            tile_texture_descriptor_apply((u32)g0, (u32)g1, (u32)g2);
            restore_scene_local_regs(caller_r);
        }
    g11 = 1;
    i960_st_u32(I960_WORKRAM, 0x2138c4, 0, (u32)g11);

    L_00036b34:
        /* Disasm: lda 0xb8,g0 — tile descriptor index, not ROM+0xb8. */
        g0 = 0xb8;
        g1 = r4;
        {
            scene_local_regs caller_r = save_scene_local_regs();

            tile_texture_descriptor_apply((u32)g0, (u32)g1, (u32)g2);
            restore_scene_local_regs(caller_r);
        }
    g4 = r4 << 3;
    r4 = (uintptr_t)((i32)g4 / (i32)10);
    /* Disasm: lda 0x5d3e40,r5 — guest table VA (ROM-mirrored workram). */
    r5 = 0x5d3e40u;
    goto L_00036b78;

    L_00036b54:
        g4 = i960_ld_u32(I960_WORKRAM, 0x2138c4, 0);
        if ((unsigned char)g4 == 0)
            goto L_00036b6c;
        g0 = 0xb8;
        g1 = 0;
        {
            scene_local_regs caller_r = save_scene_local_regs();

            tile_texture_descriptor_apply((u32)g0, (u32)g1, (u32)g2);
            restore_scene_local_regs(caller_r);
        }

    L_00036b6c:
        r4 = (u32)r4 >> 1;
        r5 = 0x5d3ec0u;

    L_00036b78:
        if ((unsigned char)r4 == 0)
            goto L_00036b98;
        g0 = geo_rng_u8((u32)g0, (u32)g1, (u32)g2);
    g12 = 0xff;
    g4 = g12 & g0;
    if ((signed char)g4 >= (signed char)r4)
        goto L_00036b98;
    g0 = 31 & g0;
    g0 = i960_ld_u32(I960_ABS, (u32)r5, (u32)(g0 << 2));
    {
        scene_local_regs caller_r = save_scene_local_regs();

        comm_palette_index_call((u32)g0);
        restore_scene_local_regs(caller_r);
    }

    L_00036b98:
        r14 = *(u32 *)(fp + 0x1a0);
        fp0 = i960_u32_to_f64(r14);
        if (fp0 <= 0.0)
            goto L_00036cc4;
        r15 = i960_ld_u32(I960_WORKRAM, 0x20b0b4, 0);
        fp0 = i960_u32_to_f64(r15);
        fp0 = i960_u32_to_f64(r15);
        fp3 = i960_rifl_read(0xed097b42u, 0x3fa7b425u);
        if (fp0 >= fp3)
            goto L_00036cc4;
        g4 = *(u32 *)(fp + 0xb0);
        g5 = i960_ld_u32(I960_WORKRAM, 0x214368, 0);
        g6 = *(u32 *)(fp + 0xb4);
        g7 = i960_ld_u32(I960_WORKRAM, 0x214364, 0);
        g0 = i960_ld_u32(I960_WORKRAM, 0x214360, 0);
        r14 = 0x2d805b5bu;
        i960_mmio_write_u32(0x884000, (u32)r14); /* copro_fifo */;
        g1 = *(u32 *)(fp + 0xac);
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g6); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g7); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g0); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g1); /* copro_fifo */;
        g5 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        *(u32 *)(fp + 0x164) = (u32)g4;
        r15 = *(u32 *)(fp + 0x164);
        fp0 = 3.141592653589793;
        fp1 = i960_u32_to_f64(r15);
        fp1 = i960_u32_to_f64(r15);
        r14 = 0xeb851eb8u;
        r15 = 0x3f8eb851u;
        i960_rifl_write(&g10, &g11, (fp0) * (fp1));
        fp0 = i960_rifl_read(r14, r15);
        fp1 = i960_rifl_read(g10, g11);
        i960_rifl_write(&g8, &g9, (fp0) * (fp1));
        g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        fp1 = i960_rifl_read(g8, g9);
        g9 = *(u32 *)(fp + 0x1f0);
        *(u32 *)(fp + 0x160) = (u32)g5;
        *(u32 *)(fp + 0x168) = (u32)g4;
        g5 = g9 + 0x48;
        g10 = *gw32((u32)(uintptr_t)(g5));
        fp0 = i960_u32_to_f64(g10);
        fp0 = i960_u32_to_f64(g10);
        i960_rifl_write(&r14, &r15, (fp0) - (fp1));
        fp0 = i960_rifl_read(r14, r15);
        g4 = i960_f64_to_u32(fp0);
        *gw32((u32)(uintptr_t)(g5)) = (u32)g4;

    L_00036cc4:
        r15 = *(u32 *)(fp + 0x1f0);
        g7 = *(u32 *)(fp + 0x120);
        g6 = r15 + 0x50;
        g4 = *gw32((u32)(uintptr_t)(g6));
        g7 = fdiv_leave(0, (u32)g7, (u32)g4);
        *(u32 *)(fp + 0xe0) = (u32)g7;
        g4 = *(u32 *)(fp + 0x124);
        g5 = *gw32((u32)(uintptr_t)(g6));
        g4 = fdiv_leave(0, (u32)g4, (u32)g5);
        *(u32 *)(fp + 0xe4) = (u32)g4;
        g4 = *(u32 *)(fp + 0x128);
        g5 = *gw32((u32)(uintptr_t)(g6));
        g4 = fdiv_leave(0, (u32)g4, (u32)g5);
        *(u32 *)(fp + 0xe8) = (u32)g4;
        g0 = r15 + 0x2c;
        g4 = *gw32((u32)(uintptr_t)(g0));
        *gw32((u32)(uintptr_t)(g0)) = fadd_leave((u32)g4, (u32)g7);
        g7 = r15 + 0x30;
        g4 = *gw32((u32)(uintptr_t)(g7));
        g5 = *(u32 *)(fp + 0xe4);
        *gw32((u32)(uintptr_t)(g7)) = fadd_leave((u32)g4, (u32)g5);
        g1 = r15 + 0x34;
        g4 = *gw32((u32)(uintptr_t)(g1));
        g5 = *(u32 *)(fp + 0xe8);
        *gw32((u32)(uintptr_t)(g1)) = fadd_leave((u32)g4, (u32)g5);
        g4 = *gw32((u32)(uintptr_t)(g7));
        g5 = i960_ld_u32(I960_WORKRAM, 0x214124, 0);
        {
            double vy = i960_u32_to_f64(g4) - i960_u32_to_f64(g5);

            *gw32((u32)(uintptr_t)(g7)) =
                isfinite(vy) ? (u32)i960_f64_to_u32(vy) : (u32)g4;
        }
        g6 = r15 + 0x38;
        g4 = *gw32((u32)(uintptr_t)(g6));
        g5 = *gw32((u32)(uintptr_t)(r15 + 0x44));
        *gw32((u32)(uintptr_t)(g6)) = fadd_leave((u32)g4, (u32)g5);
        g6 = r15 + 0x3c;
        g4 = *gw32((u32)(uintptr_t)(g6));
        g5 = *gw32((u32)(uintptr_t)(r15 + 0x48));
        *gw32((u32)(uintptr_t)(g6)) = fadd_leave((u32)g4, (u32)g5);
        g6 = r15 + 0x40;
        g4 = *gw32((u32)(uintptr_t)(g6));
        g5 = *gw32((u32)(uintptr_t)(r15 + 0x4c));
        *gw32((u32)(uintptr_t)(g6)) = fadd_leave((u32)g4, (u32)g5);
        g4 = i960_ld_u32(I960_WORKRAM, 0x214120, 0);
        if ((unsigned char)g4 == 0) {
            *gw32((u32)(uintptr_t)(g1)) = (u32)g14;
            *gw32((u32)(uintptr_t)(g0)) = (u32)g14;
        }
    g8 = *(u32 *)(fp + 0x1f0);
    g5 = *gw32((u32)(uintptr_t)(g0));
    g6 = g8 + 20;
    g4 = *gw32((u32)(uintptr_t)(g6));
    g4 = fadd_leave((u32)g4, (u32)g5);
    *gw32((u32)(uintptr_t)(g6)) = (u32)g4;
    g6 = g8 + 24;
    g4 = *gw32((u32)(uintptr_t)(g6));
    g5 = *gw32((u32)(uintptr_t)(g7));
    g4 = fadd_leave((u32)g4, (u32)g5);
    *gw32((u32)(uintptr_t)(g6)) = (u32)g4;
    g6 = g8 + 28;
    g4 = *gw32((u32)(uintptr_t)(g6));
    g5 = *gw32((u32)(uintptr_t)(g1));
    g4 = fadd_leave((u32)g4, (u32)g5);
    g0 = *(u32 *)(fp + 0x1a0);
    *gw32((u32)(uintptr_t)(g6)) = (u32)g4;
    {
        scene_local_regs caller_r = save_scene_local_regs();

        geo_view_mode_latch((u32)g0, (u32)g1, (u32)g2);
        restore_scene_local_regs(caller_r);
    }
    g9 = *(u32 *)(fp + 0x1a0);
    fp1 = i960_u32_to_f64(g9);
    if (fp1 <= 0.0)
        goto L_00036eb4;
    g4 = i960_host_race_course_index();
    if ((unsigned char)g4 != 3)
        goto L_00036e38;
    fp0 = i960_rifl_read(0xcccccccdu, 0x3ffcccccu);
    i960_rifl_write(&r14, &r15, (fp0) * (fp1));
    fp0 = i960_rifl_read(0x7ae147aeu, 0x3fefae14u);
    fp1 = i960_rifl_read(r14, r15);
    i960_rifl_write(&g10, &g11, (fp0) - (fp1));
    fp0 = i960_rifl_read(g10, g11);
    goto L_00036e7c;

    L_00036e38:
        g11 = *(u32 *)(fp + 0x1a0);
        g4 = i960_ld_u32(I960_WORKRAM, 0x2141f0, 0);
        fp3 = i960_u32_to_f64(g11);
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (fp3));
        fp0 = i960_u32_to_f64(g4);
        fp0 = i960_u32_to_f64(g4);
        fp1 = i960_rifl_read(0x8b439581u, 0x3fefe76cu);
        i960_rifl_write(&g8, &g9, (fp1) - (fp0));
        fp0 = i960_rifl_read(g8, g9);
        if (fp0 < 0.0) {
            fp0 = 0.0;
        }

    L_00036e7c:
        g6 = i960_f64_to_u32(fp0);
        g9 = *(u32 *)(fp + 0x1f0);
        g5 = g9 + 0x2c;
        g4 = *gw32((u32)(uintptr_t)(g5));
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(g6)));
        *gw32((u32)(uintptr_t)(g5)) = (u32)g4;
        g5 = g9 + 0x30;
        g4 = *gw32((u32)(uintptr_t)(g5));
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(g6)));
        *gw32((u32)(uintptr_t)(g5)) = (u32)g4;
        g5 = g9 + 0x34;
        g4 = *gw32((u32)(uintptr_t)(g5));
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(g6)));
        goto L_00037044;

    L_00036eb4:
        g4 = i960_ld_u32(I960_WORKRAM, 0x213974, 0);
        /* @0x36EBC: cmpibe 0,g4,0x37048 */
        if (g4 == 0)
            goto L_00037048;
        g4 = i960_ld_u32(I960_WORKRAM, 0x20a560, 0);
        g13 = i960_ld_u32(I960_WORKRAM, 0x2141ec, 0);
        /* @0x36ED0: cmpibge 0,g4,0x37024 — skip follow/car vel scale when timer <= 0. */
        if ((i32)g4 <= 0)
            goto L_00037024;
        g7 = i960_ld_u32(I960_WORKRAM, 0x2139f8, 0);
        /* @0x36EDC: cmpibe 0,g7,0x37024 */
        if (g7 == 0)
            goto L_00037024;
        g6 = i960_ld_u32(I960_WORKRAM, 0x213980, 0);
        g4 = *gw32((u32)(uintptr_t)(g7));
        g5 = *gw32((u32)(uintptr_t)(g6));
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) - (i960_u32_to_f64(g5)));
        *(u32 *)(fp + 0x160) = (u32)g4;
        g4 = *gw32((u32)(uintptr_t)(g7 + 0x4));
        g5 = *gw32((u32)(uintptr_t)(g6 + 0x4));
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) - (i960_u32_to_f64(g5)));
        *(u32 *)(fp + 0x164) = (u32)g4;
        g4 = *gw32((u32)(uintptr_t)(g7 + 0x8));
        g5 = *gw32((u32)(uintptr_t)(g6 + 0x8));
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) - (i960_u32_to_f64(g5)));
        g10 = 0x2c805959u;
        i960_mmio_write_u32(0x884000, (u32)g10); /* copro_fifo */;
        *(u32 *)(fp + 0x168) = (u32)g4;
        g4 = *gw32((u32)(uintptr_t)(g7 + 0xc));
        g0 = *(u32 *)(fp + 0x160);
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g0); /* copro_fifo */;
        g4 = *gw32((u32)(uintptr_t)(g7 + 0x10));
        g6 = *(u32 *)(fp + 0x164);
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g6); /* copro_fifo */;
        g4 = *gw32((u32)(uintptr_t)(g7 + 0x14));
        g5 = *(u32 *)(fp + 0x168);
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
        g11 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        fp0 = i960_u32_to_f64(g11);
        fp1 = i960_u32_to_f64(g11);
        if (fp0 <= 0.0)
            goto L_00037024;
        g12 = 0x2c005858u;
        i960_mmio_write_u32(0x884000, (u32)g12); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g0); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g6); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
        g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(g4)));
        g4 = i960_f64_to_u32((fp1) / (i960_u32_to_f64(g4)));
        fp0 = i960_u32_to_f64(g4);
        r14 = 0x47ae147bu;
        r15 = 0x3f747ae1u;
        fp1 = i960_u32_to_f64(g4);
        fp1 = i960_rifl_read(r14, r15);
        i960_rifl_write(&g8, &g9, (fp1) * (fp0));
        fp1 = i960_u32_to_f64(g13);
        fp0 = i960_rifl_read(g8, g9);
        i960_rifl_write(&g10, &g11, (fp0) + (fp1));
        fp1 = i960_rifl_read(g10, g11);
        if (fp1 <= 1.0)
            goto L_0003701c;
        g4 = 0x3f800000u;
        goto L_00037020;

    L_0003701c:
        g4 = i960_f64_to_u32(fp1);

    L_00037020:
        g13 = g4;

    L_00037024:
        g11 = *(u32 *)(fp + 0x1f0);
        g5 = g11 + 0x2c;
        g4 = *gw32((u32)(uintptr_t)(g5));
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(g13)));
        *gw32((u32)(uintptr_t)(g5)) = (u32)g4;
        g5 = g11 + 0x34;
        g4 = *gw32((u32)(uintptr_t)(g5));
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(g13)));

    L_00037044:
        *gw32((u32)(uintptr_t)(g5)) = (u32)g4;

    L_00037048:
        g4 = i960_ld_u32(I960_WORKRAM, 0x214120, 0);
        if ((unsigned char)g4 != 2)
            goto L_000370ac;
        g12 = *(u32 *)(fp + 0x1f0);
        g5 = g12 + 0x2c;
        r14 = *gw32((u32)(uintptr_t)(g5));
        fp0 = i960_u32_to_f64(r14);
        fp0 = i960_u32_to_f64(r14);
        fp1 = i960_rifl_read(0x624dd2fu, 0x3fef9581u);
        i960_rifl_write(&g10, &g11, (fp1) * (fp0));
        fp0 = i960_rifl_read(g10, g11);
        g4 = i960_f64_to_u32(fp0);
        *gw32((u32)(uintptr_t)(g5)) = (u32)g4;
        g5 = g12 + 0x34;
        g11 = *gw32((u32)(uintptr_t)(g5));
        fp0 = i960_u32_to_f64(g11);
        fp0 = i960_u32_to_f64(g11);
        i960_rifl_write(&r14, &r15, (fp1) * (fp0));
        fp0 = i960_rifl_read(r14, r15);
        g4 = i960_f64_to_u32(fp0);
        *gw32((u32)(uintptr_t)(g5)) = (u32)g4;

    L_000370ac:
        r15 = 0x2c005858u;
        i960_mmio_write_u32(0x884000, (u32)r15); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
        g8 = *(u32 *)(fp + 0x1f0);
        i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
        g5 = g8 + 0x2c;
        g4 = *gw32((u32)(uintptr_t)(g5));
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        g6 = g8 + 0x30;
        g4 = *gw32((u32)(uintptr_t)(g6));
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        g7 = g8 + 0x34;
        g4 = *gw32((u32)(uintptr_t)(g7));
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        g9 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        fp0 = i960_u32_to_f64(g9);
        fp0 = i960_u32_to_f64(g9);
        fp3 = i960_rifl_read(0xa12f684cu, 0x3ff284bdu);
        if (fp0 <= fp3)
            goto L_000371a4;
        fp1 = i960_rifl_read(0xa12f684cu, 0x3ff284bdu);
        i960_rifl_write(&g8, &g9, (fp1) / (fp0));
        fp1 = i960_rifl_read(g8, g9);
        g9 = *gw32((u32)(uintptr_t)(g5));
        fp0 = i960_u32_to_f64(g9);
        fp0 = i960_u32_to_f64(g9);
        i960_rifl_write(&g10, &g11, (fp1) * (fp0));
        fp0 = i960_rifl_read(g10, g11);
        g4 = i960_f64_to_u32(fp0);
        *gw32((u32)(uintptr_t)(g5)) = (u32)g4;
        g11 = *gw32((u32)(uintptr_t)(g6));
        fp0 = i960_u32_to_f64(g11);
        fp0 = i960_u32_to_f64(g11);
        i960_rifl_write(&r14, &r15, (fp1) * (fp0));
        fp0 = i960_rifl_read(r14, r15);
        g4 = i960_f64_to_u32(fp0);
        *gw32((u32)(uintptr_t)(g6)) = (u32)g4;
        r15 = *gw32((u32)(uintptr_t)(g7));
        fp0 = i960_u32_to_f64(r15);
        fp0 = i960_u32_to_f64(r15);
        i960_rifl_write(&g8, &g9, (fp1) * (fp0));
        fp0 = i960_rifl_read(g8, g9);
        g4 = i960_f64_to_u32(fp0);
        *gw32((u32)(uintptr_t)(g7)) = (u32)g4;

    L_000371a4:
        g9 = *(u32 *)(fp + 0x1f0);
        r4 = g9 + 0x38;
        g0 = *gw32((u32)(uintptr_t)(r4));
        g0 = geo_view_float_clamp((u32)g0, (u32)g1, (u32)g2);
    r5 = g9 + 0x3c;
    *gw32((u32)(uintptr_t)(r4)) = (u32)g0;
    g0 = *gw32((u32)(uintptr_t)(r5));
    g0 = geo_view_float_clamp((u32)g0, (u32)g1, (u32)g2);
    r4 = g9 + 0x40;
    *gw32((u32)(uintptr_t)(r5)) = (u32)g0;
    g0 = *gw32((u32)(uintptr_t)(r4));
    g0 = geo_view_float_clamp((u32)g0, (u32)g1, (u32)g2);
    *gw32((u32)(uintptr_t)(r4)) = (u32)g0;
    g0 = *(u32 *)(fp + 0x1f0);
    geo_view_scene_epilogue((u32)g0, (u32)g1, (u32)g2);
    g8 = save_g8;
    g9 = save_g9;
    g10 = save_g10;
    g11 = save_g11;
    g12 = save_g12;
    sp = sp_save;
    fp = fp_save;
    return;
}
