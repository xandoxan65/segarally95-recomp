/* bal return trampoline @ 0x16B28 (comm-attract inner handlers @ 0xF900). */
// @rom 0x16b28 +0x14 comm_attract_bal_trampoline

#include "i960_lift.h"
#include "i960_mem.h"

void comm_attract_bal_trampoline(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* mov g14,g0; mov 0,g14; st g14,0x20a978; bx (g0) — C caller resumes after call. */
    g14 = 0;
    i960_st_u32(I960_WORKRAM, 0x20a978, 0, 0);
}
