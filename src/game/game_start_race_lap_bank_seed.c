/* Lap bank seed @ 0x18248 — stores 16 → 0x20a9fc.
 *
 * bal from game_start_race_lap_bank_clear when 0x202230==1 or course<2
 * (practice / desert). 0x20a9fc indexes the 0x1d002f0 lap-row table
 * (see @ 0x1A018: lda (g5)[g5*2] then 0x1d002f0(g4)[g5*4]).
 *
 * source: disasm/maincpu/maincpu_018248_18.asm */
// @rom 0x18248 +0x18 game_start_race_lap_bank_seed

#include "i960_lift.h"
#include "i960_mem.h"

void game_start_race_lap_bank_seed(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x18250–0x18254: mov 16,g4; st g4,0x20a9fc. */
    i960_st_u32(I960_WORKRAM, 0x20a9fc, 0, 16u);
}
