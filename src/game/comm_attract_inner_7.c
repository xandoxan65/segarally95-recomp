/* Comm-attract inner mode 7 @ 0x115A0 (mountain / alternate course pair). */
/* source: decomp/disasm/maincpu/maincpu_0115a0_4e0.asm */
// @rom 0x115a0 +0x4e0 comm_attract_inner_7

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_mem.h"
#include "comm_attract_script_frame.h"
#include "lift_syms.h"
#include "model2_memory.h"
#include "model2_rom.h"

#include <stdio.h>

#define ATTRACT_STR_PAIR_A     0x0200fbe0u
#define ATTRACT_STR_PAIR_B     0x0202ebf0u
#define ATTRACT_STR_COURSE_A   0x000b7344u
#define ATTRACT_STR_COURSE_B   0x000b1460u
#define ATTRACT_TILE_SCALE     0x2710u

static void attract_tile_string_draw(u32 dest_fp_off, u32 str_vaddr, u32 link)
{
    comm_attract_script_frame_bind_fp();
    g3 = ATTRACT_TILE_SCALE;
    tile_attract_string_draw((void *)(fp + dest_fp_off), str_vaddr, link);
}

static void comm_attract_inner_7_draw_pair(u32 str_a, u32 str_b)
{
    u32 link;

    link = i960_ld_u32(I960_WORKRAM, 0x20a7fc, 0);
    attract_tile_string_draw(0x40, str_a, link);
    attract_tile_string_draw(0xa0, str_b, link);
}

/*
 * Jump tables @ 0x5B0774 / 0x5B08E0 (after table_idx -= 2):
 * course path (0x11914): indices 0,4,5,9,10,11,12
 * pair path   (0x117A8 / 0x11968): 1,2,3,6,7,8
 */
static int inner7_use_course_strings(u32 table_idx)
{
    u32 i;

    if (table_idx < 2u)
        return 0;
    i = table_idx - 2u;
    if (i > 12u)
        return 0;
    switch (i) {
    case 0u:
    case 4u:
    case 5u:
    case 9u:
    case 10u:
    case 11u:
    case 12u:
        return 1;
    default:
        return 0;
    }
}

/* @0x116F4–0x1173C — pen pack with bit4 cleared (script 1). */
static void comm_attract_inner_7_pens_clrbit4(void)
{
    u32 pen_a;
    u32 pen_b;
    u32 a;
    u32 b;

    comm_attract_script_frame_bind_fp();
    pen_a = *(u32 *)(fp + 0x90);
    pen_b = *(u32 *)(fp + 0xf0);
    a = ((pen_a << 1) & 0xfff0u) | (pen_a & 7u);
    a &= ~(1u << 4);
    b = ((pen_b << 1) & 0xfff0u) | (pen_b & 7u);
    b &= ~(1u << 4);
    *(u32 *)(fp + 0x90) = a;
    *(u32 *)(fp + 0x100) = b;
    *(u32 *)(fp + 0xf0) = b;
}

/* @0x117F0–0x118A8 / @0x119C0–0x11A6C — pair-path pen merge into 0x90/0xf0. */
static void comm_attract_inner_7_pens_pair_merge(int allow_nibble_swap)
{
    u32 table_idx;
    u32 script;
    u32 pen_a;
    u32 pen_b;
    u32 a;
    u32 b;
    u32 merged_a;
    u32 merged_b;

    comm_attract_script_frame_bind_fp();
    table_idx = i960_ld_u32(I960_WORKRAM, 0x20a800, 0);
    script = i960_ld_u32(I960_WORKRAM, 0x20a7c4, 0);
    pen_a = *(u32 *)(fp + 0x90);
    pen_b = *(u32 *)(fp + 0xf0);

    a = ((pen_a << 1) & 0xfff0u) | (pen_a & 7u);
    b = ((pen_b << 1) & 0xfff0u) | (pen_b & 7u);
    merged_a = a & ~(1u << 4);
    merged_b = b & ~(1u << 4);

    /* @0x1183C: when table_idx==15 and script==2, nibble-swap low 4 of pens. */
    if (allow_nibble_swap && table_idx == 15u && script == 2u) {
        u32 lo_a = merged_a & 7u;
        u32 lo_b = merged_b & 7u;
        u32 hi_a = merged_a & ~15u;
        u32 hi_b = merged_b & ~15u;

        merged_a = (hi_a | lo_b) & 0xffu;
        merged_a |= (a & 0xffffff00u);
        merged_b = (hi_b | lo_a) & 0xffu;
        merged_b |= (b & 0xffffff00u);
    }

    *(u32 *)(fp + 0x90) = merged_a;
    *(u32 *)(fp + 0xf0) = merged_b;
    if (allow_nibble_swap)
        *(u32 *)(fp + 0x104) = merged_b;
}

static void comm_attract_inner_7_finish(void *arg1)
{
    comm_attract_script_frame_bind_fp();
    comm_attract_geo_script_finish((void *)(fp + 0x40), arg1, 0);
}

