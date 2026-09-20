/* Semantic lift: FIFO upload runner @ ROM 0x2A5A0 (D/U bit-0 → merge). */
/* source: decomp/disasm/maincpu/maincpu_02a5a0_140.asm */
// @rom 0x2a5a0 +0x130 cgm_fifo_upload_runner

#include "i960_lift.h"

extern void palram_bus_merge_host(u32 g0, u32 g1, u32 g2);

/*
 * @0x2A5A0–0x2A6CC: Bresenham walk of (g0,g1)→(g2,g3). When entry g5 bit 0
 * is set (D/U), each step calls ``0x2A4E0`` with ``g2`` = payload (entry g4).
 * No XOR on this path. Returns rotated flag word in g0.
 */
u32 cgm_fifo_upload_runner(u32 arg0, u32 arg1, u32 arg2)
{
    i32 x0 = (i32)arg0;
    i32 y0 = (i32)arg1;
    i32 x1 = (i32)arg2;
    i32 y1 = (i32)g3;
    i32 dx = x1 - x0;
    i32 dy = y1 - y0;
    u32 payload = (u32)g4;
    u32 flags = (u32)g5;
    i32 abs_dx = dx < 0 ? -dx : dx;
    i32 abs_dy = dy < 0 ? -dy : dy;
    i32 step_x = dx < 0 ? -1 : 1;
    i32 step_y = dy < 0 ? -1 : 1;
    i32 x = x0;
    i32 y = y0;
    i32 err;
    i32 two_dx;
    i32 two_dy;
    u32 i;
    u32 n;

    /* @0x2A5F8 cmpible abs_dx,abs_dy → Y-major when abs_dx <= abs_dy. */
    if (abs_dx > abs_dy) {
        /* X-major @ 0x2A5FC. */
        err = -abs_dx;
        two_dy = abs_dy * 2;
        two_dx = abs_dx * 2;
        n = (u32)abs_dx;
        for (i = 0; i < n; i++) {
            if ((flags & 1u) != 0)
                palram_bus_merge_host((u32)x, (u32)y, payload);
            err += two_dy;
            x += step_x;
            if (err >= 0) {
                err -= two_dx;
                y += step_y;
            }
            /* @0x2A648–0x2A654: shro 1 | shlo 15 on flag word. */
            flags = ((flags >> 1) & 0x7fffu) | ((flags << 15) & 0x8000u);
        }
    } else {
        /* Y-major @ 0x2A664. */
        err = -abs_dy;
        two_dx = abs_dx * 2;
        two_dy = abs_dy * 2;
        n = (u32)abs_dy;
        for (i = 0; i < n; i++) {
            if ((flags & 1u) != 0)
                palram_bus_merge_host((u32)x, (u32)y, payload);
            err += two_dx;
            y += step_y;
            if (err >= 0) {
                err -= two_dy;
                x += step_x;
            }
            flags = ((flags >> 1) & 0x7fffu) | ((flags << 15) & 0x8000u);
        }
    }

    g0 = flags;
    return flags;
}
