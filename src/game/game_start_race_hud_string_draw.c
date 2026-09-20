/* HUD digit/string walk @ 0x1DC00 — cgm_seed passes g0=x, g1=y, g2=str EA.
 *
 * Walks a C string and draw_scene_dispatch's each glyph from the race digit
 * CGM batch @ 0x20aeb0. Digit/punct/letter windows match the cmp ranges in
 * disasm (addo-31 literals → ASCII bounds).
 *
 * source: disasm/maincpu/maincpu_01dc00_150.asm */
// @rom 0x1dc00 +0x124 game_start_race_hud_string_draw

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>

void game_start_race_hud_string_draw(u32 arg0, u32 arg1, u32 arg2)
{
    u32 x = arg0;
    u32 y = arg1;
    u32 str = arg2;
    u32 ch;
    u32 slot;
    u32 batch;
    int draw;
    static int logged;

    if (!logged) {
        fprintf(stderr, "lift: race_hud_string_draw\n");
        fflush(stderr);
        logged = 1;
    }

    if (str == 0u)
        return;
    /* Early-out on empty string (ldob (g2) before loop). */
    if (i960_ld_u8(I960_ABS, str, 0) == 0u)
        return;

    batch = i960_ld_u32(I960_WORKRAM, 0x20aeb0, 0);

    for (;;) {
        ch = i960_ld_u8(I960_ABS, str, 0);
        if (ch == 0u)
            break;
        str += 1u;
        draw = 0;
        slot = 0;

        /* '0'..'9': r10=0x2f, r11=0x39 via addo 31,{16,26}. */
        if (ch > 0x2fu && ch <= 0x39u) {
            slot = ch - 0x30u; /* lda 0xffffffd0(g4) */
            draw = 1;
        } else if (ch == 0x27u) {
            /* ''' → slot 13 */
            slot = 13u;
            draw = 1;
        } else if (ch == 0x22u) {
            /* '"' → slot 14 */
            slot = 14u;
            draw = 1;
        } else if (ch == 0x2du) {
            /* '-' → slot 15 */
            slot = 15u;
            draw = 1;
        } else if (ch == 0x2eu) {
            /* '.' → slot 16 */
            slot = 16u;
            draw = 1;
        } else if (ch > 0x60u && ch <= 0x7au) {
            /* 'a'..'z': lda 0xffffffb2(g4) = ch − 0x4e */
            slot = ch - 0x4eu;
            draw = 1;
        } else if (ch > 0x40u && ch <= 0x5au) {
            /* 'A'..'Z': lda 0xffffffd2(g3) = ch − 0x2e */
            slot = ch - 0x2eu;
            draw = 1;
        }

        if (draw) {
            g0 = x;
            g1 = y;
            g2 = batch;
            g3 = slot;
            g4 = 0;
            draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
        }

        /* Always advance x (addo r4,1), then continue while *r6 != 0. */
        x += 1u;
    }
}
