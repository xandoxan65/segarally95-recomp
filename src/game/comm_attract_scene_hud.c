/* Attract per-frame scene HUD @ 0x12CC0 (callx via 0x20A78C → staged 0x5B1CC0).
 * Draws CREDIT (C060 mode 0) then refreshes copyright / board glyphs.
 * source: disasm/maincpu/maincpu_012b80_220.asm */
// @rom 0x12cc0 +0xd0 comm_attract_scene_hud

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"

static void hud_draw(u32 x, u32 y, u32 slot, u32 glyph, u32 flags)
{
    g0 = x;
    g1 = y;
    g2 = slot;
    g3 = glyph;
    g4 = flags;
    draw_scene_dispatch(x, y, slot);
}

void comm_attract_scene_hud(u32 arg0, u32 arg1, u32 arg2)
{
    u32 inner;
    u32 board_type;
    u32 flag;
    u32 r4;
    u32 slot;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x12CC0–0x12CC4: CREDIT / free-play tile draw. */
    comm_scene_catalog_init(0, 0, 0);

    /* @0x12CC8–0x12CE0: board-type-1/2 extras skipped when inner==9. */
    inner = i960_ld_u32(I960_WORKRAM, 0x20209c, 0);
    if (inner != 9u) {
        board_type = i960_ld_u32(I960_WORKRAM, 0x20a530, 0);
        if (board_type == 1u || board_type == 2u) {
            /* @0x12CE4–0x12D50 */
            flag = i960_ld_u8(I960_WORKRAM, 0x202019, 0);
            slot = i960_ld_u32(I960_WORKRAM, 0x20a7b8, 0);
            hud_draw(9, 5, slot, i960_ld_u32(I960_WORKRAM, 0x20a7bc, 0), 0);

            /*
             * @0x12D08–0x12D30: cmpi g5,0; testne r4; subo r4,0,r4; and 5,r4,r4;
             * lda 1(r4),g3 — glyph index 1 when flag==0, else 6.
             */
            r4 = (flag != 0u) ? 1u : 0u;
            r4 = (u32)(0 - (int)r4);
            r4 &= 5u;
            hud_draw(16, 6, slot, r4 + 1u, 0);

            slot = i960_ld_u32(I960_WORKRAM, 0x20a7b8, 0);
            hud_draw(21, 12, slot, r4, 0);
        }
    }

    /* @0x12D54–0x12D88: SEGA / copyright slots from scene_alloc. */
    slot = i960_ld_u32(I960_WORKRAM, 0x20a7a8, 0);
    hud_draw(48, 43, slot, 0, 0);
    slot = i960_ld_u32(I960_WORKRAM, 0x20a7ac, 0);
    hud_draw(29, 45, slot, 0, 0);
}
