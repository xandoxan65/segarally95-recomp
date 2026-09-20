/* Race CGM batch seed @ 0x1DAD0 — cam_boot after tile clear / scene_list_seed.
 *
 * Eight catalog_draw_setup calls on fixed main_data CGM headers; stores the
 * returned batch indices into 0x20ac78…0x20ac94. Without this, 0x20c954 stays
 * 0 after scene_list_seed and every draw_scene_dispatch takes the OOB
 * printf/glyph path (screen full of random tiles).
 *
 * source: disasm/maincpu/maincpu_01dad0_200.asm */
// @rom 0x1dad0 +0x120 game_start_race_cgm_batch_seed

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>

static u32 seed_one(u32 cgm_va, u32 slot_va)
{
    u32 batch;

    g0 = 0;
    g1 = 0;
    g2 = cgm_va;
    g3 = 0;
    g4 = 4;
    batch = catalog_draw_setup(0, 0, cgm_va);
    i960_st_u32(I960_WORKRAM, slot_va, 0, batch);
    return batch;
}

void game_start_race_cgm_batch_seed(u32 arg0, u32 arg1, u32 arg2)
{
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    if (!logged) {
        lift_log( "lift: race_cgm_batch_seed\n");
        fflush(stderr);
        logged = 1;
    }

    /* @0x1DAD0–0x1DBF0: eight CGM headers → 0x20ac78…0x20ac94. */
    seed_one(0x021e0a28u, 0x20ac78u);
    seed_one(0x028c3818u, 0x20ac7cu);
    seed_one(0x028a920cu, 0x20ac80u);
    seed_one(0x0288b8c0u, 0x20ac84u);
    seed_one(0x0288ba60u, 0x20ac88u); /* HUD overlays used by hud_layers/slot0 */
    seed_one(0x021f8408u, 0x20ac8cu);
    seed_one(0x02087808u, 0x20ac90u);
    seed_one(0x0288a508u, 0x20ac94u);

    {
        static int once;

        if (!once) {
            lift_log(
                    "lift: race_cgm_batch 20ac88=%u limit=%u\n",
                    (unsigned)i960_ld_u32(I960_WORKRAM, 0x20ac88, 0),
                    (unsigned)i960_ld_u32(I960_WORKRAM, 0x20c954, 0));
            fflush(stderr);
            once = 1;
        }
    }
}
