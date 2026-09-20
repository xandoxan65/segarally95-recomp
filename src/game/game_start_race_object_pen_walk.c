/* Race object-pen list walk @ 0x24C50 — g0 selects flag bit (1→bit0, 0→bit1).
 *
 * Walks 0x10-byte descriptors from 0x2139f0 (optionally +0x10 when the
 * secondary object block is armed). Stops when descriptor+8 == 0. Each live
 * node (+4 != 0 and flag bit) calls object_pen_desc @ 0x248b0.
 *
 * source: disasm/maincpu/maincpu_024c50_c0.asm */
// @rom 0x24c50 +0x84 game_start_race_object_pen_walk

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"

void game_start_race_object_pen_walk(u32 arg0, u32 arg1, u32 arg2)
{
    u32 gate;
    u32 node;
    u32 link;
    u32 flag_bit;
    u8 flags;

    (void)arg1;
    (void)arg2;

    /* @0x24C58: mov g0,r7 — which flag bit to test. */
    flag_bit = arg0;

    node = 0x2139f0u;
    gate = i960_ld_u32(I960_WORKRAM, 0x2139d8, 0);
    if (gate != 0u) {
        if (i960_ld_u32(I960_WORKRAM, 0x2139f8, 0) != 0u) {
            flags = i960_ld_u8(I960_WORKRAM, 0x2139f0, 0);
            if ((flags & 2u) != 0u)
                node = node + 0x10u; /* @0x24C80 */
        }
    }

    /* @0x24C84–0x24C8C: enter while *(node+8) != 0. */
    link = i960_ld_u32(I960_WORKRAM, node, 8);
    while (link != 0u) {
        if (i960_ld_u32(I960_WORKRAM, node, 4) != 0u) {
            flags = i960_ld_u8(I960_WORKRAM, node, 0);
            if (flag_bit != 0u) {
                /* @0x24CA4: require bit0. */
                if ((flags & 1u) != 0u)
                    game_start_race_object_pen_desc(node, 0, 0);
            } else if ((flags & 2u) != 0u) {
                /* @0x24CB0: require bit1. */
                game_start_race_object_pen_desc(node, 0, 0);
            }
        }
        /* @0x24CC0–0x24CD0: next 0x10-byte record; stop on +8 == 0. */
        node += 0x10u;
        link = i960_ld_u32(I960_WORKRAM, node, 8);
    }
}
