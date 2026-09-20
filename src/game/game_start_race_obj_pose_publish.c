/* Node → pose publish @ 0x2BC08 — bal from road-attach 0x2C8D0.
 * Copies node drive fields into the pose block at node+0x8c (0x213b40+slot).
 * source: disasm/maincpu/maincpu_02bc08_100.asm */
// @rom 0x2bc08 +0xe0 game_start_race_obj_pose_publish

#include "i960_lift.h"
#include "i960_mem.h"

void game_start_race_obj_pose_publish(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0;
    u32 pose;
    u32 w0, w1;
    u8 b0, b1;

    (void)arg1;
    (void)arg2;

    if (node == 0u)
        return;

    pose = i960_ld_u32(I960_ABS, node, 0x8c);
    if (pose == 0u)
        return;

    /* @0x2BC14–0x2BC40: xyz + secondary + angles → pose. */
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

    i960_st_u32(I960_ABS, pose, 0x2c, i960_ld_u32(I960_ABS, node, 0x84));
    i960_st_u32(I960_ABS, pose, 0x28, i960_ld_u32(I960_ABS, node, 0x80));
    i960_st_u32(I960_ABS, pose, 0x24, i960_ld_u32(I960_ABS, node, 0x74));
    i960_st_u32(I960_ABS, pose, 0x30, i960_ld_u32(I960_ABS, node, 0x64));
    i960_st_u32(I960_ABS, pose, 0x34, i960_ld_u32(I960_ABS, node, 0x68));
    i960_st_u32(I960_ABS, pose, 0x38, i960_ld_u32(I960_ABS, node, 0x6c));
    i960_st_u32(I960_ABS, pose, 0x3c, i960_ld_u32(I960_ABS, node, 0x70));
    i960_st_u32(I960_ABS, pose, 0x40, i960_ld_u32(I960_ABS, node, 0xb4));
    i960_st_u32(I960_ABS, pose, 0x44, i960_ld_u32(I960_ABS, node, 0xb8));
    i960_st_u32(I960_ABS, pose, 0x48, i960_ld_u32(I960_ABS, node, 0xbc));
    i960_st_u32(I960_ABS, pose, 0x4c, i960_ld_u32(I960_ABS, node, 0xc0));

    /* @0x2BCA0–0x2BCBC: merge node+17 flags into pose+0x51. */
    b0 = i960_ld_u8(I960_ABS, node + 17u, 0);
    b1 = i960_ld_u8(I960_ABS, pose + 0x51u, 0);
    b0 = (u8)(b0 & 0x3fu);
    b1 = (u8)(b1 & (u8)(3u << 6));
    i960_st_u8(I960_ABS, node + 17u, 0, (u8)(b0 | b1));

    /* @0x2BCC0–0x2BCE0: pose+0x50 ← node+0x10; set bit7; pose+0x54 ← node+0x88. */
    {
        u32 slot50 = pose + 0x50u;
        u32 v = i960_ld_u32(I960_ABS, node, 0x10);

        i960_st_u32(I960_ABS, slot50, 0, v);
        b0 = i960_ld_u8(I960_ABS, slot50, 0);
        i960_st_u32(I960_ABS, pose, 0x54, i960_ld_u32(I960_ABS, node, 0x88));
        i960_st_u8(I960_ABS, slot50, 0, (u8)(b0 | (u8)(1u << 7)));
    }
}
