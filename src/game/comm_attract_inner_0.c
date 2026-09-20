/* Comm-attract inner mode 0 @ 0xF840 (jump table slot 0 @ 0x5AE480). */
// @rom 0xf840 +0x130 comm_attract_inner_0

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"

void comm_attract_inner_0(u32 arg0, u32 arg1, u32 arg2)
{
    u32 slots;
    u32 link;
    u32 end;
    u32 type;
    u32 counter;
    u8 flags;
    u32 inner;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    slots = i960_ld_u8(I960_WORKRAM, 0x20a581, 0);
    i960_st_u32(I960_WORKRAM, 0x20a7bc, 0, (u32)g14);

    if (slots != 0) {
        link = 0x1a121c2u;
        end = link + (((slots << 3) - slots) << 6);
        while (link < end) {
            type = i960_ld_u16(I960_ABS, link, 0);
            if (type < 4u) {
                counter = i960_ld_u32(I960_WORKRAM, 0x20a7bc, 0);
                i960_st_u32(I960_WORKRAM, 0x20a7bc, 0, counter + 1);
            }
            link = i960_ld_u32(I960_ABS, link, 0x1c0);
            if (link == 0 || link >= end)
                break;
        }
    }

    comm_draw_setup(0, 0, 0);
    boot_tile_map_init(0, 0, 0);
    tile_attract_palram_gate(0xffffu);

    i960_st_u32(I960_WORKRAM, 0x213970, 0, 0x42340000u);
    i960_st_u32(I960_WORKRAM, 0x20a78c, 0, 0x005b1b70u);
    i960_st_u32(I960_WORKRAM, 0x213974, 0, 15u);
    i960_st_u32(I960_WORKRAM, 0x20a780, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x20a560, 0, (u32)g14);
    i960_st_u8(I960_WORKRAM, 0x202049, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a564, 0, 2u);

    comm_attract_bal_trampoline(0, 0, 0);

    i960_st_u32(I960_WORKRAM, 0x20a7c8, 0, 1u);
    i960_st_u32(I960_WORKRAM, 0x20a7cc, 0, 5u);
    /* First inner_1 countdown tick uses script record[1] → inner 2 @ record+0xC. */
    i960_st_u32(I960_WORKRAM, 0x20a7dc, 0, 1u);

    flags = i960_ld_u8(I960_WORKRAM, 0x20204c, 0);
    inner = i960_ld_u32(I960_WORKRAM, 0x20209c, 0);
    flags &= (u8)~32u;
    i960_st_u8(I960_WORKRAM, 0x20204c, 0, flags);
    flags = i960_ld_u8(I960_WORKRAM, 0x20204c, 0);
    i960_st_u32(I960_WORKRAM, 0x20a770, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x20a7c0, 0, (u32)g14);
    inner++;
    i960_st_u32(I960_WORKRAM, 0x20209c, 0, inner);
    flags &= 0x7fu;
    i960_st_u8(I960_WORKRAM, 0x20204c, 0, flags);
}
