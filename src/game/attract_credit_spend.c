/* Spend one playable credit @ 0xBEA0.
 * g0 = chute index. On success: chute -= unit @ 0x202028, then re-query via BE68.
 * On failure (chute < unit): returns -1.
 * source: disasm/maincpu/maincpu_00be00_200.asm */
// @rom 0xbea0 +0x40 attract_credit_spend

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

u32 attract_credit_spend(u32 chute_index, u32 arg1, u32 arg2)
{
    u32 chute_ea;
    u16 chute;
    u16 unit;

    (void)arg1;
    (void)arg2;

    chute_ea = 0x01d00022u + (chute_index * 16u);
    unit = (u16)(i960_ld_u16(I960_WORKRAM, 0x202028, 0) & 0xffffu);
    chute = (u16)i960_ld_u16(I960_ABS, chute_ea, 0);
    if (chute < unit) {
        g0 = (u32)-1;
        return (u32)g0;
    }

    i960_st_u16(I960_ABS, chute_ea, 0, (u16)(chute - unit));
    return attract_credit_query(chute_index, 0, 0);
}
