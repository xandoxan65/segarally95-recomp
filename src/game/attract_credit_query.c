/* Playable-credit query @ 0xBE68 (bal entry; lda return @ 0xBE60).
 * g0 = chute index; returns chute[g0] / price_unit @ 0x202028, or -1 if free-play.
 * source: disasm/maincpu/maincpu_00be00_200.asm */
// @rom 0xbe68 +0x38 attract_credit_query

#include "i960_lift.h"
#include "i960_mem.h"

u32 attract_credit_query(u32 chute_index, u32 arg1, u32 arg2)
{
    u8 flags;
    u16 chute;
    u16 unit;

    (void)arg1;
    (void)arg2;

    flags = (u8)i960_ld_u8(I960_WORKRAM, 0x202024, 0);
    if ((flags & 8u) != 0) {
        g0 = (u32)-1; /* free-play */
        return (u32)g0;
    }

    chute = (u16)i960_ld_u16(I960_ABS, 0x01d00022u + (chute_index * 16u), 0);
    unit = (u16)i960_ld_u16(I960_WORKRAM, 0x202028, 0);
    if (unit == 0)
        unit = 1;
    g0 = (u32)(chute / unit);
    return (u32)g0;
}
