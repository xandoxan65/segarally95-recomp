/* Comm-attract inner mode 3 @ 0x10940 (carousel / course banner). */
// @rom 0x10940 +0x4f0 comm_attract_inner_3

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "comm_attract_script_frame.h"
#include "lift_syms.h"
#include "model2_memory.h"
#include "model2_rom.h"
#include "model2_snd.h"

#include <stdio.h>

#define ATTRACT_STR_DESERT_A   0x0203e420u
#define ATTRACT_STR_DESERT_B   0x0201f410u
#define ATTRACT_STR_COURSE_A   0x000a0bfcu
#define ATTRACT_STR_COURSE_B   0x00098430u
#define ATTRACT_STR_BANNER     0x0290a100u
#define ATTRACT_TILE_SCALE     0x2710u

static void attract_tile_string_draw(u32 dest_fp_off, u32 str_vaddr, u32 link)
{
    lift_log( "lift: string_draw dest=fp+%#x str=%#x link=%#x\n",
            dest_fp_off, str_vaddr, link);
    comm_attract_script_frame_bind_fp();
    g3 = ATTRACT_TILE_SCALE;
    tile_attract_string_draw((void *)(fp + dest_fp_off), str_vaddr, link);
    lift_log( "lift: string_draw done\n");
}

static void comm_attract_inner_3_draw_pair(u32 str_a, u32 str_b)
{
    u32 link;

    link = i960_ld_u32(I960_WORKRAM, 0x20a7fc, 0);
    attract_tile_string_draw(0x40, str_a, link);
    attract_tile_string_draw(0xa0, str_b, link);
}

/*
 * Desert-only @ 0x10C20 / 0x10DC8: lda 0x22220000(pen&7) is an address
 * form (not a table load), then OR into (pen<<1)&0xfff0. Course pairs keep
 * keyframe +0x50 pens unchanged.
 */
static void comm_attract_inner_3_desert_pen_rewrite(void)
{
    u32 pen_a;
    u32 pen_b;
    u32 band;
    u32 code;

    pen_a = comm_attract_fp_u32(0x90);
    pen_b = comm_attract_fp_u32(0xf0);

    band = (pen_a << 1) & 0xfff0u;
    code = 0x22220000u + (pen_a & 7u);
    comm_attract_fp_st_u32(0x90, band | code);

    band = (pen_b << 1) & 0xfff0u;
    code = 0x22220000u + (pen_b & 7u);
    /* @0x10C6C/@0x10E14 also poke 0x104/0x108; live pen is 0xf0. */
    comm_attract_fp_st_u32(0xf0, band | code);
}

/*
 * Disasm @ 0x10E1C / 0x10D4C: g0 = 0x40(fp), g1 = second object or 0.
 * Pair paths set g1 = 0xa0(fp) (movl r4,g1 after second string_draw).
 * Banner @ 0x10D44: mov 0,g1. Same-pointer calls were wrong for modes that
 * read g1/r5 (and the epilogue arg0!=r5 / r5!=0 branches).
 */
static void comm_attract_inner_3_script_finish_pair(void)
{
    comm_attract_script_frame_bind_fp();
    comm_attract_geo_script_finish((void *)(fp + 0x40), (void *)(fp + 0xa0), 0);
}

static void comm_attract_inner_3_script_finish_banner(void)
{
    comm_attract_script_frame_bind_fp();
    comm_attract_geo_script_finish((void *)(fp + 0x40), NULL, 0);
}

static void comm_attract_inner_3_draw_banner(void)
{
    u32 link;

    comm_attract_script_frame_bind_fp();
    link = i960_ld_u32(I960_WORKRAM, 0x20a7fc, 0);
    attract_tile_string_draw(0x40, ATTRACT_STR_BANNER, link);
    /* @0x10D44: g1 = 0 after banner-only string_draw. */
    comm_attract_inner_3_script_finish_banner();
}

/* @0x10D54: banner string_draw then g1 = g0 (same object). */
static void comm_attract_inner_3_draw_banner_same(void)
{
    u32 link;

    comm_attract_script_frame_bind_fp();
    link = i960_ld_u32(I960_WORKRAM, 0x20a7fc, 0);
    attract_tile_string_draw(0x40, ATTRACT_STR_BANNER, link);
    comm_attract_script_frame_bind_fp();
    comm_attract_geo_script_finish((void *)(fp + 0x40), (void *)(fp + 0x40), 0);
}

