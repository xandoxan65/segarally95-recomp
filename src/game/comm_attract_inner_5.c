/* Comm-attract inner mode 5 @ 0x11040 (post-reseed course / desert pair). */
/* source: decomp/disasm/maincpu/maincpu_011040_360.asm */
// @rom 0x11040 +0x360 comm_attract_inner_5

#include "i960_lift.h"
#include "i960_mem.h"
#include "comm_attract_script_frame.h"
#include "lift_syms.h"
#include "model2_rom.h"

#include <stdio.h>

#define ATTRACT_STR_PAIR_A     0x028e9258u
#define ATTRACT_STR_PAIR_B     0x028f99acu
#define ATTRACT_STR_COURSE_A   0x000ad414u
#define ATTRACT_STR_COURSE_B   0x000a93c8u
#define ATTRACT_TILE_SCALE     0x2710u

static void attract_tile_string_draw(u32 dest_fp_off, u32 str_vaddr, u32 link)
{
    comm_attract_script_frame_bind_fp();
    g3 = ATTRACT_TILE_SCALE;
    tile_attract_string_draw((void *)(fp + dest_fp_off), str_vaddr, link);
}

static void comm_attract_inner_5_draw_pair(u32 str_a, u32 str_b)
{
    u32 link;

    link = i960_ld_u32(I960_WORKRAM, 0x20a7fc, 0);
    attract_tile_string_draw(0x40, str_a, link);
    attract_tile_string_draw(0xa0, str_b, link);
}

/* @0x11354 — swap low nibbles between 0x90(fp) and 0xf0(fp) (obj+0x50). */
static void comm_attract_inner_5_pen_nibble_swap(void)
{
    u32 pen_a;
    u32 pen_b;

    comm_attract_script_frame_bind_fp();
    pen_a = *(unsigned char *)(fp + 0x90);
    pen_b = *(unsigned char *)(fp + 0xf0);
    *(unsigned char *)(fp + 0x90) =
        (unsigned char)((pen_a & 0xf0u) | (pen_b & 0x0fu));
    *(unsigned char *)(fp + 0xf0) =
        (unsigned char)((pen_b & 0xf0u) | (pen_a & 0x0fu));
}

/*
 * Pens come from keyframe +0x50 via string_draw. ROM has no 0x22220000
 * rewrite here (that path is inner_3 desert @ 0x10C20). Invented seed_pens
 * forced both cars onto the same pen (Celica).
 */
static void comm_attract_inner_5_finish(int do_swap)
{
    if (do_swap)
        comm_attract_inner_5_pen_nibble_swap();
    comm_attract_script_frame_bind_fp();
    comm_attract_geo_script_finish((void *)(fp + 0x40), (void *)(fp + 0xa0), 0);
}

/*
 * cmpibg lit,reg / cmpibl lit,reg use i960 src1 ? src2 (lit vs g4).
 * Course-string window: table==1 or table in {10,11}.
 */
static int inner5_use_course_strings(u32 table_idx)
{
    return table_idx == 1u || table_idx == 10u || table_idx == 11u;
}

static void comm_attract_inner_5_script_dispatch(void)
{
    u32 script;
    u32 table_idx;

    script = i960_ld_u32(I960_WORKRAM, 0x20a7c4, 0);
    table_idx = i960_ld_u32(I960_WORKRAM, 0x20a800, 0);

    /* @0x1114C — script 1: pair strings, finish (no swap). */
    if (script == 1u) {
        comm_attract_inner_5_draw_pair(ATTRACT_STR_PAIR_A, ATTRACT_STR_PAIR_B);
        comm_attract_inner_5_finish(0);
        return;
    }

    /* @0x111A4 — scripts 2 / 3. */
    if (script == 2u || script == 3u) {
        if (inner5_use_course_strings(table_idx)) {
            comm_attract_inner_5_draw_pair(ATTRACT_STR_COURSE_A, ATTRACT_STR_COURSE_B);
            /* @0x11214: swap only when table==1. */
            comm_attract_inner_5_finish(table_idx == 1u);
        } else {
            comm_attract_inner_5_draw_pair(ATTRACT_STR_PAIR_A, ATTRACT_STR_PAIR_B);
            /* @0x11264–0x11274: swap when table==12 && script==2. */
            comm_attract_inner_5_finish(table_idx == 12u && script == 2u);
        }
        return;
    }

    /* @0x11278 — scripts 4 / 5 / 6 / 7; other scripts return. */
    if (script != 4u && script != 5u && script != 6u && script != 7u)
        return;

    if (inner5_use_course_strings(table_idx)) {
        comm_attract_inner_5_draw_pair(ATTRACT_STR_COURSE_A, ATTRACT_STR_COURSE_B);
        comm_attract_inner_5_finish(table_idx == 1u);
    } else {
        comm_attract_inner_5_draw_pair(ATTRACT_STR_PAIR_A, ATTRACT_STR_PAIR_B);
        /* @0x11344–0x11350: swap when table==12 && (script==4 || script==6). */
        comm_attract_inner_5_finish(
            table_idx == 12u && (script == 4u || script == 6u));
    }
}

