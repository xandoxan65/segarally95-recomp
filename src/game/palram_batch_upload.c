/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_02a200_80.asm */
// @rom 0x2a200 +0x80 palram_batch_upload

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret */
/* abi: u32 arg0=g0, u32 arg1=g1, u32 arg2=g2 → void */

void palram_batch_upload(u32 arg0, u32 arg1, u32 arg2)
{
    u32 tile_base;
    u32 row_skip;
    u32 g13_mask;

    if (g4 & 1) {
        tile_base = (((u32)arg1 << 6) + (u32)arg0) * 2u + 0x01004000u;
    } else {
        tile_base = (((u32)arg1 << 6) + (u32)arg0) * 2u + 0x01000000u;
    }

    g7 = 0;
    if ((signed)g3 <= 0)
        return;

    /* @0x2A234–0x2A240: g13 scratch = 0x40 then 0x8000 — XOR mask is bit15 only. */
    row_skip = ((0x40u - (arg2 & 0xffffu)) & 0xffffu) << 1;
    g13_mask = 0x8000u;

    L_0002a244:
        g5 = 0;
        if ((signed)g2 > 0) {
            do {
                u16 raw;
                u16 entry;

                raw = i960_ld_u16(I960_ABS, tile_base, 0);
                entry = (u16)(raw ^ (u16)g13_mask);
                i960_st_u16(I960_ABS, tile_base, 0, entry);
                tile_base = tile_base + 2u;
                g5 = g5 + 1u;
            } while (g5 < (u32)g2);
        }
        g7 = g7 + 1u;
        tile_base = tile_base + row_skip;
        if (g7 < (u32)g3)
            goto L_0002a244;
}
