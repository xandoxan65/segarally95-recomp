/* BX return stub @ 0x1D38. */
// @rom 0x1d38 +0x8 comm_bx_return_g1

#include "i960_lift.h"

void comm_bx_return_g1(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg2;
    g1 = arg1;
}
