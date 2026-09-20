/* Boot System-24 tile splash: sync regs + char RAM upload @ 0x26690. */
// @rom 0x26690 +0xb8 boot_tile_splash_init

#include "i960_lift.h"
#include "i960_mem.h"

#define TILE_CHAR_BASE    0x01080000u
#define TILE_SYNC_RODATA  0x0000FFACu
#define TILE_FONT_SOURCE  0x005C5250u
#define TILE_BANK_STRIDE  0x00001000u

void boot_tile_splash_init(u32 arg0, u32 arg1, u32 arg2)
{
    u32 bank;
    u32 char_dest;
    u16 hsync;
    u16 vsync;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    hsync = (u16)i960_ld_u16(I960_ROM, TILE_SYNC_RODATA, 0);
    i960_st_u16(I960_ABS, 0x01040000u, 0, hsync);
    vsync = (u16)i960_ld_u8(I960_ROM, TILE_SYNC_RODATA, 0x52);
    i960_st_u16(I960_ABS, 0x01060000u, 0, vsync);

    char_dest = TILE_CHAR_BASE;
    for (bank = 0; bank <= 7u; bank++) {
        g0 = TILE_FONT_SOURCE;
        g1 = char_dest;
        g2 = 0x80u; /* setbit 7,0 — 128 source rows */
        g3 = 1u;
        i960_call_rom(0x26b90);
        char_dest += TILE_BANK_STRIDE;
    }

    g0 = TILE_FONT_SOURCE;
    g1 = char_dest;
    g2 = 0x80u;
    g3 = 1u;
    g4 = 2u;
    i960_call_rom(0x26c40);

    i960_call_rom(0x26a10);

    i960_st_u32(I960_WORKRAM, 0x20b910, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x20b1f4, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x20b1f0, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x20b900, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x20b1a0, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x20b1ac, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x20b1a8, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x20b1b0, 0, (u32)g14);
}
