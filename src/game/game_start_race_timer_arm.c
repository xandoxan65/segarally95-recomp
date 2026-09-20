/* Practice/champ scene slot @ 0x1BF50 — arm race timer when 0x20aab0 == 0xf0.
 * Attract hud_setup seeds 0x20aab0 = 15<<4. Table entries (practice [12],
 * attract [9]/[14]) callx here. st g14 → 0x20a560: scene callx leaves
 * g14 cleared on this host path (same convention as practice empty-slot
 * stores). Then draw_setup / catalog seed, decrement 0x20aab0 through
 * 0xef → −1, and on −1 return to attract mode 2.
 * source: disasm/maincpu/maincpu_01bf50_200.asm */
// @rom 0x1bf50 +0x140 game_start_race_timer_arm

#include "i960_lift.h"
#include "i960_mem.h"
#include "i960_host.h"

#include "lift_syms.h"

#include <stdio.h>

void game_start_race_timer_arm(u32 arg0, u32 arg1, u32 arg2)
{
    u32 phase;
    u32 base;
    u32 lo;
    u32 hi;
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    phase = i960_ld_u32(I960_WORKRAM, 0x20aab0, 0);
    if (phase == (15u << 4)) {
        /* @0x1BF60: st g14 → timer (host g14==0 on scene callx path). */
        g14 = 0;
        i960_st_u32(I960_WORKRAM, 0x20a560, 0, (u32)g14);
        if (!logged) {
            fprintf(stderr, "lift: race_timer_arm → 0 (20aab0==0xf0)\n");
            fflush(stderr);
            logged = 1;
        }
        comm_draw_setup(0, 0, 0);
        i960_call_rom(0xa768);

        if (i960_ld_u32(I960_WORKRAM, 0x202230, 0) == 0u)
            base = 0x01d002f0u;
        else {
            u32 course = i960_host_race_course_index();

            base = 0x01d00484u + course * 0x194u;
        }
        lo = i960_ld_u32(I960_ABS, base, 0);
        hi = i960_ld_u32(I960_ABS, base + 4u, 0);
        lo = lo + i960_ld_u32(I960_WORKRAM, 0x2020b8, 0);
        hi = hi + 1u;
        i960_st_u32(I960_ABS, base, 0, lo);
        i960_st_u32(I960_ABS, base + 4u, 0, hi);
        i960_st_u32(I960_ABS, 0x01d002e8u, 0, 1u);

        g0 = 0xffffu;
        tile_attract_palram_gate(0xffffu);
        g0 = 0;
        g1 = 18;
        g2 = 0x028a670cu;
        g3 = 0;
        g4 = 0;
        catalog_draw_setup(0, 18, 0x028a670cu);

        phase = i960_ld_u32(I960_WORKRAM, 0x20aab0, 0);
        i960_st_u8(I960_WORKRAM, 0x202049, 0, 0); /* stob g14 */
        i960_st_u32(I960_WORKRAM, 0x20aab0, 0, phase - 1u);
    }

    phase = i960_ld_u32(I960_WORKRAM, 0x20aab0, 0);
    if (phase == 0xefu) {
        g0 = 0xa6u;
        comm_palette_index_call(0xa6u);
        phase = i960_ld_u32(I960_WORKRAM, 0x20aab0, 0);
        i960_st_u32(I960_WORKRAM, 0x20aab0, 0, phase - 1u);
        return;
    }

    phase = i960_ld_u32(I960_WORKRAM, 0x20aab0, 0);
    phase = phase - 1u;
    i960_st_u32(I960_WORKRAM, 0x20aab0, 0, phase);
    if (phase != (u32)-1)
        return;

    /* @0x1C060: back to attract mode 2. */
    g0 = 0x01000000u;
    i960_call_rom(0x26b60);
    g0 = 0x9au;
    g1 = 0;
    tile_texture_descriptor_apply(0x9au, 0, 0);
    g14 = 0;
    i960_st_u32(I960_WORKRAM, 0x20209c, 0, 0u);
    i960_st_u32(I960_WORKRAM, 0x202098, 0, 2u);
}
