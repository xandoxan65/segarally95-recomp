/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_011a80_108.asm */
// @rom 0x11a80 +0x108 geo_attract_copro_vec_scale

#include "i960_lift.h"
#include "i960_fp.h"
#include "model2_rom.h"
#include "i960_mem.h"

#include <stdio.h>
#include <string.h>

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret  callee r6 */
/* abi: u32 arg0=g0, void * arg1=g1, void * arg2=g2 → void */
/* call site: caller 0x123b0, g0=scale, g1=0x20a7e0, g2=fp+0x40 */
/*
 * Mode 7/8: `lda 0x40(fp),g2` then `call 0x11a80`. On i960, `call` allocates a
 * new frame — callee `0x40(fp)` is NOT the caller's scene slot. g2 still points
 * at the caller's scene (resolved before the call).
 *
 * Host lift shares one global `fp`, so without a private frame the unit stores
 * at 0x40(fp)/0x44(fp) clobber a2 and the later `st g7,0x48(fp)` overwrites
 * eye.z with unit.z. That is a lift bug, not ROM aliasing.
 *
 * Correct eye: scene − scale·normalize(delta) on all three axes. Host:
 * 0x2a005454 stores delta; 0x17802f2f returns normalize; 0x13802727 loads −eye.
 */

void geo_attract_copro_vec_scale(u32 arg0, void * arg1, void * arg2)
{
    u32 * a1 = (u32 *)arg1;
    u32 * a2 = (u32 *)arg2;
    u32 scale = arg0;
    u32 ux, uy, uz;
    u32 px, py, pz;
    float eye_x, eye_y, eye_z;
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    /* Callee frame — register save is 0x00..0x3c; temps at 0x40+ like disasm. */
    u8 frame[0x50];

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    sp = sp + 16;

    g4 = *(u32 *)a1;
    g5 = *(u32 *)((uintptr_t)a1 + 0x4);
    g6 = *(u32 *)((uintptr_t)a1 + 0x8);
    r8 = (uintptr_t)i960_vaddr_ptr(0x2a005454);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g6); /* copro_fifo */;
    r8 = (uintptr_t)i960_vaddr_ptr(0x17802f2f);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    g4 = *(u32 *)a1;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g4 = *(u32 *)((uintptr_t)a1 + 0x4);
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g4 = *(u32 *)((uintptr_t)a1 + 0x8);
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;

    /* @0x11AEC–0x11B0C: unit → callee 0x40(fp)/0x44(fp); uz stays in g7. */
    ux = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    *(u32 *)(fp + 0x40) = ux;
    uy = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    *(u32 *)(fp + 0x44) = uy;
    uz = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    g7 = uz;

    /* @0x11B10–0x11B1C: (scale*uz, scale*ux, scale*uy). */
    g4 = i960_f64_to_u32((i960_u32_to_f64(uz)) * (i960_u32_to_f64(scale)));
    g13 = i960_f64_to_u32((i960_u32_to_f64(ux)) * (i960_u32_to_f64(scale)));
    g0 = i960_f64_to_u32((i960_u32_to_f64(uy)) * (i960_u32_to_f64(scale)));

    /* @0x11B20–0x11B3C: reload scene through a2 (caller pointer — not this fp). */
    px = *(u32 *)a2;
    py = *(u32 *)((uintptr_t)a2 + 4);
    pz = *(u32 *)((uintptr_t)a2 + 8);
    px = i960_f64_to_u32((i960_u32_to_f64(px)) - (i960_u32_to_f64(g13)));
    py = i960_f64_to_u32((i960_u32_to_f64(py)) - (i960_u32_to_f64(g0)));
    pz = i960_f64_to_u32((i960_u32_to_f64(pz)) - (i960_u32_to_f64(g4)));
    *(u32 *)a2 = px;
    *(u32 *)((uintptr_t)a2 + 4) = py;
    *(u32 *)((uintptr_t)a2 + 8) = pz;

    g5 = px ^ (1u << 31);
    g4 = py ^ (1u << 31);
    g6 = pz ^ (1u << 31);
    r8 = (uintptr_t)i960_vaddr_ptr(0x13802727);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    /* @0x11B68: unit.z → callee 0x48(fp) only (discarded on ret; caller keeps eye.z). */
    *(u32 *)(fp + 0x48) = (u32)g7;
    i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g6); /* copro_fifo */;

    {
        static unsigned s_vs;

        if (s_vs < 6u) {
            eye_x = (float)i960_u32_to_f64(px);
            eye_y = (float)i960_u32_to_f64(py);
            eye_z = (float)i960_u32_to_f64(pz);
            fprintf(stderr,
                    "lift: vec_scale scale=%.3g unit=(%.3g,%.3g,%.3g) "
                    "cam_pt=(%.3g,%.3g,%.3g)\n",
                    i960_u32_to_f64(scale),
                    i960_u32_to_f64(ux), i960_u32_to_f64(uy), i960_u32_to_f64(uz),
                    eye_x, eye_y, eye_z);
            s_vs++;
        }
    }

    sp = sp_save;
    fp = fp_save;
}
