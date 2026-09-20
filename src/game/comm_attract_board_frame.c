/* Comm-attract per-frame credit / start gate @ 0xF6D0 (board_dispatch @ 0xFDAC).
 * Free-play (query < 0): START (202064 bit4) → mode 3.
 * Paid plays (query > 0): spend one play → mode 3 (ROM auto-start; no START).
 * source: disasm/maincpu/maincpu_00f6c0_150.asm */
// @rom 0xf6d0 +0xf0 comm_attract_board_frame

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>

void comm_attract_board_frame(u32 arg0, u32 arg1, u32 arg2)
{
    u32 board_type;
    u32 credits;
    u32 start_game;
    u8 out;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    board_type = i960_ld_u32(I960_WORKRAM, 0x20a530, 0);
    if (board_type == 3u) {
        /* Linked-cabinet peer scan @ 0xF71C — no link on host. */
        return;
    }

    /* ROM: mov 0,g0 before bal BE68 — always chute 0 for local credit. */
    credits = attract_credit_query(0, 0, 0);

    /*
     * F6EC cmpible 0,g0 → paid/zero path @ F704.
     * Free-play (g0 < 0) falls through to START bit4 @ F6F0.
     * Paid (g0 > 0) spends @ BEA0 then sets start (F6FC).
     */
    if ((i32)credits < 0) {
        start_game = (i960_ld_u8(I960_WORKRAM, 0x202064, 0) & 0x10u) != 0;
    } else if ((i32)credits > 0) {
        attract_credit_spend(0, 0, 0);
        start_game = 1;
    } else {
        start_game = 0;
    }

    if (start_game == 0)
        return;

    out = (u8)i960_ld_u8(I960_WORKRAM, 0x20204c, 0);
    out = (u8)(out & ~(1u << 2));
    out = (u8)(out & 0x7fu);
    out = (u8)(out & ~(1u << 5));
    i960_st_u8(I960_WORKRAM, 0x20204c, 0, out);

    i960_st_u32(I960_WORKRAM, 0x20209c, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x202098, 0, 3);
    fprintf(stderr, "lift: game start — mode 3 (plays left %u)\n",
            (unsigned)i960_ld_u16(I960_ABS, 0x01d00022u, 0));
}
