/* Race object/time seed @ 0x219C0 — logo_path after cam object_init.
 *
 * Practice (0x202230==1) @ 0x219D4:
 *   cmpibge 0,timer → 0x217e0 when timer <= 0 (Intel: src1 >= src2)
 *   else bal 0x21918 (timer > 0 — linked session already seeded)
 * Solo phase_0 leaves timer = −1, so 0x217e0 must run.
 *
 * source: disasm/maincpu/maincpu_0219c0_100.asm,
 *         maincpu_021918_c0.asm */
// @rom 0x219c0 +0x30 game_start_race_obj_seed
// @rom 0x21918 +0x38 game_start_race_obj_seed_practice

#include "i960_lift.h"
#include "i960_mem.h"
#include "i960_host.h"

#include "lift_syms.h"

#include <stdio.h>

void game_start_race_obj_seed_cars(u32 arg0, u32 arg1, u32 arg2);
void game_start_race_obj_seed_practice(u32 arg0, u32 arg1, u32 arg2)
{
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    if (!logged) {
        fprintf(stderr, "lift: race_obj_seed_practice\n");
        fflush(stderr);
        logged = 1;
    }

    /* @0x21918: mov g14,g0; mov 0,g14 — restore return; g14 stays 0. */
    g0 = g14;
    g14 = 0;

    /* st 1 → 0x2139c8; clear 0x2021f0 / 0x2140d4; st −1 → 0x20caf0. */
    i960_st_u32(I960_WORKRAM, 0x2139c8, 0, 1);
    i960_st_u32(I960_WORKRAM, 0x2021f0, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x2140d4, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20caf0, 0, 0xffffffffu);

    /* bx (g0) — host returns; g0 was pre-call return (unused). */
    g0 = 0;
}

void game_start_race_obj_seed(u32 arg0, u32 arg1, u32 arg2)
{
    u32 mode;
    u32 timer;
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    if (!logged) {
        fprintf(stderr, "lift: race_obj_seed\n");
        fflush(stderr);
        logged = 1;
    }

    /* @0x219C0: addo 16,sp — private frame unused on host. */
    if (i960_host_skip_practice())
        i960_st_u32(I960_WORKRAM, 0x202230, 0, 1u);
    mode = i960_ld_u32(I960_WORKRAM, 0x202230, 0);
    if (mode == 1u) {
        timer = i960_ld_u32(I960_WORKRAM, 0x20a560, 0);
        /* cmpibge 0,timer,0x219e8 → 0x217e0 when timer <= 0. */
        if ((i32)timer <= 0)
            game_start_race_obj_seed_cars(0, 0, 0);
        else
            game_start_race_obj_seed_practice(0, 0, 0);
        return;
    }

    /* Non-practice long path @ 0x219F0 — still unlifted. */
    i960_call_rom(0x219f0);
}
