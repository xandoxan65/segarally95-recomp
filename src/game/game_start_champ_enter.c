/* Championship path after mode-select @ 0x1C090 — table slot 4.
 * Clears choice/scene latches, then advances 20209c → slot 5.
 * source: disasm/maincpu bytes @ 0x1c090..0x1c0d8 */
// @rom 0x1c090 +0x50 game_start_champ_enter

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>

void game_start_champ_enter(u32 arg0, u32 arg1, u32 arg2)
{
    u32 sub;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x1C090–0x1C0AC: stos g14 → choice / scene latches. */
    i960_st_u32(I960_WORKRAM, 0x202230, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x2020b8, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x2020ac, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20aab4, 0, 0);

    /* @0x1C0B0 bal 0x1E898 / @0x1C0B4 bal 0x1C7E8 — clear scratch words. */
    i960_st_u32(I960_WORKRAM, 0x20af90, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20af98, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20aea8, 0, 0);

    i960_st_u32(I960_WORKRAM, 0x2020c8, 0, 14u);
    sub = i960_ld_u32(I960_WORKRAM, 0x20209c, 0) + 1u;
    i960_st_u32(I960_WORKRAM, 0x20209c, 0, sub);
    fprintf(stderr, "lift: champ_enter → submode %u\n", (unsigned)sub);
}
