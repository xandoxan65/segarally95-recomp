/* Attract mode reset gate @ 0x26228 (comm_board_dispatch @ 0xCDC0 bal). */
// @rom 0x26228 +0x18 comm_attract_reset

#include "i960_lift.h"
#include "i960_mem.h"

void comm_attract_reset(u32 arg0, u32 arg1, u32 arg2)
{
    u32 gate;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    gate = i960_ld_u32(I960_WORKRAM, 0x20b180, 0);
    if (gate == 0)
        return;

    /* HW bx (g4); static lift no-op. */
}
