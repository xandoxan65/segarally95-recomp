/* Service-credit edge @ 0xAE28 — release of IN0 bit3 (SERVICE1) on 0x202068,
 * or bit1 of start-path release @ 0x202078. Increments chute @ 0x01D00022.
 * source: disasm/maincpu/maincpu_00ae28_100.asm */
// @rom 0xae28 +0xd8 game_coin_service_edge

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_rom.h"

void game_coin_service_edge(u32 arg0, u32 arg1, u32 arg2)
{
    u8 flags;
    u32 n;
    u32 i;
    u32 chute;
    u16 cur;
    u32 limit;
    u32 bumped;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    flags = (u8)i960_ld_u8(I960_WORKRAM, 0x202024, 0);
    if ((flags & 8u) != 0)
        return;

    if (((i960_ld_u8(I960_WORKRAM, 0x202068, 0) >> 3) & 1u) == 0
        && ((i960_ld_u32(I960_WORKRAM, 0x202078, 0) >> 1) & 1u) == 0)
        return;

    n = ((flags & 1u) != 0) ? 2u : 1u;
    bumped = 0;
    chute = 0x01d00022u;
    for (i = 0; i < n; i++) {
        u32 ix = ((u32)i960_ld_u8(I960_WORKRAM, 0x202019, 0) << 2) & 12u;

        limit = model2_workram_mirror_u32(0x005a9410u + ix);
        cur = (u16)i960_ld_u16(I960_ABS, chute, 0);
        if (cur < limit) {
            i960_st_u16(I960_ABS, chute, 0, (u16)(cur + 1u));
            bumped = 1;
        }
        chute += 16u;
    }

    if (!bumped)
        return;

    i960_st_u32(I960_WORKRAM, 0x20a518, 0, 2);
    i960_st_u16(I960_ABS, 0x01d00008u, 0,
                (u16)(i960_ld_u16(I960_ABS, 0x01d00008u, 0) + 1u));
    i960_st_u32(I960_ABS, 0x01d00040u, 0,
                i960_ld_u32(I960_ABS, 0x01d00040u, 0) + 1u);
}
