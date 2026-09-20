/* Game-start phase 0 @ 0x1B4D0 — mode-3 table slot 0 (staged 0x5BA4D0).
 * Clears view flags, runs start-init @ 0x1ABB0, advances 20209c by 2.
 * source: disasm/maincpu/maincpu_01b4d0_100.asm */
// @rom 0x1b4d0 +0x80 game_start_phase_0

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>

void game_start_phase_0(u32 arg0, u32 arg1, u32 arg2)
{
    u32 board;
    u32 sub;
    u8 out;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    board = i960_ld_u32(I960_WORKRAM, 0x20a530, 0);
    i960_st_u32(I960_WORKRAM, 0x213840, 0, 6);
    i960_st_u32(I960_WORKRAM, 0x213850, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20227c, 0, 0);

    if (board != 0u) {
        /* @0x1B4F8: bbc 7 of packed IN0 @ 0x20205c — linked/cabinet gate. */
        if (((i960_ld_u8(I960_WORKRAM, 0x20205c, 0) >> 7) & 1u) == 0) {
            i960_st_u32(I960_WORKRAM, 0x20a564, 0, 3);
            if (i960_ld_u32(I960_WORKRAM, 0x20a550, 0) != 2u)
                return;
            /* @0x1B56C+ layer-reg path — leave for a later slice. */
            return;
        }
    }

    i960_st_u32(I960_WORKRAM, 0x20a564, 0, 4);
    game_start_flags_init(0, 0, 0);

    out = (u8)i960_ld_u8(I960_WORKRAM, 0x20204c, 0);
    out = (u8)(out & 0x7fu);
    i960_st_u8(I960_WORKRAM, 0x20204c, 0, out);
    i960_st_u32(I960_WORKRAM, 0x20a560, 0, (u32)-1);

    sub = i960_ld_u32(I960_WORKRAM, 0x20209c, 0) + 2u;
    i960_st_u32(I960_WORKRAM, 0x20209c, 0, sub);

    lift_log( "lift: game_start_phase_0 → submode %u\n", (unsigned)sub);
}
