/* Mode-3 table slot 3 @ 0x14440 — championship / practice select loop.
 * Was staged as null; unlifted callx was a silent host no-op, so the
 * select icons (0x13ce0 → 0x14530) never ran after display_setup.
 * source: disasm/maincpu/maincpu_014440_80.asm */
// @rom 0x14440 +0x80 game_start_mode_select

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>

extern u32 game_start_select_timer_gate(void);
extern void game_start_mode_select_frame(u32 arg0, u32 arg1, u32 arg2);

void game_start_mode_select(u32 arg0, u32 arg1, u32 arg2)
{
    u32 gate;
    u32 sub;
    u32 choice;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    geo_fifo_bootstrap(0, 0, 0);
    game_start_mode_select_frame(0, 0, 0);

    if (i960_ld_u8(I960_WORKRAM, 0x20201a, 0) == 0u)
        geo_view_scene_apply(0, 0, 0);

    i960_st_u32(I960_WORKRAM, 0x2139c0, 0, 1u);

    gate = game_start_select_timer_gate();
    /* @0x14468: non-zero gate → stay on submode 3. */
    if (gate != 0u)
        return;

    /* Timer exhausted — clear 0x202049 (ROM bal 0x39678) and advance. */
    i960_st_u8(I960_WORKRAM, 0x202049, 0, 0);

    choice = i960_ld_u32(I960_WORKRAM, 0x202230, 0);
    /* @0x1447c/@0x14484: stos g14 → 0x20b914 / 0x20b918 (scroll disable). */
    i960_st_u16(I960_WORKRAM, 0x20b914, 0, 0);
    i960_st_u16(I960_WORKRAM, 0x20b918, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x214354, 0, 0);

    sub = i960_ld_u32(I960_WORKRAM, 0x20209c, 0);
    if (choice == 0u)
        sub += 1u;
    else
        sub += 3u;
    i960_st_u32(I960_WORKRAM, 0x20209c, 0, sub);
    fprintf(stderr, "lift: mode_select done choice=%u → submode %u\n",
            (unsigned)choice, (unsigned)sub);
}
