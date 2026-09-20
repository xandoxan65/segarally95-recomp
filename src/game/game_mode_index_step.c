/* Mode index + table hash step @ 0x2930 (boot_attract_flags_init callee). */
// @rom 0x2930 +0x40 game_mode_index_step

#include "i960_lift.h"
#include "i960_mem.h"

void game_mode_index_step(u32 arg0, u32 arg1, u32 arg2)
{
    u32 mode_index;
    u32 table_cursor;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    mode_index = i960_ld_u32(I960_WORKRAM, 0x202014, 0);
    table_cursor = i960_ld_u32(I960_WORKRAM, 0x202012, 0);
    i960_st_u16(I960_WORKRAM, 0x202012, 0, 36u);
    mode_index++;
    i960_st_u32(I960_WORKRAM, 0x202014, 0, mode_index);
    g0 = table_cursor;
    g1 = 34;
    i960_call_rom(0x297e0);
    i960_st_u16(I960_WORKRAM, 0x202010, 0, (u16)g0);
    i960_call_rom(0x2220);
}
