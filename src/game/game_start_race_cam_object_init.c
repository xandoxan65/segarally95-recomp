/* Race cam object init @ 0x34140 — logo_path after pool alloc into g0/0x2140cc.
 *
 * Seeds cam floats, links +4 → 0x5d3f40, fills +0x14/+0x38 via pose_table,
 * latches view mode, then scene epilogue into the 0x213b40 block.
 *
 * source: disasm/maincpu/maincpu_034140_280.asm */
// @rom 0x34140 +0x234 game_start_race_cam_object_init

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>

void game_start_race_cam_object_init(u32 arg0, u32 arg1, u32 arg2)
{
    u32 cam = arg0 != 0u ? arg0 : (u32)g0;
    u32 r4;
    u32 r5;
    u32 r6;
    u8 b;
    static int logged;

    (void)arg1;
    (void)arg2;

    if (!logged) {
        lift_log( "lift: race_cam_object_init cam=%#x\n", cam);
        fflush(stderr);
        logged = 1;
    }

    if (cam == 0u)
        return;

    r5 = cam;
    r6 = cam;

    /* @0x34140–0x34170: 0x214124 seed from 0x2020b4. */
    if (i960_ld_u32(I960_WORKRAM, 0x2020b4, 0) != 0u)
        i960_st_u32(I960_WORKRAM, 0x214124, 0, 0x3b659c2du);
    else
        i960_st_u32(I960_WORKRAM, 0x214124, 0, 0x3b37b024u);

    /* @0x34178–0x3421C: float / link defaults on the cam node. */
    i960_st_u32(I960_ABS, cam, 0x50, 0x447a0000u);
    i960_st_u32(I960_ABS, cam, 0x54, 0x448f4000u);
    i960_st_u32(I960_ABS, cam, 0x68, 0x3e4ccccdu);
    i960_st_u32(I960_ABS, cam, 0x70, 0x3e4ccccdu);
    i960_st_u32(I960_ABS, cam, 0x4, 0x005d3f40u);
    i960_st_u32(I960_ABS, cam, 0x58, 0x448f4000u);
    i960_st_u32(I960_ABS, cam, 0x5c, 0x43960000u);
    i960_st_u32(I960_ABS, cam, 0x64, 0x3e4ccccdu);
    i960_st_u32(I960_ABS, cam, 0x6c, 0x3e4ccccdu);

    {
        u32 lo = i960_ld_u32(I960_WORKRAM, 0x5c7890, 0);
        u32 hi = i960_ld_u32(I960_WORKRAM, 0x5c7890, 4);
        u32 z = i960_ld_u32(I960_WORKRAM, 0x5c7898, 0);

        i960_st_u32(I960_ABS, cam, 0x60, 0);
        i960_st_u32(I960_ABS, cam, 0x74, 0);
        i960_st_u32(I960_ABS, cam, 0x20, lo);
        i960_st_u32(I960_ABS, cam, 0x24, hi);
        b = (u8)i960_ld_u8(I960_ABS, cam, 0);
        b = (u8)((b & 0xc3u) | 4u);
        i960_st_u8(I960_ABS, cam, 0, b);
        i960_st_u32(I960_ABS, cam, 0x2c, lo);
        i960_st_u32(I960_ABS, cam, 0x30, hi);
        i960_st_u32(I960_ABS, cam, 0x34, z);
        i960_st_u32(I960_ABS, cam, 0x44, lo);
        i960_st_u32(I960_ABS, cam, 0x48, hi);
        i960_st_u32(I960_ABS, cam, 0x4c, z);
        i960_st_u32(I960_ABS, cam, 0x28, z);
    }

    i960_st_u32(I960_ABS, cam, 0x7c, 0x44480000u);
    i960_st_u32(I960_ABS, cam, 0x78, i960_ld_u32(I960_WORKRAM, 0x202044, 0));

    /* Clear bit7 @ cam+0x10; link 0x213b40 @ +0x8c; clear +0x84. */
    r4 = cam + 0x10u;
    b = (u8)i960_ld_u8(I960_ABS, r4, 0);
    i960_st_u8(I960_ABS, r4, 0, (u8)(b & 0x7fu));
    i960_st_u32(I960_ABS, cam, 0x8c, 0x00213b40u);
    i960_st_u32(I960_ABS, cam, 0x84, 0);

    b = (u8)i960_ld_u8(I960_ABS, cam, 0x11);
    i960_st_u8(I960_ABS, cam, 0x11, (u8)(b & 0xc3u));

    /* bal 0x21cd8: g0=&cam+0x14, g1=&cam+0x38. */
    game_start_race_cam_pose_table(cam + 0x14u, cam + 0x38u, 0);

    /* Merge low nibble of 0x2139cc into cam+0x10 (and with −16, or nibble). */
    {
        u8 nibble = (u8)(i960_ld_u8(I960_WORKRAM, 0x2139cc, 0) & 0x0fu);
        u8 flags = (u8)i960_ld_u8(I960_ABS, r4, 0);

        flags = (u8)((flags & 0xf0u) | nibble);
        i960_st_u8(I960_ABS, r4, 0, flags);
    }

    i960_st_u32(I960_WORKRAM, 0x2138c4, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x2138c0, 0, 0);

    /* Pack 0x20201b into cam+0x11 low 2 bits. */
    {
        u32 mode = i960_ld_u8(I960_WORKRAM, 0x20201b, 0);
        u8 dest = (u8)i960_ld_u8(I960_ABS, r6 + 0x11u, 0);

        if (mode != 0u) {
            mode = (mode - 1u) & 3u;
            dest = (u8)((dest & (u8)~3u) | (u8)mode);
        } else {
            dest = (u8)(dest & (u8)~3u);
        }
        i960_st_u8(I960_ABS, r6 + 0x11u, 0, dest);
    }

    /* stos −1 @ cam+0x8a / +0x88; stq zeros @ 0x2138a0; stq ones @ 0x2138b0. */
    i960_st_u16(I960_ABS, r5, 0x8a, (u16)0xffffu);
    i960_st_u16(I960_ABS, r5, 0x88, (u16)0xffffu);
    i960_st_u32(I960_WORKRAM, 0x214200, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x2138a0, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x2138a4, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x2138a8, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x2138ac, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x2138b0, 0, 0x3f800000u);
    i960_st_u32(I960_WORKRAM, 0x2138b4, 0, 0x3f800000u);
    i960_st_u32(I960_WORKRAM, 0x2138b8, 0, 0x3f800000u);
    i960_st_u32(I960_WORKRAM, 0x2138bc, 0, 0x3f800000u);

    /* call 0x34ce0 with g0 = −1.0f; restore cam. */
    g0 = 0xbf800000u;
    geo_view_mode_latch((u32)g0, 0, 0);

    g0 = r5;
    /* @0x377e0: ROM row → 0x2141c0/d0/e0 + latch/control/params. */
    geo_view_table_seed(r5, 0, 0);

    g0 = r5;
    geo_view_scene_epilogue((u32)g0, 0, 0);

    {
        static int pose_logged;

        if (!pose_logged) {
            lift_log(
                    "lift: race_cam_object_init 213b40 xyz=(%.3g,%.3g,%.3g)\n",
                    i960_u32_to_f64(i960_ld_u32(I960_ABS, 0x00213b40u, 0)),
                    i960_u32_to_f64(i960_ld_u32(I960_ABS, 0x00213b40u, 4)),
                    i960_u32_to_f64(i960_ld_u32(I960_ABS, 0x00213b40u, 8)));
            fflush(stderr);
            pose_logged = 1;
        }
    }

    i960_st_u32(I960_WORKRAM, 0x2140c4, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x2140c8, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20b0b4, 0, 0);

    g0 = r5;
}
