/* Comm-board absent xfer wait @ 0x1CB8 (comm_board_idle_gate callee). */
// @rom 0x1cb8 +0x78 comm_board_absent_xfer

#include "i960_lift.h"
#include "i960_mem.h"

void comm_board_absent_xfer(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    g1 = g14;
    g14 = 0;
    /* Lift: skip MMIO poll/xfer loop; bx (g1). */
}
