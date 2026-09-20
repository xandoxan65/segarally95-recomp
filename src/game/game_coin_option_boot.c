/* Coin / mode option boot @ 0x2A20 — called from game_subsys_boot when
 * mode-table checksum mismatches or 0x202012 != 36.
 * source: disasm/maincpu/maincpu_002a00_80.asm */
// @rom 0x2a20 +0x18 game_coin_option_boot

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"

void game_coin_option_boot(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x2A20: bal 0x2978; bal 0xA948; st g14,0x202014; call 0x2930 */
    game_option_flags_init(0, 0, 0);
    coin_option_defaults(0, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x202014, 0, 0);
    game_mode_index_step(0, 0, 0);
    /* Host: ensure chute masks/credits (normally @ 0x2A58 → A9D0). */
    game_coin_sram_reset(0, 0, 0);
}
