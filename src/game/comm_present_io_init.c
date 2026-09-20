/* Comm present IO setup @ 0x2180. */
// @rom 0x2180 +0x8 comm_present_io_init

#include "i960_lift.h"

void comm_present_io_init(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    i960_call_rom(0x1e20);
}
