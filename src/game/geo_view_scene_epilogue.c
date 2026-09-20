/* Scene epilogue @ 0x34078 — node → pose, then height samples from 0x2138a0.
 *
 * Same copy family as pose_publish @ 0x2BC08, but:
 *   pose+0x50/54 first from node+0x10 / +0x88 (no bit7 merge, no +0x51 flags)
 *   pose+0x40..0x4c ← workram 0x2138a0..0x2138ac (vec_push samples), not node+0xb4
 *
 * source: ROM 0x34078..0x34130 (disasm/maincpu/maincpu_034078_80.asm + remainder) */
// @rom 0x34078 +0xbc geo_view_scene_epilogue

#include "i960_lift.h"
#include "i960_mem.h"

void geo_view_scene_epilogue(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0;
    u32 pose;
    u32 w0, w1;

    (void)arg1;
    (void)arg2;

    /* @0x34078: mov g14,g1; mov 0,g14; ld 0x8c(g0),g4. */
    pose = i960_ld_u32(I960_ABS, node, 0x8c);

    /* @0x34084–0x34090: pose+0x50 ← node+0x10; pose+0x54 ← node+0x88. */
    i960_st_u32(I960_ABS, pose, 0x50, i960_ld_u32(I960_ABS, node, 0x10));
    i960_st_u32(I960_ABS, pose, 0x54, i960_ld_u32(I960_ABS, node, 0x88));

    /* @0x34094–0x340C0: xyz / secondary / angles. */
    w0 = i960_ld_u32(I960_ABS, node, 0x14);
    w1 = i960_ld_u32(I960_ABS, node, 0x18);
    i960_st_u32(I960_ABS, pose, 0, w0);
    i960_st_u32(I960_ABS, pose, 4, w1);
    i960_st_u32(I960_ABS, pose, 8, i960_ld_u32(I960_ABS, node, 0x1c));

    w0 = i960_ld_u32(I960_ABS, node, 0x2c);
    w1 = i960_ld_u32(I960_ABS, node, 0x30);
    i960_st_u32(I960_ABS, pose, 0xc, w0);
    i960_st_u32(I960_ABS, pose, 0x10, w1);
    i960_st_u32(I960_ABS, pose, 0x14, i960_ld_u32(I960_ABS, node, 0x34));

    w0 = i960_ld_u32(I960_ABS, node, 0x38);
    w1 = i960_ld_u32(I960_ABS, node, 0x3c);
    i960_st_u32(I960_ABS, pose, 0x18, w0);
    i960_st_u32(I960_ABS, pose, 0x1c, w1);
    i960_st_u32(I960_ABS, pose, 0x20, i960_ld_u32(I960_ABS, node, 0x40));

    i960_st_u32(I960_ABS, pose, 0x28, i960_ld_u32(I960_ABS, node, 0x80));
    i960_st_u32(I960_ABS, pose, 0x2c, i960_ld_u32(I960_ABS, node, 0x84));
    i960_st_u32(I960_ABS, pose, 0x24, i960_ld_u32(I960_ABS, node, 0x74));
    i960_st_u32(I960_ABS, pose, 0x30, i960_ld_u32(I960_ABS, node, 0x64));
    i960_st_u32(I960_ABS, pose, 0x34, i960_ld_u32(I960_ABS, node, 0x68));
    i960_st_u32(I960_ABS, pose, 0x38, i960_ld_u32(I960_ABS, node, 0x6c));

    /* @0x340F4: ld 0x70(g0),g0; st g0,0x3c(g4). */
    i960_st_u32(I960_ABS, pose, 0x3c, i960_ld_u32(I960_ABS, node, 0x70));

    /* @0x340FC–0x34128: pose+0x40..0x4c ← 0x2138a0..0x2138ac. */
    i960_st_u32(I960_ABS, pose, 0x40, i960_ld_u32(I960_WORKRAM, 0x2138a0, 0));
    i960_st_u32(I960_ABS, pose, 0x44, i960_ld_u32(I960_WORKRAM, 0x2138a4, 0));
    i960_st_u32(I960_ABS, pose, 0x48, i960_ld_u32(I960_WORKRAM, 0x2138a8, 0));
    i960_st_u32(I960_ABS, pose, 0x4c, i960_ld_u32(I960_WORKRAM, 0x2138ac, 0));
}
