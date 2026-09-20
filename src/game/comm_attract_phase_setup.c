/* Comm attract phase setup tail @ 0x137D0 (comm_attract_board_dispatch @ 0xFB58). */
// @rom 0x137d0 +0x40 comm_attract_phase_setup

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_rom.h"

void comm_attract_phase_setup(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    i960_call_rom(0x16b70);
    i960_call_rom(0x12b70);
    i960_call_rom(0x16b40);

    /* lda 0x5b1cc0 → store address, not *contents. */
    i960_st_u32(I960_WORKRAM, 0x20a78c, 0, 0x005b1cc0u);

    i960_st_u32(I960_WORKRAM, 0x20a770, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x20a7c0, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x20209c, 0, 9u);
}
