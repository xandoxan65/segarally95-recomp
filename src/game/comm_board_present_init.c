/* Comm-board present init @ 0x1C50. */
// @rom 0x1c50 +0x10 comm_board_present_init

#include "i960_lift.h"

void comm_board_present_init(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    i960_call_rom(0x1a58);
    i960_call_rom(0x2180);
}
