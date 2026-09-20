/* Race object near-scan @ 0x22940 — count peers near object index g0.
 *
 * Walks 0x213980[0..0x213978), calls span test @ 0x22850 per peer.
 * Returns 0x2021f0 + match_count in g0 (slot0/slot2 → 0x2020c8).
 *
 * source: disasm/maincpu/maincpu_022940_120.asm */
// @rom 0x22940 +0xc4 game_start_race_obj_scan

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <string.h>

void game_start_race_obj_scan(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    u8 frame[0x80];
    u32 self_ix = arg0;
    u32 self_obj;
    u32 count;
    u32 matches;
    u32 i;
    u32 self_z;
    u32 cursor;
    u32 mask;

    (void)arg1;
    (void)arg2;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;

    self_obj = i960_ld_u32(I960_WORKRAM, 0x213980, self_ix << 2);
    *(u32 *)(fp + 0x40) = i960_ld_u32(I960_ABS, self_obj, 0);
    *(u32 *)(fp + 0x44) = i960_ld_u32(I960_ABS, self_obj, 4);
    *(u32 *)(fp + 0x48) = i960_ld_u32(I960_ABS, self_obj, 8);
    self_z = i960_ld_u32(I960_ABS, self_obj, 0x54u);

    count = i960_ld_u32(I960_WORKRAM, 0x213978, 0);
    matches = 0u;
    i960_st_u32(I960_WORKRAM, 0x20ac70, 0, 0u);
    /* @0x22990: addo 31,29 → 60. */
    mask = 60u;

    if ((i32)count > 0) {
        cursor = 0x00213980u;
        for (i = 0u; (i32)i < (i32)count; i++) {
            u32 other;

            if (i != self_ix) {
                other = i960_ld_u32(I960_WORKRAM, cursor, 0);
                *(u32 *)(fp + 0x50) = i960_ld_u32(I960_ABS, other, 0);
                *(u32 *)(fp + 0x54) = i960_ld_u32(I960_ABS, other, 4);
                g2 = i960_ld_u32(I960_ABS, other, 0x54u);
                *(u32 *)(fp + 0x58) = i960_ld_u32(I960_ABS, other, 8);

                g0 = self_z;
                g1 = fp + 0x40u;
                g3 = fp + 0x50u;
                game_start_race_obj_span_test((u32)g0, (void *)g1, (u32)g2);
                matches += (u32)g0;
                if (g0 != 0u) {
                    u32 flags = i960_ld_u8(I960_ABS, other, 0x51u);

                    if ((flags & mask) == 0u)
                        i960_st_u32(I960_WORKRAM, 0x20ac70, 0, 1u);
                }
            }
            cursor += 4u;
        }
    }

    g0 = i960_ld_u32(I960_WORKRAM, 0x2021f0, 0) + matches;
    fp = fp_save;
}
