/* String-length tile blanker @ 0x271D8 (bal link in g14).
 * Writes tile 0 for each non-NUL byte at g0, advancing cursor X (cap 60).
 * source: disasm/maincpu/maincpu_0271d8_78.asm */
// @rom 0x271d8 +0x78 boot_tile_script_format

#include "i960_lift.h"
#include "i960_mem.h"

void boot_tile_script_format(u32 arg0, u32 arg1, u32 arg2)
{
    u32 row;
    u32 col;
    u32 map;
    u8 ch;
    u32 limit;

    (void)arg1;
    (void)arg2;

    g0 = arg0;
    row = i960_ld_u32(I960_WORKRAM, 0x20b1ac, 0);
    col = i960_ld_u32(I960_WORKRAM, 0x20b1a8, 0);
    ch = i960_ld_u8(I960_ABS, (u32)g0, 0);
    g0 = (u32)g0 + 1u;
    limit = 31u + 29u; /* 60 */
    map = 0x01000000u + (((row << 6) + col) << 1);
    if (ch == 0)
        return;

    for (;;) {
        col = i960_ld_u32(I960_WORKRAM, 0x20b1a8, 0);
        i960_st_u16(I960_ABS, map, 0, 0);
        if (col > limit)
            return;
        col = col + 1u;
        i960_st_u32(I960_WORKRAM, 0x20b1a8, 0, col);
        map = map + 2u;
        ch = i960_ld_u8(I960_ABS, (u32)g0, 0);
        g0 = (u32)g0 + 1u;
        if (ch == 0)
            return;
    }
}
