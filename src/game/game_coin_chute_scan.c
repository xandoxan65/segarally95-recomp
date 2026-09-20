/* Scan coin chutes 0..1 @ 0xAC30 → call 0xAB10 each.
 * source: disasm/maincpu/maincpu_00ac30_80.asm */
// @rom 0xac30 +0x18 game_coin_chute_scan

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"

void game_coin_chute_scan(u32 arg0, u32 arg1, u32 arg2)
{
    u32 i;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    for (i = 0; i < 2u; i++)
        game_coin_chute_slot(i, 0, 0);

    /*
     * AB10 consumes release edges but does not clear them. If coin_frame runs
     * again before the next sense sample, the same edge would re-credit.
     * Hardware IRQ pairs sense→frame once per vblank; clear after the scan.
     */
    i960_st_u8(I960_WORKRAM, 0x202040, 0, 0);
}
