/* Post-ranking HUD stamps @ 0x134A0 (hook 0x5B24A0 after scroll_step flush).
 * Draws chrome from catalog 0x20a7a0 while 0x2021f4 == 0x12c, then counts
 * down and restores scene_hud (0x5B1CC0) when the counter hits −1.
 * Without this lift, callx 0x5B24A0 was a silent no-op so attract overlays
 * died after the ranking strip flushed.
 * source: disasm/maincpu/maincpu_0134a0_150.asm */
// @rom 0x134a0 +0x130 comm_attract_hud_post_rank

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

static void stamp(u32 x, u32 y, u32 glyph)
{
    u32 slot = i960_ld_u32(I960_WORKRAM, 0x20a7a0, 0);

    g0 = x;
    g1 = y;
    g2 = slot;
    g3 = glyph;
    g4 = 0;
    draw_scene_dispatch(x, y, slot);
}

void comm_attract_hud_post_rank(u32 arg0, u32 arg1, u32 arg2)
{
    u32 counter;
    u32 board;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    counter = i960_ld_u32(I960_WORKRAM, 0x2021f4, 0);
    if (counter == 0x12cu) {
        stamp(0, 1, 4);
        stamp(9, 23, 2);
        stamp(8, 35, 3); /* addo 31,4 */
        stamp(2, 39, 1); /* addo 31,8 */
        stamp(37, 25, 5); /* addo 31,6 */
        stamp(35, 35, 6); /* addo 31,4 into g0 then mov g0,g1 — y=35 */
        stamp(34, 39, 0); /* addo 31,3 / addo 31,8 */
    }

    counter = i960_ld_u32(I960_WORKRAM, 0x2021f4, 0);
    counter -= 1u;
    i960_st_u32(I960_WORKRAM, 0x2021f4, 0, counter);

    /* @0x13590: counter == −1 → clear rect, restore HUD hook. */
    if (counter == (u32)(0u - 1u)) {
        g0 = 0;
        g1 = 0;
        g2 = 1u << 6;
        g3 = 43; /* addo 31,12 */
        boot_tile_map_fill((u32)g0, (u32)g1, (u32)g2, (u32)g3);

        board = i960_ld_u32(I960_WORKRAM, 0x20a530, 0);
        if (board != 3u)
            i960_st_u32(I960_WORKRAM, 0x20a78c, 0, 0x005b1cc0u);
    }

    comm_attract_hud_copyright_stamps(0, 0, 0);
}
