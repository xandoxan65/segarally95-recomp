/* Per-IRQ coin / credit frame @ 0xAF00.
 * When mode != 4: chute scan + service edge; always credit apply; then AA58.
 * source: disasm/maincpu/maincpu_00af00_150.asm */
// @rom 0xaf00 +0x64 game_coin_frame

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"

void game_coin_frame(u32 arg0, u32 arg1, u32 arg2)
{
    u32 mode;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    mode = i960_ld_u32(I960_WORKRAM, 0x202098, 0);
    i960_st_u32(I960_WORKRAM, 0x20a518, 0, 0);

    if (mode != 4u) {
        game_coin_chute_scan(0, 0, 0);
        game_coin_service_edge(0, 0, 0);
    }
    game_coin_credit_apply(0, 0, 0);

    mode = i960_ld_u32(I960_WORKRAM, 0x202098, 0);
    if (mode != 4u) {
        u32 gate = i960_ld_u32(I960_WORKRAM, 0x20a518, 0);

        if (gate != 0
            && ((i960_ld_u8(I960_WORKRAM, 0x202024, 0) >> 3) & 1u) == 0) {
            g0 = (gate == 2u) ? 0x82u : 0x81u;
            g1 = 0x7f;
            tile_texture_descriptor_apply((u32)g0, (u32)g1, 0);
        }
    }

    /* @0xAF5C bal 0xAA58 — coin lamp/timer housekeeping; leave unlifted. */
    i960_call_rom(0xaa58);
}
