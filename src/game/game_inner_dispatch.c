/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_0001aa30_120.asm */
// @rom 0x1aa30 +0x170 game_inner_dispatch

#include "i960_lift.h"
#include "i960_mem.h"
#include "i960_host.h"
#include "lift_syms.h"

#include <stdio.h>

void game_inner_dispatch(u32 arg0, u32 arg1, u32 arg2)
{
    u32 mode;
    u32 submode;
    u32 countdown;
    u32 main_mode;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    if (i960_host_dispatch_halted())
        return;

    mode = i960_ld_u32(I960_WORKRAM, 0x20209c, 0);
    if (mode == 0) {
        /* @0xCDC0 comm_board_dispatch resets 20209c to 0 while 20AA20 stays 3
         * (boot promote). Re-running the cold-init clear would wipe copyright. */
        submode = i960_ld_u32(I960_WORKRAM, 0x20aa20, 0);
        if (submode < 3u) {
            game_inner_flags_clear(0, 0, 0);
            tile_map_banks_clear(0, 0, 0);
            scene_list_seed(0, 0, 0);
            g0 = 0;
            scene_view_scale(0, 0, 0);
            /* @ 0x1AA58: cmpibe 0,g4,0x1AA70 — branch when latch clear (skip sound init). */
            if (i960_ld_u32(I960_WORKRAM, 0x20aa24, 0) == 0) {
                i960_st_u32(I960_WORKRAM, 0x20aa20, 0, 3);
            } else {
                i960_st_u32(I960_WORKRAM, 0x20aa24, 0, (u32)g14);
                i960_st_u32(I960_WORKRAM, 0x20aa20, 0, (u32)g14);
            }
        }
        mode = i960_ld_u32(I960_WORKRAM, 0x20209c, 0);
        i960_st_u32(I960_WORKRAM, 0x20209c, 0, mode + 1);
    }

    submode = i960_ld_u32(I960_WORKRAM, 0x20aa20, 0);

    /* @ 0x1AAAC: submode 3 promotes main mode 0→1 (cmpibe 3 → 0x1AB84). */
    if (submode == 3) {
        main_mode = i960_ld_u32(I960_WORKRAM, 0x202098, 0);
        i960_st_u32(I960_WORKRAM, 0x20209c, 0, (u32)g14);
        i960_st_u32(I960_WORKRAM, 0x202098, 0, main_mode + 1);
        return;
    }

    if (submode == 1)
        goto countdown_phase;

    if (submode == 2)
        goto wait_phase_b;

    if (submode != 0)
        return;

    /* submode 0 @ 0x1AAB4: "SOUND INITIALIZE ..." when 20AA24 was set by 0x1A7B8. */
    g0 = 20;
    g1 = 20;
    tile_cursor_seed(20, 20);
    g0 = 0x005b9a10u;
    i960_host_boot_clock_mark();
    fprintf(stderr, "lift: boot — SOUND INITIALIZE start\n");
    boot_tile_script_run(0x005b9a10u);
    /* @0x1AAD4 shlo 4,15,g5 → 240 frames before copyright @ 0x1A990 */
    i960_st_u32(I960_WORKRAM, 0x2021f4, 0, 15u << 4);
    goto advance_submode;

countdown_phase:
    if (i960_host_boot_fast_countdown()) {
        i960_st_u32(I960_WORKRAM, 0x2021f4, 0, (u32)-1);
        countdown = (u32)-1;
    } else {
        countdown = i960_ld_u32(I960_WORKRAM, 0x2021f4, 0);
        countdown = countdown - 1u;
        i960_st_u32(I960_WORKRAM, 0x2021f4, 0, countdown);
    }
    /* @0x1AB04: cmpibne g4, -1 — copyright only after counter wraps 0 → -1. */
    if (countdown != (u32)-1)
        return;
    tile_map_banks_clear(0, 0, 0);
    /* @0x1AB0C: ldob 0x202019 — do not clear COUNTRY (prior lift wrongly stos 0). */
    {
        u8 show_gate = i960_ld_u8(I960_WORKRAM, 0x202019, 0);

        if (show_gate == 0)
            boot_copyright_notice_show(0, 0, 0);
        /* @0x1AB1C: lda 0x258 when gate clear; @0x1AB24: mov 3 otherwise. */
        if (show_gate == 0)
            i960_st_u32(I960_WORKRAM, 0x2021f4, 0, 0x258u);
        else
            i960_st_u32(I960_WORKRAM, 0x2021f4, 0, 3);
    }
    goto advance_submode;

wait_phase_b:
    countdown = i960_ld_u32(I960_WORKRAM, 0x2021f4, 0);
    if (countdown > 0) {
        i960_st_u32(I960_WORKRAM, 0x2021f4, 0, countdown - 1);
        return;
    }

advance_submode:
    submode = i960_ld_u32(I960_WORKRAM, 0x20aa20, 0);
    i960_st_u32(I960_WORKRAM, 0x20aa20, 0, submode + 1);
}
