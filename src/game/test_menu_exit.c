/* EXIT TEST MODE @ 0x7FD0 — menu slot 10.
 * source: disasm/maincpu/maincpu_007fd0_200.asm */
// @rom 0x7fd0 +0x148 test_menu_exit

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>

static void printf_guest(u32 fmt_va)
{
    uintptr_t fp_save = fp;

    fp = 0;
    libc_printf((const char *)(uintptr_t)fmt_va, (u32)g1, (u32)g2);
    fp = fp_save;
}

/* g0!=0: enter/init; g0==0: frame. Exit sets mode 0 (leave test menu). */
u32 test_menu_exit(u32 arg0, u32 arg1, u32 arg2)
{
    u32 plays;
    u32 timer;
    u32 flags;

    (void)arg1;
    (void)arg2;

    if (arg0 == 0)
        goto frame;

    tile_map_banks_clear((u32)g0, (u32)g1, (u32)g2);
    plays = i960_ld_u32(I960_WORKRAM, 0x202014, 0);
    if (plays > 0x270fu) {
        tile_cursor_seed(17u, 10u);
        boot_tile_script_run(0x5a6f20u);
        boot_tile_script_run(0x5a6f40u);
        boot_tile_script_run(0x5a6f70u);
        boot_tile_script_run(0x5a6fa0u);
        timer = (u32)-1;
    } else {
        tile_cursor_seed(24u, 20u);
        printf_guest(0x5a6f10u);
        timer = 31u + 29u;
    }
    i960_st_u32(I960_WORKRAM, 0x20a2f0, 0, timer);
    scene_list_seed((u32)g0, (u32)g1, (u32)g2);
    i960_call_rom(0xa768);
    if (i960_ld_u32(I960_WORKRAM, 0x20a2f4, 0) != 0) {
        i960_st_u32(I960_WORKRAM, 0x213850, 0, (u32)g14);
        i960_st_u32(I960_WORKRAM, 0x213840, 0, (u32)g14);
        i960_call_rom(0x4bc0);
        i960_st_u32(I960_WORKRAM, 0x213850, 0, (u32)g14);
        i960_st_u32(I960_WORKRAM, 0x213840, 0, 1u);
    }
    if (i960_ld_u32(I960_WORKRAM, 0x20a400, 0) != 0)
        game_mode_index_step((u32)g0, (u32)g1, (u32)g2);

frame:
    timer = i960_ld_u32(I960_WORKRAM, 0x20a2f0, 0);
    if (timer == (u32)-1) {
        g0 = 1;
        geo_scene_row_print((u32)g0, (u32)g1, (u32)g2);
        flags = i960_ld_u32(I960_WORKRAM, 0x202074, 0);
        if (flags & 1u)
            goto leave;
        flags = i960_ld_u8(I960_WORKRAM, 0x202064, 0);
        if ((flags >> 2) & 1u)
            goto leave;
        flags = i960_ld_u8(I960_WORKRAM, 0x202064, 0);
        if ((flags >> 4) & 1u)
            goto leave;
        g0 = 0;
        return 0;
    }

    timer--;
    i960_st_u32(I960_WORKRAM, 0x20a2f0, 0, timer);
    if (timer == (u32)-1)
        goto leave;
    g0 = 0;
    return 0;

leave:
    /* Store 0 into mode/submode — leave operator menu for attract. */
    i960_st_u32(I960_WORKRAM, 0x202098, 0, 0);
    g0 = 0;
    i960_st_u32(I960_WORKRAM, 0x20209c, 0, 0);
    lift_log( "lift: EXIT TEST MODE → attract\n");
    return 0;
}
