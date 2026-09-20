/* Mode-3/5 table slot [3] @ 0x15200 — car-select per-frame loop.
 * Calls geo frame @ 0x14820, one-shot texture bind, confirm @ 0x15030.
 * source: disasm/maincpu/maincpu_015200_e0.asm */
// @rom 0x15200 +0xe0 game_start_car_select

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_memory.h"

#include "lift_syms.h"

#include <stdio.h>

extern u32 game_start_select_timer_gate(void);
extern u32 game_start_select_confirm(u32 arg0, u32 arg1, u32 arg2);
extern void game_start_car_select_frame(u32 arg0, u32 arg1, u32 arg2);

void game_start_car_select(u32 arg0, u32 arg1, u32 arg2)
{
    u32 gate;
    u32 pad;
    u32 course;
    u32 sub;
    u32 timer_live;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    geo_fifo_bootstrap(0, 0, 0);
    /* @0x15204–0x15214: GEO clear color / background. */
    i960_mmio_write_u32(0x800080u, 0u);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x3b800000u);

    game_start_car_select_frame(0, 0, 0);
    /* One-shot path marker — panels are practice table slot [3] only. */
    {
        static int logged;

        if (!logged) {
            fprintf(stderr,
                    "lift: car_select_frame emit (expect ~524 verts, "
                    "panel cx≈−10/0/+10)\n");
            logged = 1;
        }
    }

    /* @0x15220–0x15250: one-shot texture descriptor, gated by timer!=2. */
    timer_live = i960_ld_u32(I960_WORKRAM, 0x20a560, 0);
    if (i960_ld_u32(I960_WORKRAM, 0x20a8b0, 0) == 0u &&
        (i32)timer_live > 0) {
        gate = game_start_select_timer_gate();
        if (gate != 2u) {
            tile_texture_descriptor_apply(0xacu, 0x50u, 0);
            i960_st_u32(I960_WORKRAM, 0x20a8b0, 0, 1u);
        }
    }

    i960_call_rom(0x13bf0);

    if (i960_ld_u8(I960_WORKRAM, 0x20201a, 0) == 0u)
        geo_view_scene_apply(0, 0, 0);

    gate = game_start_select_timer_gate();
    /* @0x15270: non-zero gate — stay on car-select. */
    if (gate != 0u)
        return;

    pad = i960_ld_u8(I960_WORKRAM, 0x202052, 0) & 0xffu;
    if (pad > 0xb0u && i960_ld_u32(I960_WORKRAM, 0x214354, 0) == 2u) {
        i960_call_rom(0x13bf0);
        if ((u32)g0 != 0u)
            i960_st_u32(I960_WORKRAM, 0x214354, 0, 3u);
    }

    /* @0x152AC: confirm helper @ 0x15030. */
    if (game_start_select_confirm(0, 0, 0) == 0u)
        return;

    i960_st_u16(I960_WORKRAM, 0x20b914, 0, 0);
    i960_st_u16(I960_WORKRAM, 0x20b918, 0, 0);
    sub = i960_ld_u32(I960_WORKRAM, 0x2020ac, 0) + 1u;
    i960_st_u32(I960_WORKRAM, 0x2020ac, 0, sub);
    course = i960_ld_u32(I960_WORKRAM, 0x214354, 0);
    fprintf(stderr, "lift: car_select done car=%u lookup=%u submode->%u\n",
            (unsigned)course,
            (unsigned)i960_ld_u32(I960_WORKRAM, 0x20a8bc, 0),
            (unsigned)sub);
}
