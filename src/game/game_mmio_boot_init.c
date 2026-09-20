/* IRQ/timer MMIO bootstrap @ 0x1788 (game_cold_boot_init tail). */
// @rom 0x1788 +0x78 game_mmio_boot_init

#include "i960_lift.h"
#include "i960_mem.h"

void game_mmio_boot_init(u32 arg0, u32 arg1, u32 arg2)
{
    u32 irq_enable;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    g0 = g14;
    g14 = 0;
    i960_mmio_write_u32(0xe80000, (u32)g14);
    g5 = 21;
    i960_st_u32(I960_WORKRAM, 0x20200c, 0, (u32)g5);
    irq_enable = i960_ld_u32(I960_WORKRAM, 0x20200c, 0);
    i960_mmio_write_u32(0xe80004, irq_enable);
    g5 = 0x61a80u;
    i960_mmio_write_u32(0xf00000, (u32)g5);
    i960_mmio_write_u32(0xf00004, (u32)g5);
    g5 = (u32)-1;
    i960_mmio_write_u32(0xf00008, (u32)g5);
    i960_mmio_write_u32(0xf0000c, (u32)g5);
    i960_st_u32(I960_WORKRAM, 0x202000, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x202008, 0, (u32)g14);
    g5 = 1;
    i960_st_u32(I960_WORKRAM, 0x202004, 0, (u32)g5);
    /* bx (g0) */
}