static void comm_attract_inner_3_desert_pair(void)
{
    comm_attract_inner_3_draw_pair(ATTRACT_STR_DESERT_A, ATTRACT_STR_DESERT_B);
    comm_attract_inner_3_desert_pen_rewrite();
    comm_attract_inner_3_script_finish_pair();
}

static void comm_attract_inner_3_course_pair(void)
{
    comm_attract_inner_3_draw_pair(ATTRACT_STR_COURSE_A, ATTRACT_STR_COURSE_B);
    comm_attract_inner_3_script_finish_pair();
}

/*
 * Jump table @ 0x5AFA6C after `subo 6` (@0x10A58).
 * table_idx < 6 wraps unsigned and takes the desert default @ 0x10A9C.
 */
static void comm_attract_inner_3_script1(u32 table_idx)
{
    u32 i = table_idx - 6u;

    if (i > 11u) {
        comm_attract_inner_3_desert_pair();
        return;
    }
    switch (i) {
    case 0u: /* 0x10D24 */
    case 5u:
        comm_attract_inner_3_draw_banner();
        break;
    case 1u: /* 0x10A9C */
    case 6u:
    case 7u:
    case 8u:
    case 9u:
    case 10u:
        comm_attract_inner_3_desert_pair();
        break;
    case 2u: /* 0x10D54 */
    case 3u:
    case 4u:
    case 11u:
        comm_attract_inner_3_draw_banner_same();
        break;
    default:
        break;
    }
}

/*
 * Jump table @ 0x5AFB60 after `subo 4` (@0x10B4C). Default @ 0x10BDC = desert.
 * Index 7 → 0x10BCC: script==2 → banner g1=0.
 */
static void comm_attract_inner_3_script2(u32 table_idx)
{
    u32 i = table_idx - 4u;

    if (i > 7u) {
        comm_attract_inner_3_desert_pair();
        return;
    }
    switch (i) {
    case 0u: /* 0x10B80 */
    case 2u:
    case 3u:
    case 4u:
    case 6u:
        comm_attract_inner_3_course_pair();
        break;
    case 1u: /* 0x10BDC */
        comm_attract_inner_3_desert_pair();
        break;
    case 5u: /* 0x10D54 */
        comm_attract_inner_3_draw_banner_same();
        break;
    case 7u: /* 0x10BCC → 0x10D24 when script==2 */
        comm_attract_inner_3_draw_banner();
        break;
    default:
        break;
    }
}

/*
 * Jump table @ 0x5AFCA8 after `subo 4` (@0x10C94). Default @ 0x10D84 = desert.
 * Index 7 → 0x10D14: script 4 or 6 → banner g1=0; else banner same.
 */
static void comm_attract_inner_3_script4567(u32 script, u32 table_idx)
{
    u32 i = table_idx - 4u;

    if (i > 7u) {
        comm_attract_inner_3_desert_pair();
        return;
    }
    switch (i) {
    case 0u: /* 0x10CC8 */
    case 2u:
    case 3u:
    case 4u:
    case 6u:
        comm_attract_inner_3_course_pair();
        break;
    case 1u: /* 0x10D84 */
        comm_attract_inner_3_desert_pair();
        break;
    case 5u: /* 0x10D54 */
        comm_attract_inner_3_draw_banner_same();
        break;
    case 7u: /* 0x10D14 */
        if (script == 4u || script == 6u)
            comm_attract_inner_3_draw_banner();
        else
            comm_attract_inner_3_draw_banner_same();
        break;
    default:
        break;
    }
}

static void comm_attract_inner_3_script_dispatch(void)
{
    u32 script;
    u32 table_idx;

    script = i960_ld_u32(I960_WORKRAM, 0x20a7c4, 0);
    table_idx = i960_ld_u32(I960_WORKRAM, 0x20a800, 0);

    /* @0x10A44 */
    if (script == 1u) {
        comm_attract_inner_3_script1(table_idx);
        return;
    }

    /* @0x10B34 */
    if (script == 2u) {
        comm_attract_inner_3_script2(table_idx);
        return;
    }

    if (script == 3u)
        return;

    /* @0x10C74–0x10C8C: scripts 4/5/6/7 share the 0x5AFCA8 jump table. */
    if (script == 4u || script == 5u || script == 6u || script == 7u) {
        comm_attract_inner_3_script4567(script, table_idx);
        return;
    }
}

