/* Race HUD row param seed @ 0x1E498 — bal from cam_init after hud_layers.
 *
 * source: /tmp/dasm_1e498/maincpu_01e498_80.asm */
// @rom 0x1e498 +0x58 game_start_race_hud_row_seed

#include "i960_lift.h"
#include "i960_mem.h"

void game_start_race_hud_row_seed(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* st −1 → 0x20aeb8. */
    i960_st_u32(I960_WORKRAM, 0x20aeb8, 0, (u32)-1);

    if (i960_ld_u32(I960_WORKRAM, 0x2139d4, 0) != 0u) {
        /* addo 31,19 / addo 31,9 */
        i960_st_u32(I960_WORKRAM, 0x20aebc, 0, 50u);
        i960_st_u32(I960_WORKRAM, 0x20aec0, 0, 40u);
    } else {
        i960_st_u32(I960_WORKRAM, 0x20aebc, 0, 27u);
        i960_st_u32(I960_WORKRAM, 0x20aec0, 0, 9u);
    }
}