void comm_attract_inner_5(u32 arg0, u32 arg1, u32 arg2)
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

    fprintf(stderr, "lift: inner5 enter\n");

    /* @0x11040 */
    comm_attract_script_frame_bind_fp();
    if (comm_attract_slot_scan(0, 0, 0) == 0)
        return;

    /* @0x11050 */
    i960_call_rom(0xd9d0);

    /* @0x11054–0x1107C */
    frame = i960_ld_u32(I960_WORKRAM, 0x20a808, 0);
    if (frame == 30u) {
        if (i960_ld_u8(I960_WORKRAM, 0x202018, 0) != 0u)
            comm_palette_index_call(0x9cu);
        tile_attract_palram_gate(0xe166u);
    }

    /* @0x11080 */
    comm_attract_carousel_advance(0, 0, 0);

    /* @0x1108C: cmpible 0,g4 — skip advance when link >= 0. */
    link = i960_ld_u32(I960_WORKRAM, 0x20a7fc, 0);
    if ((signed int)link < 0) {
        u32 inner = i960_ld_u32(I960_WORKRAM, 0x20209c, 0);

        i960_st_u32(I960_WORKRAM, 0x20209c, 0, inner + 1u);
    }

    /* @0x110A4–0x110B8 — note: stores 0x20a804 when timers *differ* (disasm). */
    timer_cur = i960_ld_u32(I960_WORKRAM, 0x20a7f8, 0);
    timer_cap = i960_ld_u32(I960_WORKRAM, 0x20a800, 0);
    if (timer_cur != timer_cap)
        i960_st_u32(I960_WORKRAM, 0x20a804, 0, 0);

    /* @0x110C0–0x110E8 — threshold offset 0xffffffb9 (−71); lda, not ld. */
    course_variant = i960_ld_u32(I960_WORKRAM, COURSE_VARIANT_INDEX, 0);
    countdown_base = model2_workram_mirror_u32(0x005ae460u + (course_variant << 2));
    frame = i960_ld_u32(I960_WORKRAM, 0x20a808, 0);
    i960_st_u32(I960_WORKRAM, 0x20a7f8, 0, timer_cap);
    threshold = countdown_base + 0xffffffb9u;
    if (frame == threshold) {
        g0 = 19;
        comm_attract_threshold_tile_clear(19u, 0, 0);
    }

    /* @0x110F4–0x1113C */
    counter = i960_ld_u32(I960_WORKRAM, 0x20a794, 0) + 1u;
    i960_st_u32(I960_WORKRAM, 0x20a794, 0, counter);
    if (counter == 1001u) {
        scene_hook = i960_ld_u32(I960_WORKRAM, 0x20a78c, 0);
        if (scene_hook == 0x005b1cc0u) {
            i960_st_u32(I960_WORKRAM, 0x20a790, 0, 0);
            i960_st_u32(I960_WORKRAM, 0x20a78c, 0, 0x005b1d90u);
        }
    }

    /* @0x1113C–0x11144 */
    geo_fifo_bootstrap(0, 0, 0);
    comm_attract_script_frame_setup(0, 0, 0);
    comm_attract_inner_5_script_dispatch();
    fprintf(stderr, "lift: inner5 done\n");
}
