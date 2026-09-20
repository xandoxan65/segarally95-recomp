/* Race colorbase CGM seed @ 0x40790 — sub_reset after START.
 *
 * Three catalog_draw_setup calls (g4=4 setup-only) upload race palette
 * colorbases via cgm_leading_colorbase, then stash the course table pointer
 * from 0x5df770[course] into 0x215c30 for the companion tick @ 0x40820.
 *
 * HUD/scripted CGM colorbases + course table for tick @ 0x40820. GEO 3D
 * colorbases @ 0x01802000 are boot/refresh (0x5FB89E), not this path.
 *
 * source: disasm/maincpu/maincpu_040790_200.asm */
// @rom 0x40790 +0x88 game_start_race_cgm_colorbase_seed

#include "i960_lift.h"
#include "i960_mem.h"
#include "i960_host.h"
#include "model2_rom.h"

#include "lift_syms.h"

#include <stdio.h>

static u32 seed_one(u32 cgm_va)
{
    g0 = 0;
    g1 = 0;
    g2 = cgm_va;
    g3 = 0;
    g4 = 4;
    return catalog_draw_setup(0, 0, cgm_va);
}

void game_start_race_cgm_colorbase_seed(u32 arg0, u32 arg1, u32 arg2)
{
    static int logged;
    u32 course;
    u32 table;
    u32 batch0;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x407A4: st g14 → 0x215c20 (tick cursor). */
    i960_st_u32(I960_WORKRAM, 0x215c20, 0, (u32)g14);

    /* @0x40790–0x407B4: CGM @ 0x20fe200 → batch index @ 0x215c24. */
    batch0 = seed_one(0x020fe200u);
    i960_st_u32(I960_WORKRAM, 0x215c24, 0, batch0);

    /* @0x407BC–0x407D4 / @0x407D8–0x407F0: two more race CGMs (setup only). */
    seed_one(0x0289bc78u);
    seed_one(0x0289c0e8u);

    /* @0x407F4–0x40810: course table @ 0x5df770[course]; arm tick gate. */
    course = i960_host_race_course_index();
    table = model2_workram_mirror_u32(0x5df770u + (course << 2));
    i960_st_u32(I960_WORKRAM, 0x215c28, 0, (u32)(0u - 1u)); /* subo 1,0 */
    i960_st_u32(I960_WORKRAM, 0x215c30, 0, table);

    if (!logged) {
        fprintf(stderr,
                "lift: race_cgm_colorbase_seed batch=%u course=%u table=%#x\n",
                (unsigned)batch0, (unsigned)course, (unsigned)table);
        fflush(stderr);
        logged = 1;
    }
}
