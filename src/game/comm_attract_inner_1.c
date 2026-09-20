/* Comm-attract inner mode 1 @ 0xF970 (countdown + inner table advance). */
// @rom 0xf970 +0x54 comm_attract_inner_1

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_rom.h"

void comm_attract_inner_1(u32 arg0, u32 arg1, u32 arg2)
{
    u32 counter;
    u32 table_idx;
    u32 record;
    u32 next_inner;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /*
     * @0xF970–0xF97C: lda 0x5ae9c4 → g0 trampoline; mov 0,g14.
     * Host: C return resumes board_dispatch (same as bx (g0) to a ret stub).
     */
    counter = i960_ld_u32(I960_WORKRAM, 0x20a7cc, 0);
    counter--;
    i960_st_u32(I960_WORKRAM, 0x20a7cc, 0, counter);

    /* @0xF994: cmpibl 0,g4 → skip advance while counter > 0. */
    if ((signed int)counter > 0)
        return;

    table_idx = i960_ld_u32(I960_WORKRAM, 0x20a7dc, 0);
    i960_st_u32(I960_WORKRAM, 0x20a784, 0, (u32)g14);

    /* @0xF9A8: advance inner via 5AE430[20a7dc] record dword @ record+0xC. */
    record = model2_workram_mirror_u32(0x005ae430u + (table_idx << 2));
    next_inner = model2_workram_mirror_u32(record + 0xc);
    i960_st_u32(I960_WORKRAM, 0x20209c, 0, next_inner);
    i960_st_u32(I960_WORKRAM, 0x214354, 0, table_idx);
}
