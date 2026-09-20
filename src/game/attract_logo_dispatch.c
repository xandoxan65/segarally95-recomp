/* Attract logo / digit CGM gate @ 0x32F70.
 * g0==0 → sega_mini submit; g0==1 → digit refresh path @ 0x32EB0.
 * source: disasm/maincpu/maincpu_032f70_40.asm */
// @rom 0x32f70 +0x30 attract_logo_dispatch

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"

void attract_logo_dispatch(u32 arg0, u32 arg1, u32 arg2)
{
    u32 gate;

    (void)arg1;
    (void)arg2;

    g0 = arg0;
    gate = i960_ld_u32(I960_WORKRAM, 0x2139c0, 0);
    if ((i32)gate >= 1)
        return;
    if ((u8)g0 == 0) {
        attract_logo_cgm_submit(0, 0, 0);
        return;
    }
    if ((u8)g0 == 1) {
        /* Digit/refresh path — still mostly unlifted; call ROM body. */
        i960_call_rom(0x32eb0);
        return;
    }
}
