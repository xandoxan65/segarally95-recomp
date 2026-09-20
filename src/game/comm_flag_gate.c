/* Comm flag gate @ 0x2200 (entry from game_subsys_boot @ 0x2C80). */
// @rom 0x2200 +0x14 comm_flag_gate

#include "i960_lift.h"
#include "i960_mem.h"

void comm_flag_gate(u32 arg0, u32 arg1, u32 arg2)
{
    u32 present;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    present = i960_ld_u32(I960_WORKRAM, 0x202090, 0);
    if (present == 0) {
        i960_call_rom(0x1d38);
        return;
    }
    i960_call_rom(0x2180);
}
