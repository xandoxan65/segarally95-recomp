/* Mode-3/5 table slot continuation @ 0x162E0 — course-select icon dispatch.
 * Was staged as null; unlifted callx (and the callee @ 0x15380) silently
 * no-op'd, so after game_start_course_display advanced 0x2020ac to 1 the
 * championship/practice course icons never redrew each frame.
 * source: disasm/maincpu/maincpu_0162e0_e8.asm */
// @rom 0x162e0 +0xe8 game_start_course_select

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>

extern u32 game_start_select_timer_gate(void);
extern void game_start_course_select_frame(u32 arg0, u32 arg1, u32 arg2);

void game_start_course_select(u32 arg0, u32 arg1, u32 arg2)
{
    u32 flags;
    u32 bound;
    u32 gate;
    u32 choice;
    u32 sel_bit;
    u32 misc;
    u32 sub;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    geo_fifo_bootstrap(0, 0, 0);
    game_start_course_select_frame(0, 0, 0);

    /* @0x162E8-0x16318: one-shot texture descriptor bind, gated by timer!=2. */
    flags = i960_ld_u32(I960_WORKRAM, 0x202230, 0);
    bound = i960_ld_u32(I960_WORKRAM, 0x20a940, 0);
    gate = game_start_select_timer_gate();
    if (flags == 0u && bound == 0u && gate != 2u) {
        tile_texture_descriptor_apply(0xacu, 0x50u, 0);
        i960_st_u32(I960_WORKRAM, 0x20a940, 0, 1u);
    }

    /* @0x16320-0x1632C: cheat-check hook (0x13bf0) — not lifted, host no-op. */
    flags = i960_ld_u32(I960_WORKRAM, 0x202230, 0);
    if (flags == 0u)
        i960_call_rom(0x13bf0);

    /* @0x16330-0x1633C: apply the current view scene unless already latched. */
    if (i960_ld_u8(I960_WORKRAM, 0x20201a, 0) == 0u)
        geo_view_scene_apply(0, 0, 0);

    gate = game_start_select_timer_gate();
    /* @0x16344: non-zero gate — stay on the course-select submode. */
    if (gate != 0u)
        return;

    i960_call_rom(0x13bf0);

    choice = i960_ld_u32(I960_WORKRAM, 0x20a8c4, 0);
    sel_bit = (choice == 0u || choice == 3u) ? 1u : 0u;

    i960_st_u32(I960_WORKRAM, 0x2020b4, 0, gate);
    i960_st_u32(I960_WORKRAM, 0x2139cc, 0, (choice >> 1) & 1u);

    misc = i960_ld_u8(I960_WORKRAM, 0x20205c, 0);
    i960_st_u32(I960_WORKRAM, 0x2139d0, 0, sel_bit);
    i960_st_u16(I960_WORKRAM, 0x20b914, 0, 0);
    i960_st_u16(I960_WORKRAM, 0x20b918, 0, 0);

    /* @0x1639C-0x163AC: bit7 set && bit4 clear -> clear scroll-hold flag. */
    if ((misc & 0x80u) != 0u) {
        misc = i960_ld_u8(I960_WORKRAM, 0x20205c, 0);
        if ((misc & 0x10u) == 0u)
            i960_st_u32(I960_WORKRAM, 0x2139d4, 0, 0);
    }

    sub = i960_ld_u32(I960_WORKRAM, 0x2020ac, 0) + 1u;
    i960_st_u32(I960_WORKRAM, 0x2020ac, 0, sub);
    lift_log( "lift: course_select done choice=%u submode->%u\n",
            (unsigned)choice, (unsigned)sub);
}
