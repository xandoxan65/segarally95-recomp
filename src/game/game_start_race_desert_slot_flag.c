/* Desert race slot flag @ 0x46470 — packs near-slot state into 0x20a730.
 *
 * ABI: g0 = pos XYZ*, g1 = angles*, g2 = lane, g3 = rank (or −1 clear path).
 * Returns 0 on early clear, 1 after commit / already-set path.
 *
 * source: /tmp/dasm_46470d/maincpu_046470_200.asm */
// @rom 0x46470 +0xf8 game_start_race_desert_slot_flag

#include "i960_lift.h"
#include "i960_mem.h"

u32 game_start_race_desert_slot_flag(u32 pos, u32 angles, u32 lane)
{
    i32 rank = (i32)g3;
    i32 capped;
    u32 flags;
    u32 nibble;
    u32 hi;
    u32 lane_bits;
    u32 rank_bits;

    capped = rank;
    if (rank < 0) {
        flags = i960_ld_u8(I960_WORKRAM, 0x20a730, 0);
        nibble = (flags >> 1) & 15u;
        if (nibble == lane) {
            flags &= ~1u;
            i960_st_u8(I960_WORKRAM, 0x20a730, 0, (u8)flags);
            g0 = 0;
            return 0;
        }
    }

    if (rank >= 3)
        capped = 3;

    flags = i960_ld_u8(I960_WORKRAM, 0x20a730, 0);
    hi = (flags >> 5) & 3u;
    if (hi > (u32)capped) {
        flags = i960_ld_u8(I960_WORKRAM, 0x20a730, 0);
        if (flags & 1u) {
            g0 = 1;
            return 1;
        }
    }

    flags = i960_ld_u8(I960_WORKRAM, 0x20a730, 0) | 1u;
    i960_st_u8(I960_WORKRAM, 0x20a730, 0, (u8)flags);

    flags = i960_ld_u8(I960_WORKRAM, 0x20a730, 0);
    lane_bits = (lane & 15u) << 1;
    flags = (flags & (u32)(0u - 31u)) | lane_bits; /* mask 0xffffffe1 */
    i960_st_u8(I960_WORKRAM, 0x20a730, 0, (u8)flags);

    flags = i960_ld_u8(I960_WORKRAM, 0x20a730, 0);
    rank_bits = ((u32)capped & 3u) << 5;
    flags = (flags & 0xffffff9fu) | rank_bits;
    i960_st_u8(I960_WORKRAM, 0x20a730, 0, (u8)flags);

    i960_st_u32(I960_WORKRAM, 0x20a738, 0, i960_ld_u32(I960_ABS, pos, 0));
    i960_st_u32(I960_WORKRAM, 0x20a73c, 0, i960_ld_u32(I960_ABS, pos, 4));
    i960_st_u32(I960_WORKRAM, 0x20a740, 0, i960_ld_u32(I960_ABS, pos, 8));
    i960_st_u32(I960_WORKRAM, 0x20a744, 0, i960_ld_u32(I960_ABS, angles, 0));
    i960_st_u32(I960_WORKRAM, 0x20a748, 0, i960_ld_u32(I960_ABS, angles, 4));
    i960_st_u32(I960_WORKRAM, 0x20a74c, 0, i960_ld_u32(I960_ABS, angles, 8));

    g0 = 1;
    return 1;
}
