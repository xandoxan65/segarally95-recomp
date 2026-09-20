/* Car peer slot refresh @ 0x2CC78 — bal from car frame @ 0x2BCF0.
 *
 * Reads pose+0x51 nibble → 0x214180[n], clamps against 0x213978 count, then
 * stores 0x213b00[i] peers into 0x20cac0/0x20cac4.
 *
 * source: disasm/maincpu/maincpu_02cc78_68.asm */
// @rom 0x2cc78 +0x68 game_start_race_obj_car_peer

#include "i960_lift.h"
#include "i960_mem.h"

void game_start_race_obj_car_peer(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0;
    u32 pose;
    u32 nibble;
    u32 slot;
    u32 next;
    u32 count;
    u32 a;
    u32 b;

    (void)arg1;
    (void)arg2;

    if (node == 0u)
        return;

    /* @0x2CC80–0x2CC8C: pose+0x51 & 0x3c (addo 31,29 → 60). */
    pose = i960_ld_u32(I960_ABS, node, 0x8c);
    nibble = i960_ld_u8(I960_ABS, pose + 0x51u, 0);
    nibble &= 0x3cu;

    slot = i960_ld_u32(I960_WORKRAM, 0x214180u + nibble, 0);
    next = slot + 1u;
    count = i960_ld_u32(I960_WORKRAM, 0x213978, 0);

    /* @0x2CC98–0x2CCAC: if slot-1 < 0 use count-1 as index. */
    a = slot - 1u;
    if ((i32)a < 0)
        a = count - 1u;

    /* @0x2CCB0–0x2CCB4: if next >= count → next = 0. */
    if (next >= count)
        next = 0u;

    b = next;
    a = i960_ld_u32(I960_WORKRAM, 0x213b00u + (a << 2), 0);
    b = i960_ld_u32(I960_WORKRAM, 0x213b00u + (b << 2), 0);
    i960_st_u32(I960_WORKRAM, 0x20cac0, 0, a);
    i960_st_u32(I960_WORKRAM, 0x20cac4, 0, b);
}
