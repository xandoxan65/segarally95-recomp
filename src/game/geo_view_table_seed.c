/* Cam view table seed @ 0x377E0 — copy ROM row @ *0x5d67a0 into
 * 0x2141c0/d0/e0 (+0x30 → 0x2141f0), then latch/input/control/params.
 *
 * Called from game_start_race_cam_object_init; seeds the floats that
 * geo_view_copro_vec_push reads at 0x2141d0 on the main path.
 *
 * source: disasm/maincpu/maincpu_0377e0_90.asm */
// @rom 0x377e0 +0x90 geo_view_table_seed

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

#include "lift_syms.h"

static void copy_q(u32 src, u32 dst)
{
    u32 i;

    for (i = 0; i < 4u; i++)
        i960_st_u32(I960_WORKRAM, dst, i * 4u,
                    i960_ld_u32(I960_WORKRAM, src, i * 4u));
}

void geo_view_table_seed(u32 arg0, u32 arg1, u32 arg2)
{
    u32 cam = arg0 != 0u ? arg0 : (u32)g0;
    u32 row;
    u32 w30;
    u32 latch_g1;
    u32 saved;

    (void)arg1;
    (void)arg2;

    if (cam == 0u)
        return;

    /* @0x377E0: ld 0x5d67a0 → row; ldq/stq three quads; ld +0x30. */
    row = i960_ld_u32(I960_WORKRAM, 0x5d67a0, 0);
    copy_q(row, 0x2141c0u);
    copy_q(row + 0x10u, 0x2141d0u);
    copy_q(row + 0x20u, 0x2141e0u);
    w30 = i960_ld_u32(I960_WORKRAM, row, 0x30u);

    /* ROM `st g14` — program keeps g14=0 (`call` does not overwrite it). */
    g14 = 0;
    i960_st_u32(I960_WORKRAM, 0x21425c, 0, (u32)g14);
    /* ldos 0x12(g0) — cam halfword into latch g1. */
    latch_g1 = (u32)i960_ld_u16(I960_ABS, cam, 0x12u);
    saved = cam;
    i960_st_u32(I960_WORKRAM, 0x214258, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x2141f0, 0, w30);

    /* call 0x38680 with g0 = −1.0f. */
    geo_view_latch_apply(0xbf800000u, latch_g1, 0);
    /* subo 1,0,g0 → −1; call 0x388d0. */
    (void)geo_view_input_gate(0xffffffffu, 0, 0);

    i960_st_u32(I960_WORKRAM, 0x214264, 0, 15u);
    i960_st_u32(I960_WORKRAM, 0x214254, 0, (u32)g14);

    g0 = saved;
    geo_view_control_seed(saved, 0, 0);
    geo_view_params(0, 0, 0);
}
