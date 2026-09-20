/* Desert race list-node init @ 0x43790 — staged as 0x5E2790.
 *
 * First callx from list walk @ 0x2AB80: seed node floats/flags, then replace
 * +4 with 0x5E17A0 (ROM 0x427A0 per-frame). Host returns to the walk (ROM
 * bx's a bal trampoline @ 0x5E2828 → ret).
 *
 * source: disasm/maincpu/maincpu_043790_200.asm */
// @rom 0x43790 +0x98 game_start_race_desert_node_init

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

#include <math.h>

void game_start_race_desert_node_init(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0;
    double s;
    double d;
    u32 out;

    (void)arg1;
    (void)arg2;

    g14 = 0;

    /* @0x437A0–0x437D0: sqrtr → sinr → divrl (double imm / sin). */
    s = sqrt(i960_u32_to_f64(0x3d798ae7u));
    s = sin(s);
    d = i960_rifl_read(0x684bda13u, 0x3feda12fu);
    if (s != 0.0)
        d = d / s;
    out = (u32)i960_f64_to_u32(d);

    /* Pool nodes live @ WORKRAM_BASE (0x500000+); same ABS path as cam_object_init. */
    i960_st_u32(I960_ABS, node, 0x70, 0x41200000u); /* 10.0f */
    i960_st_u16(I960_ABS, node, 0x54, 0);
    i960_st_u32(I960_ABS, node, 0x60, 0);
    i960_st_u32(I960_ABS, node, 0x64, 0);
    i960_st_u32(I960_ABS, node, 0x68, 0);
    i960_st_u32(I960_ABS, node, 0x6c, 0);
    i960_st_u32(I960_ABS, node, 0x14, 0);
    i960_st_u32(I960_ABS, node, 0x38, 0);
    i960_st_u32(I960_ABS, node, 0x34, 0);
    i960_st_u32(I960_ABS, node, 0x5c, 0x447a0000u); /* 1000.0f */
    i960_st_u32(I960_ABS, node, 0x30, 0);
    /* @0x43810–0x4381C: next callx → per-frame @ ROM 0x427A0. */
    i960_st_u32(I960_ABS, node, 4, 0x005e17a0u);
    i960_st_u32(I960_ABS, node, 0x74, out);
}
