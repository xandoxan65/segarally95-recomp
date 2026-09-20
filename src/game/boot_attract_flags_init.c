/* Attract flag bootstrap @ 0x3260 (cold boot when 0x20A554==0). */
// @rom 0x3260 +0x3c boot_attract_flags_init

#include "i960_lift.h"
#include "i960_mem.h"

void boot_attract_flags_init(u32 arg0, u32 arg1, u32 arg2)
{
    u8 flag_b;
    u8 flag_a;
    u32 call_setup;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    call_setup = 0;
    flag_b = (u8)i960_ld_u8(I960_WORKRAM, 0x20201b, 0);
    if (flag_b == 0) {
        call_setup = 1;
        i960_st_u8(I960_WORKRAM, 0x20201b, 0, (u8)g14);
    }
    flag_a = (u8)i960_ld_u8(I960_WORKRAM, 0x20201a, 0);
    if (flag_a == 0) {
        call_setup = 1;
        i960_st_u8(I960_WORKRAM, 0x20201a, 0, (u8)g14);
    }
    if (call_setup)
        i960_call_rom(0x2930);
}
