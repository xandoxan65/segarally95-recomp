/* Coin SRAM / chute reset @ 0xA9D0 — clears credits and seeds COIN1/COIN2 masks.
 * MAME shlo g6,1,g4 → g4 = 1<<g6 (masks 1 and 2).
 * source: disasm/maincpu/maincpu_00a9d0_80.asm */
// @rom 0xa9d0 +0x74 game_coin_sram_reset

#include "i960_lift.h"
#include "i960_mem.h"

void game_coin_sram_reset(u32 arg0, u32 arg1, u32 arg2)
{
    u32 i;
    u32 credits_ea = 0x01d00020u;
    u32 chute_ea = 0x01d00022u;
    u32 mask_ea = 0x01d00016u;
    u32 hdr_ea = 0x01d00014u;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    for (i = 0; i < 2u; i++) {
        u16 mask = (u16)(1u << i);

        i960_st_u16(I960_ABS, credits_ea, 0, 0);
        i960_st_u16(I960_ABS, chute_ea, 0, 0);
        i960_st_u16(I960_ABS, mask_ea, 0, mask);
        i960_st_u16(I960_ABS, hdr_ea, 0, mask);
        /* Clear related slot words A9D0 zeros (timers / counters). */
        i960_st_u16(I960_ABS, chute_ea - 6u, 0, 0);
        i960_st_u16(I960_ABS, chute_ea - 8u, 0, 0);
        i960_st_u16(I960_ABS, chute_ea - 10u, 0, 0);

        credits_ea += 16u;
        chute_ea += 16u;
        mask_ea += 16u;
        hdr_ea += 16u;
    }

    /* @0xAA3C–0xAA40 bal 0xA768 / 0xA718 — leave unlifted. */
    i960_call_rom(0xa768);
    i960_call_rom(0xa718);
}
