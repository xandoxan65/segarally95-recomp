/* Comm-board presence probe @ 0x1C60 (game_cold_boot_init). */
// @rom 0x1c60 +0x48 comm_board_probe

#include "i960_lift.h"
#include "i960_mem.h"

void comm_board_probe(u32 arg0, u32 arg1, u32 arg2)
{
    u32 mmio;
    u32 present;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x1C64–0x1C88: write then read 'M' @ 0x01C00202; present if loopback. */
    i960_st_u16(I960_ABS, 0x01c00202u, 0, 0x004du);
    mmio = i960_ld_u16(I960_ABS, 0x01c00202u, 0);
    mmio &= 0xffu;
    present = (mmio == 0x004du) ? 1u : 0u;
    i960_st_u32(I960_WORKRAM, 0x202090, 0, present);

    if (present == 0)
        i960_call_rom(0x1bd0);
    else
        i960_call_rom(0x1c50);
}