static void comm_attract_inner_7_script_dispatch(void)
{
    u32 script;
    u32 table_idx;

    script = i960_ld_u32(I960_WORKRAM, 0x20a7c4, 0);
    table_idx = i960_ld_u32(I960_WORKRAM, 0x20a800, 0);

    /* @0x116A4 — script 1. */
    if (script == 1u) {
        comm_attract_inner_7_draw_pair(ATTRACT_STR_PAIR_A, ATTRACT_STR_PAIR_B);
        comm_attract_inner_7_pens_clrbit4();
        comm_attract_inner_7_finish((void *)(fp + 0xa0));
        return;
    }

    /* @0x11748 — scripts 2 / 3. */
    if (script == 2u || script == 3u) {
        if (inner7_use_course_strings(table_idx)) {
            comm_attract_inner_7_draw_pair(ATTRACT_STR_COURSE_A, ATTRACT_STR_COURSE_B);
            comm_attract_inner_7_finish((void *)(fp + 0xa0));
        } else {
            comm_attract_inner_7_draw_pair(ATTRACT_STR_PAIR_A, ATTRACT_STR_PAIR_B);
            comm_attract_inner_7_pens_pair_merge(1);
            comm_attract_inner_7_finish((void *)(fp + 0xa0));
        }
        return;
    }

    /* @0x118AC — scripts 4 / 5 / 6 / 7. */
    if (script != 4u && script != 5u && script != 6u && script != 7u)
        return;

    if (inner7_use_course_strings(table_idx)) {
        comm_attract_inner_7_draw_pair(ATTRACT_STR_COURSE_A, ATTRACT_STR_COURSE_B);
        comm_attract_inner_7_finish((void *)(fp + 0xa0));
    } else {
        comm_attract_inner_7_draw_pair(ATTRACT_STR_PAIR_A, ATTRACT_STR_PAIR_B);
        comm_attract_inner_7_pens_pair_merge(script == 4u || script == 6u);
        comm_attract_inner_7_finish((void *)(fp + 0xa0));
    }
}

void comm_attract_inner_7(u32 arg0, u32 arg1, u32 arg2)
{
    u32 frame;
    u32 link;
    u32 timer_cur;
    u32 timer_cap;
    u32 course_variant;
    u32 countdown_base;
    u32 threshold;
    u32 counter;
    u32 scene_hook;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    lift_log( "lift: inner7 enter\n");

    /* @0x115A0 */
    comm_attract_script_frame_bind_fp();
    if (comm_attract_slot_scan(0, 0, 0) == 0)
        return;

    i960_call_rom(0xd9d0);

    frame = i960_ld_u32(I960_WORKRAM, 0x20a808, 0);
    if (frame == 30u) {
        if (i960_ld_u8(I960_WORKRAM, 0x202018, 0) != 0u)
            comm_palette_index_call(0x9cu);
        tile_attract_palram_gate(0xe166u);
    }

    comm_attract_carousel_advance(0, 0, 0);

    /* @0x115EC: cmpible 0,g4 — skip advance when link >= 0. */
    link = i960_ld_u32(I960_WORKRAM, 0x20a7fc, 0);
    if ((signed int)link < 0) {
        u32 inner = i960_ld_u32(I960_WORKRAM, 0x20209c, 0);

        i960_st_u32(I960_WORKRAM, 0x20209c, 0, inner + 1u);
    }

    timer_cur = i960_ld_u32(I960_WORKRAM, 0x20a7f8, 0);
    timer_cap = i960_ld_u32(I960_WORKRAM, 0x20a800, 0);
    if (timer_cur != timer_cap)
        i960_st_u32(I960_WORKRAM, 0x20a804, 0, 0);

    /* @0x11640 — threshold offset 0xffffffb5 (−75); lda, not ld. */
    course_variant = i960_ld_u32(I960_WORKRAM, COURSE_VARIANT_INDEX, 0);
    countdown_base = model2_workram_mirror_u32(0x005ae460u + (course_variant << 2));
    frame = i960_ld_u32(I960_WORKRAM, 0x20a808, 0);
    i960_st_u32(I960_WORKRAM, 0x20a7f8, 0, timer_cap);
    threshold = countdown_base + 0xffffffb5u;
    if (frame == threshold) {
        g0 = 12;
        comm_attract_threshold_tile_clear(12u, 0, 0);
    }

    counter = i960_ld_u32(I960_WORKRAM, 0x20a794, 0) + 1u;
    i960_st_u32(I960_WORKRAM, 0x20a794, 0, counter);
    if (counter == 1001u) {
        scene_hook = i960_ld_u32(I960_WORKRAM, 0x20a78c, 0);
        if (scene_hook == 0x005b1cc0u) {
            i960_st_u32(I960_WORKRAM, 0x20a790, 0, 0);
            i960_st_u32(I960_WORKRAM, 0x20a78c, 0, 0x005b1d90u);
        }
    }

    geo_fifo_bootstrap(0, 0, 0);
    comm_attract_script_frame_setup(0, 0, 0);
    comm_attract_inner_7_script_dispatch();
    lift_log( "lift: inner7 done\n");
}
