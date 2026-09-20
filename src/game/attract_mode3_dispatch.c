/* Mode-3 attract sub-dispatch @ 0x1B8C0 — callx 0x5BA7D0[20209c&15].
 * source: disasm/maincpu/maincpu_01b800_150.asm */
// @rom 0x1b8c0 +0x50 attract_mode3_dispatch

#include "i960_lift.h"
#include "i960_mem.h"
#include "i960_host_staging.h"
#include "model2_rom.h"

#define MODE3_JUMP_TABLE 0x005ba7d0u

void attract_mode3_dispatch(u32 arg0, u32 arg1, u32 arg2)
{
    u32 board;
    u32 index;
    u32 handler;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    board = i960_ld_u32(I960_WORKRAM, 0x20a530, 0);
    if ((u8)board == 3) {
        i960_call_rom(0x16500);
        return;
    }

    index = i960_ld_u32(I960_WORKRAM, 0x20209c, 0) & 15u;
    handler = i960_ld_u32(I960_WORKRAM, MODE3_JUMP_TABLE, index << 2);
    if (handler == 0)
        handler = model2_workram_mirror_u32(MODE3_JUMP_TABLE + (index << 2));
    if (handler == 0) {
        i960_st_u32(I960_WORKRAM, 0x20209c, 0, (u32)g14);
        i960_st_u32(I960_WORKRAM, 0x202098, 0, 2);
        return;
    }
    if (!i960_host_staging_call_lifted(handler))
        i960_call_indirect(handler);
}
