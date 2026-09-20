/* Car-node road attach @ 0x2C850 — called per entry from 0x217E0.
 * Copies table floats into node+0x14, packs slot words, then 0x2C8D0
 * (road sampler; still call_rom until lifted).
 * source: /tmp/dasm_2c850.asm */
// @rom 0x2c850 +0x74 game_start_race_obj_car_attach

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

void game_start_race_obj_road_attach(u32 arg0, u32 arg1, u32 arg2);

void game_start_race_obj_car_attach(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0;
    u32 src = arg1;
    u32 dest;
    u32 w;
    u16 s;
    u8 lo;
    u8 hi;
    u32 saved_node;
    u32 saved_src;

    (void)arg2;

    if (node == 0u || src == 0u)
        return;

    /* @0x2C850: ldl 0x8(g1) → stl (g0+20). */
    {
        u32 a = i960_ld_u32(I960_ABS, src, 8);
        u32 b = i960_ld_u32(I960_ABS, src, 12);

        i960_st_u32(I960_ABS, node, 0x14, a);
        i960_st_u32(I960_ABS, node, 0x18, b);
    }
    w = i960_ld_u32(I960_ABS, src, 0x10);
    saved_node = node;
    saved_src = src;
    i960_st_u16(I960_ABS, node, 0x8a, 0);
    dest = node + 0x88u;
    i960_st_u32(I960_ABS, node, 0x1c, w);
    i960_st_u16(I960_ABS, dest, 0, 0);

    g0 = node + 0x14u;
    g1 = dest;
    g0 = geo_view_slot_word_pack((u32)g0, (u32)g1, 0);

    s = i960_ld_u16(I960_ABS, dest, 0);
    i960_st_u32(I960_ABS, saved_node, 0xe4, (u32)s);
    w = i960_ld_u32(I960_ABS, saved_src, 4);
    i960_st_u32(I960_ABS, saved_node, 0xec, w);

    hi = i960_ld_u8(I960_ABS, saved_node + 0x10u, 0);
    lo = i960_ld_u8(I960_ABS, saved_src, 0);
    hi = (u8)(hi & (u8)~0x0fu);
    lo = (u8)(lo & 0x0fu);
    i960_st_u8(I960_ABS, saved_node + 0x10u, 0, (u8)(hi | lo));

    g0 = saved_node;
    game_start_race_obj_road_attach((u32)g0, 0, 0);
}
