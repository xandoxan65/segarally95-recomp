/* Attract HUD scroll/hscr paint @ 0x13238 (bal from scene_hud_alt @ 0x12DFC).
 * Writes 0x20a790 halfword (and its negate) across tile-map scroll slots. */
// @rom 0x13238 +0x60 comm_attract_hud_scroll_fill

#include "i960_lift.h"
#include "i960_mem.h"

void comm_attract_hud_scroll_fill(u32 arg0, u32 arg1, u32 arg2)
{
    u16 scroll;
    u16 neg;
    u32 dst;
    i32 count;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x13240–0x13264: 0x20a790 → 0x0100808E downward. */
    scroll = i960_ld_u16(I960_WORKRAM, 0x20a790, 0);
    dst = 0x0100808eu;
    count = 47; /* addo 31,16 */
    while (count >= 0) {
        count--;
        i960_st_u16(I960_ABS, dst, 0, scroll);
        dst -= 2u;
        if (count < 0)
            break;
    }

    /* @0x13268–0x13290: −scroll → 0x010080EE downward. */
    neg = (u16)(0u - (u32)scroll);
    dst = 0x010080eeu;
    count = 31;
    while (count >= 0) {
        count--;
        i960_st_u16(I960_ABS, dst, 0, neg);
        dst -= 2u;
        if (count < 0)
            break;
    }
}
