/* BX return stub @ 0x1A58. */
// @rom 0x1a58 +0x8 comm_bx_return_g0

#include "i960_lift.h"

void comm_bx_return_g0(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg1;
    (void)arg2;
    g0 = arg0;
}
