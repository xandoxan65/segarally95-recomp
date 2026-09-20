/* Per-chute coin pulse @ 0xAB10 — g0 = chute index 0/1.
 * Gated by release edges @ 0x202040 & slot mask @ 0x01D00014[g0*16]+2.
 * source: disasm/maincpu/maincpu_00ab10_120.asm */
// @rom 0xab10 +0x118 game_coin_chute_slot

#include "i960_lift.h"
#include "i960_mem.h"

void game_coin_chute_slot(u32 chute, u32 arg1, u32 arg2)
{
    u8 flags;
    u32 slot13;
    u32 slot2;
    u32 mask;
    u32 edge;
    u16 price;
    u16 acc;
    u32 p;
    u32 q;

    (void)arg1;
    (void)arg2;
    g0 = chute;

    flags = (u8)i960_ld_u8(I960_WORKRAM, 0x202024, 0);
    slot13 = 0x01d00014u + (chute * 16u);
    if ((flags & 1u) == 0)
        slot2 = 0x01d00014u;
    else
        slot2 = slot13;

    edge = i960_ld_u8(I960_WORKRAM, 0x202040, 0) & 0xffu;
    mask = i960_ld_u16(I960_ABS, slot13 + 2u, 0);
    if ((edge & mask) == 0)
        return;

    price = (u16)i960_ld_u16(I960_WORKRAM, 0x20202c, 0);
    if (price != 0) {
        p = slot2 + 10u;
        acc = (u16)i960_ld_u16(I960_ABS, p, 0);
        acc = (u16)(acc + i960_ld_u16(I960_WORKRAM, 0x202030u + chute * 2u, 0));
        i960_st_u16(I960_ABS, p, 0, acc);
        if ((acc & 0xffffu) >= price) {
            q = slot2 + 12u;
            i960_st_u16(I960_ABS, p, 0, (u16)(acc - price));
            i960_st_u16(I960_ABS, q, 0,
                        (u16)(i960_ld_u16(I960_ABS, q, 0) + 1u));
        }
    }

    p = slot2 + 12u;
    acc = (u16)(i960_ld_u16(I960_ABS, p, 0)
                + i960_ld_u16(I960_WORKRAM, 0x202030u + chute * 2u, 0));
    i960_st_u16(I960_ABS, p, 0, acc);

    if ((flags & 2u) != 0)
        q = slot2 + 4u;
    else
        q = slot13 + 4u;
    i960_st_u16(I960_ABS, q, 0, (u16)(i960_ld_u16(I960_ABS, q, 0) + 1u));

    {
        u32 meter = 0x01d0000cu + (chute * 4u);
        u32 v = i960_ld_u32(I960_ABS, meter, 0);

        if (v <= 0xfffffffeu)
            v += 1u;
        i960_st_u32(I960_ABS, meter, 0, v);
    }

    i960_st_u32(I960_WORKRAM, 0x20a518, 0, 1);
    i960_st_u32(I960_ABS, 0x01d0003cu, 0,
                i960_ld_u32(I960_ABS, 0x01d0003cu, 0) + 1u);
}
