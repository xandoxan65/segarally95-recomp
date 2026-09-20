/* Comm-board cabinet probe tail @ 0xCDF8 (game_cold_boot_init bal target). */
// @rom 0xcdf8 +0x40 comm_board_setup_probe

#include "i960_lift.h"
#include "i960_mem.h"

void comm_board_setup_probe(u32 arg0, u32 arg1, u32 arg2)
{
    u8 node_flag;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    i960_st_u8(I960_ROM, 0x1a14000, 0, (u8)g14);
    i960_st_u8(I960_ROM, 0x1a14002, 0, (u8)g14);

    node_flag = i960_ld_u8(I960_ROM, 0x1a14000, 0);
    if ((node_flag & 1u) == 0)
        i960_st_u32(I960_WORKRAM, 0x20a554, 0, (u32)g14);
    else
        i960_st_u32(I960_WORKRAM, 0x20a554, 0, 1);

    /* HW bx (g0) to 0x5ABE38 format table; static lift stops here. */
}
