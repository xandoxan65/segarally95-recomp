/* Matrix dir emit @ 0x29120 — chase / alt-chase helper.
 *
 * After object yaw/pitch/roll on the TGP matrix:
 *   0x26 readback → zero T → 0x20 push → 0x22 reload R|T=0 →
 *   0x2c(g0,g1,g2) xform → write XYZ to GEO 0x804000 → 0x21 pop.
 *
 * Chase passes g0=g1=0xbf34fdf4, g2=0x3f34fdf4 before the call.
 * Private host frame for fp locals (0x40..0x78).
 *
 * source: /tmp/dasm_29120c/maincpu_029120_180.asm + tail @ 0x29258 */
// @rom 0x29120 +0x1a0 geo_view_matrix_dir_emit

#include "i960_lift.h"
#include "i960_mem.h"

#include <string.h>

void geo_view_matrix_dir_emit(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    u8 frame[0x80];
    u32 m[12];
    u32 ox, oy, oz;
    unsigned i;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    sp = sp + 0x50u;

    /* Keep incoming scale/dir args in g0..g2 (chase sets them pre-call). */
    g0 = arg0;
    g1 = arg1;
    g2 = arg2;

    /* @0x29130: TGP 0x26 → 12-word current matrix into 0x50(fp).. */
    i960_mmio_write_u32(0x884000, 0x13002626u);
    for (i = 0; i < 12u; i++)
        m[i] = i960_mmio_read_u32(0x884000);

    /* Disasm zeros translation slots (indices 9..11) before reload. */
    m[9] = 0u;
    m[10] = 0u;
    m[11] = 0u;

    for (i = 0; i < 9u; i++) {
        u32 v = m[i];

        memcpy(frame + 0x50u + i * 4u, &v, 4);
    }
    /* 0x74/0x78 held T then cleared; keep zeros at 0x74..0x7c. */

    /* @0x291E8: push, load zero-T matrix, xform (g0,g1,g2). */
    i960_mmio_write_u32(0x884000, 0x10002020u);
    i960_mmio_write_u32(0x884000, 0x11002222u);
    for (i = 0; i < 12u; i++) {
        u32 v = 0u;

        if (i < 9u)
            memcpy(&v, frame + 0x50u + i * 4u, 4);
        i960_mmio_write_u32(0x884000, v);
    }

    i960_mmio_write_u32(0x884000, 0x16002c2cu);
    i960_mmio_write_u32(0x884000, (u32)g0);
    i960_mmio_write_u32(0x884000, (u32)g1);
    i960_mmio_write_u32(0x884000, (u32)g2);

    ox = i960_mmio_read_u32(0x884000);
    oy = i960_mmio_read_u32(0x884000);
    oz = i960_mmio_read_u32(0x884000);

    /* @0x2927C–0x29298: poke GEO ports; g14→0x8000a0. */
    i960_mmio_write_u32(0x8000a0, (u32)g14);
    i960_mmio_write_u32(0x804000, ox);
    i960_mmio_write_u32(0x804000, oy);
    i960_mmio_write_u32(0x804000, oz);

    /* @0x292A0: pop matrix saved by 0x20. */
    i960_mmio_write_u32(0x884000, 0x10802121u);

    sp = sp_save;
    fp = fp_save;
}
