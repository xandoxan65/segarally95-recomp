/* Attract phase 1 @ 0xC750 — node / slave ID setup (table @ 0x5AB730). */
// @rom 0xc750 +0x50 tile_attract_phase_slave

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_rom.h"

void tile_attract_phase_slave(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node_flag;
    u32 phase_index;
    u32 resume;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    resume = 0x005ab7d4u;
    node_flag = i960_ld_u8(I960_ROM, 0x1a14000, 0);
    if ((node_flag & 1u) != 0) {
        i960_st_u32(I960_WORKRAM, 0x20a524, 0, 0x005ab730u);
        i960_st_u32(I960_WORKRAM, 0x20a520, 0,
                    model2_workram_mirror_u32(0x005ab730u + 0x12cu));
        i960_st_u32(I960_WORKRAM, 0x20209c, 0, 5);
        (void)resume;
        return;
    }

    if (i960_ld_u32(I960_WORKRAM, 0x20a580, 0) == 0)
        return;

    i960_st_u8(I960_ROM, 0x1a14002, 0, 1);
    phase_index = i960_ld_u32(I960_WORKRAM, 0x20209c, 0);
    i960_st_u8(I960_ROM, 0x1a14000, 0, 1);
    i960_st_u32(I960_WORKRAM, 0x20209c, 0, phase_index + 1);
}