void comm_attract_inner_3(u32 arg0, u32 arg1, u32 arg2)
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

    lift_log( "lift: inner3 enter\n");

    /* @0x10944 */
    comm_attract_script_frame_bind_fp();
    if (comm_attract_slot_scan(0, 0, 0) == 0)
        return;

    lift_log( "lift: inner3 after slot_scan\n");

    /* @0x10950 — board comm update (unlifted). */
    i960_call_rom(0xd9d0);

    /* @0x10954–0x1097C */
    frame = i960_ld_u32(I960_WORKRAM, 0x20a808, 0);
    if (frame == 30u) {
        if (i960_ld_u8(I960_WORKRAM, 0x202018, 0) != 0u)
            comm_palette_index_call(0x9cu);
        else if (model2_snd_log_enabled())
            fprintf(stderr, "lift: sound skip index=0x9c advertise=0\n");
        tile_attract_palram_gate(0xe166u);
    }

    /* @0x10980 */
    comm_attract_carousel_advance(0, 0, 0);

    /*
     * @0x1098C: cmpible 0,g4 → branch (skip) when 0 <= link.
     * Advance inner only when link is strictly negative.
     */
    link = i960_ld_u32(I960_WORKRAM, 0x20a7fc, 0);
    if ((signed int)link < 0) {
        u32 inner = i960_ld_u32(I960_WORKRAM, 0x20209c, 0);
        i960_st_u32(I960_WORKRAM, 0x20209c, 0, inner + 1u);
    }

    /* @0x109B4: cmpibe → skip clear when equal; clear 0x20a804 when timers differ. */
    timer_cur = i960_ld_u32(I960_WORKRAM, 0x20a7f8, 0);
    timer_cap = i960_ld_u32(I960_WORKRAM, 0x20a800, 0);
    if (timer_cur != timer_cap)
        i960_st_u32(I960_WORKRAM, 0x20a804, 0, 0);

    /* @0x109C0–0x109D8 */
    course_variant = i960_ld_u32(I960_WORKRAM, COURSE_VARIANT_INDEX, 0);
    countdown_base = model2_workram_mirror_u32(0x005ae460u + (course_variant << 2));
    frame = i960_ld_u32(I960_WORKRAM, 0x20a808, 0);
    i960_st_u32(I960_WORKRAM, 0x20a7f8, 0, timer_cap);

    /* @0x109E0–0x109F0 — lda (address arithmetic), not ld. */
    threshold = countdown_base + 0xffffffb6u;
    if (frame == threshold) {
        g0 = 12;
        comm_attract_threshold_tile_clear(12u, 0, 0);
    }

    /* @0x109F4–0x10A3C */
    counter = i960_ld_u32(I960_WORKRAM, 0x20a794, 0) + 1u;
    i960_st_u32(I960_WORKRAM, 0x20a794, 0, counter);
    if (counter == 1001u) {
        scene_hook = i960_ld_u32(I960_WORKRAM, 0x20a78c, 0);
        if (scene_hook == 0x005b1cc0u) {
            i960_st_u32(I960_WORKRAM, 0x20a790, 0, 0);
            i960_st_u32(I960_WORKRAM, 0x20a78c, 0, 0x005b1d90u);
        }
    }

    /* @0x10A3C–0x10A40 */
    lift_log( "lift: inner3 before geo_fifo + script\n");
    geo_fifo_bootstrap(0, 0, 0);
    lift_log( "lift: inner3 after geo_fifo\n");
    lift_log( "lift: inner3 before script_frame_setup\n");
    comm_attract_script_frame_setup(0, 0, 0);
    lift_log( "lift: inner3 after script_frame_setup\n");
    lift_log( "lift: inner3 before script_dispatch script=%u\n",
            (unsigned)i960_ld_u32(I960_WORKRAM, 0x20a7c4, 0));
    comm_attract_inner_3_script_dispatch();
    lift_log( "lift: inner3 done\n");
}
