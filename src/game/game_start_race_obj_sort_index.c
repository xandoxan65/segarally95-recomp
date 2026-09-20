/* Object sort-index seed @ 0x231F0 — called from 0x22C70 / 0x22E20.
 *
 * Clears 0x20b0c0[0..count), calls 0x23130 (rank helper), then writes
 * 0x213980[rank] into 0x213b00[] and stores sequential ranks at 0x214180.
 *
 * source: disasm/maincpu/maincpu_0231f0_90.asm */
// @rom 0x231f0 +0x80 game_start_race_obj_sort_index

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>

void game_start_race_obj_rank_fill(u32 arg0, u32 arg1, u32 arg2);

void game_start_race_obj_sort_index(u32 arg0, u32 arg1, u32 arg2)
{
    u32 count;
    u32 i;
    u32 slot;
    u32 rank;
    u32 obj;
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    count = i960_ld_u32(I960_WORKRAM, 0x213978, 0);
    /* @0x231FC: cmpibge 0,count → skip clear when count <= 0. */
    if ((i32)count > 0) {
        slot = 0x0020b0c0u;
        for (i = 0; i < count; i++) {
            i960_st_u32(I960_WORKRAM, slot, 0, i);
            slot += 4u;
        }
    }

    game_start_race_obj_rank_fill(0, 0, 0);

    count = i960_ld_u32(I960_WORKRAM, 0x213978, 0);
    if ((i32)count <= 0)
        return;

    slot = 0x00213b00u;
    for (i = 0; i < count; i++) {
        rank = i960_ld_u32(I960_WORKRAM, 0x20b0c0u + i * 4u, 0);
        obj = i960_ld_u32(I960_WORKRAM, 0x213980u + (rank << 2), 0);
        i960_st_u32(I960_WORKRAM, 0x214180u + (rank << 2), 0, i);
        i960_st_u32(I960_WORKRAM, slot, 0, obj);
        slot += 4u;
    }

    if (!logged && (i32)count >= 2) {
        u32 r0 = i960_ld_u32(I960_WORKRAM, 0x20b0c0, 0);
        u32 r1 = i960_ld_u32(I960_WORKRAM, 0x20b0c4, 0);
        u32 p0 = i960_ld_u32(I960_WORKRAM, 0x213b00, 0);
        u32 p1 = i960_ld_u32(I960_WORKRAM, 0x213b04, 0);
        i32 s0 = (i32)(signed short)i960_ld_u16(I960_ABS, p0, 0x56);
        i32 s1 = (i32)(signed short)i960_ld_u16(I960_ABS, p1, 0x56);

        fprintf(stderr,
                "lift: race_obj_sort ranks=%u/%u poses=%#x/%#x span56=%d/%d "
                "view0=%#x\n",
                (unsigned)r0, (unsigned)r1, (unsigned)p0, (unsigned)p1, s0, s1,
                (unsigned)i960_ld_u32(I960_WORKRAM, 0x213b00, 0));
        fflush(stderr);
        logged = 1;
    }
}
