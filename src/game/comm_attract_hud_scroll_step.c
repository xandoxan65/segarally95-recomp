/* Attract HUD scroll step / batch release @ 0x13340 (hook 0x5B2340).
 * Advances 0x20A790 by 16 each tick; on later thresholds flushes the two
 * CGM batches seeded by scene_hud_alt and swaps the scene hook forward.
 * source: disasm/maincpu/maincpu_013340_160.asm */
// @rom 0x13340 +0x160 comm_attract_hud_scroll_step

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"

extern void comm_attract_hud_scroll_fill_dual(u32 arg0, u32 arg1, u32 arg2);

void comm_attract_hud_scroll_step(u32 arg0, u32 arg1, u32 arg2)
{
    u32 scroll;
    u32 bit9 = 1u << 9;   /* 0x200 */
    u32 bit10 = 1u << 10; /* 0x400 */
    u32 bit11 = 1u << 11; /* 0x800 */
    u32 tripple9 = 3u << 9; /* 0x600 */
    u32 counter;
    u32 dst;
    u32 idx;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x13340: bal 0x132a8 — dual-row hscr fill for ranking strips. */
    comm_attract_hud_scroll_fill_dual(0, 0, 0);

    scroll = i960_ld_u32(I960_WORKRAM, 0x20a790, 0) + 16u;
    i960_st_u32(I960_WORKRAM, 0x20a790, 0, scroll);

    /* @0x1335C: cmpibg scroll, 0x200 → else scroll_fill + copyright. */
    if ((i32)scroll <= (i32)bit9) {
        comm_attract_hud_scroll_fill(0, 0, 0);
        goto copyright;
    }

    /* @0x13370: cmpible scroll, 0x200 → skip clamp; else counter hold. */
    scroll = i960_ld_u32(I960_WORKRAM, 0x20a790, 0);
    if ((i32)scroll > (i32)bit9) {
        /* @0x13374–0x13390: counter ≤ 0x577 → clamp to 0x200. */
        counter = i960_ld_u32(I960_WORKRAM, 0x20a794, 0);
        if ((i32)counter <= 0x577) {
            i960_st_u32(I960_WORKRAM, 0x20a790, 0, bit9);
            goto copyright;
        }
    }

    /* @0x13394: exactly 0x400 → clear rect + hook scene_hud_alt (ranks 9–16). */
    scroll = i960_ld_u32(I960_WORKRAM, 0x20a790, 0);
    if (scroll == bit10) {
        g0 = 6;
        g1 = 3;
        g2 = 51;
        g3 = 37;
        i960_call_rom(0x27260);
        i960_st_u32(I960_WORKRAM, 0x20a78c, 0, 0x005b1d90u);
        goto copyright;
    }

    /* @0x133D0: cmpible scroll, 0x600 → skip; else counter hold at 0x600.
     * (ROM clamps only when scroll > 0x600 — inverted lift skipped 0x400.) */
    scroll = i960_ld_u32(I960_WORKRAM, 0x20a790, 0);
    if ((i32)scroll > (i32)tripple9) {
        counter = i960_ld_u32(I960_WORKRAM, 0x20a794, 0);
        if ((i32)counter <= 0x6c1) {
            i960_st_u32(I960_WORKRAM, 0x20a790, 0, tripple9);
            goto copyright;
        }
    }

    /* @0x13400/0x13410: scroll_fill when scroll ≤ 0x7ff && counter > 0x6c2. */
    scroll = i960_ld_u32(I960_WORKRAM, 0x20a790, 0);
    if ((i32)scroll <= 0x7ff) {
        counter = i960_ld_u32(I960_WORKRAM, 0x20a794, 0);
        if ((i32)counter > 0x6c2) {
            comm_attract_hud_scroll_fill(0, 0, 0);
            goto copyright;
        }
    }

    /* @0x1341C: only 0x800 triggers flush. */
    scroll = i960_ld_u32(I960_WORKRAM, 0x20a790, 0);
    if (scroll != bit11)
        goto copyright;

    /* @0x1342C–0x13490 */
    g0 = 6;
    g1 = 3;
    g2 = 51;
    g3 = 37;
    i960_call_rom(0x27260);

    /* @0x13440–0x13460: clear hscr strip g5=24..0x147. */
    dst = 0x01008030u;
    idx = 24u;
    for (;;) {
        idx++;
        i960_st_u16(I960_ABS, dst, 0, 0);
        dst += 2u;
        if ((i32)idx > 0x147)
            break;
    }

    i960_st_u32(I960_WORKRAM, 0x2021f4, 0, 0x12cu);
    /* @0x13478 / @0x13484: release both HUD catalogs. */
    cgm_1111_flush(i960_ld_u32(I960_WORKRAM, 0x20a7b0, 0), 0, 0);
    cgm_1111_flush(i960_ld_u32(I960_WORKRAM, 0x20a79c, 0), 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a78c, 0, 0x005b24a0u);

copyright:
    comm_attract_hud_copyright_stamps(0, 0, 0);
}
