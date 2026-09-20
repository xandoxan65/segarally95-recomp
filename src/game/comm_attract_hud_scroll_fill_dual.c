/* Dual-row hscr fill @ 0x132a8 (bal from scene_hud_alt digit loop + scroll_step).
 * Paints scroll / −scroll into 0x010080F0 strips for ranking rows.
 * source: disasm/maincpu/maincpu_0132a8_90.asm */
// @rom 0x132a8 +0x90 comm_attract_hud_scroll_fill_dual

#include "i960_lift.h"
#include "i960_mem.h"

void comm_attract_hud_scroll_fill_dual(u32 arg0, u32 arg1, u32 arg2)
{
    u16 scroll;
    u16 neg;
    u32 dst;
    u32 row_off;
    i32 col;
    i32 pass;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x132B0–0x132EC: scroll at (g6)[0x010080F0], g0=1,3,5,7; g6+=0x60. */
    scroll = i960_ld_u16(I960_WORKRAM, 0x20a790, 0);
    row_off = 0;
    for (pass = 1; pass <= 8; pass += 2) {
        dst = 0x010080f0u;
        for (col = 0; col <= 23; col++) {
            i960_st_u16(I960_ABS, dst + row_off, 0, scroll);
            dst += 2u;
        }
        row_off += 0x60u;
    }

    /* @0x132F0–0x13330: −scroll at (g7)[0x010080F0], g0=2,4,6,8; g7=48+n*0x60. */
    neg = (u16)(0u - (u32)scroll);
    row_off = 48u; /* addo 31,17 */
    for (pass = 2; pass <= 8; pass += 2) {
        dst = 0x010080f0u;
        for (col = 0; col <= 23; col++) {
            i960_st_u16(I960_ABS, dst + row_off, 0, neg);
            dst += 2u;
        }
        row_off += 0x60u;
    }
}
