/* Alternate attract scene HUD @ 0x12D90 (hook 0x5B1D90 after inner_3 frame 1001).
 * Board type 3 returns immediately — do not invent row-scroll or stamp work.
 *
 * flag==0: clear + stamps + ranking digit loop (r11=1).
 * flag!=0: ranking refresh only (r11=9) — not an early epilogue skip.
 * Epilogue @ 0x1320C: copyright stamps + hook swap to 0x5B2340.
 */
// @rom 0x12d90 +0x280 comm_attract_scene_hud_alt

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"

extern void comm_attract_hud_scroll_fill_dual(u32 arg0, u32 arg1, u32 arg2);
extern u32 attract_rank_name_lookup(u32 needle_va);
extern void tile_attract_time_split(u32 value, u16 out[4]);
extern u32 erase_scene_dispatch(u32 arg0, u32 arg1, u32 arg2);

static void hud_draw(u32 x, u32 y, u32 slot, u32 glyph, u32 flags)
{
    g0 = x;
    g1 = y;
    g2 = slot;
    g3 = glyph;
    g4 = flags;
    draw_scene_dispatch(x, y, slot);
}

static void hud_erase(u32 x, u32 y, u32 slot, u32 glyph, u32 flags)
{
    g0 = x;
    g1 = y;
    g2 = slot;
    g3 = glyph;
    g4 = flags;
    erase_scene_dispatch(x, y, slot);
}

static u32 row_y(u32 row)
{
    /* lda 0xc(r7)[r7*2] → 3*row + 12 */
    return row * 3u + 12u;
}

static void paint_name_fallback(u32 r7, u32 r8, u32 r15)
{
    u32 r4 = 0;
    u32 r5 = 17;
    u32 y = row_y(r7);
    u32 g4v;
    u32 g3v;
    u32 slot;

    do {
        g4v = i960_ld_u8(I960_ABS, 0x01d003bcu + r8 + r4, 0);
        slot = i960_ld_u32(I960_WORKRAM, 0x20a798, 0);
        if (g4v == r15) {
            hud_erase(r5, y, slot, 33u /* 31+2 */, 2);
        } else {
            g3v = g4v + 0xffffffe0u; /* lda 0xffffffe0(g3) */
            hud_draw(r5, y, slot, g3v, 2);
        }
        r4++;
        r5 += 3u;
    } while ((i32)r4 <= 2);
}

static void ranking_digit_loop(u32 r11)
{
    u32 r9;
    u32 r8;
    u32 r10;
    u32 r12;
    u32 r13;
    u32 r14;
    u32 r15;
    u32 r7;
    u32 g3v;
    u32 g4v;
    u32 slot;
    u32 flag;
    u32 match;
    u32 y;
    u16 time_buf[4];

    /* @0x12EE0 */
    comm_attract_hud_scroll_fill_dual(0, 0, 0);

    /* @0x12EE4–0x12EF0: empty range → epilogue */
    r9 = r11;
    if ((i32)r9 >= (i32)(r11 + 8u))
        return;

    /* @0x12EF4–0x12F24: seed row pointers from r11 (stride 12). */
    r8 = r11 * 12u;
    r14 = 0x01d003c0u + r8;
    r12 = 0x01d002f0u + r8;
    r13 = 0x01d003b8u + r8;
    r10 = 0x01d003c2u + r8;
    r15 = 32u; /* addo 31,1 */

    for (;;) {
        flag = i960_ld_u32(I960_WORKRAM, 0x20a790, 0);
        r7 = r9;
        if (flag != 0u)
            r7 = r9 - 8u;
        y = row_y(r7);

        g3v = r9 / 10u;
        if (g3v != 0u) {
            slot = i960_ld_u32(I960_WORKRAM, 0x20a798, 0);
            hud_draw(8, y, slot, 0x10u + g3v, 2);
        }

        g3v = r9 % 10u;
        slot = i960_ld_u32(I960_WORKRAM, 0x20a798, 0);
        hud_draw(11, y, slot, 0x10u + g3v, 2);

        g4v = i960_ld_u16(I960_ABS, r10, 0);
        if (g4v == 0u) {
            paint_name_fallback(r7, r8, r15);
        } else {
            match = attract_rank_name_lookup(r12 + 0xccu);
            if ((i32)match >= 0) {
                slot = i960_ld_u32(I960_WORKRAM, 0x20a7b0, 0);
                hud_draw(17, y, slot, match, 2);
            } else {
                i960_st_u16(I960_ABS, r10, 0, 0);
                paint_name_fallback(r7, r8, r15);
            }
        }

        tile_attract_time_split(i960_ld_u32(I960_ABS, r13, 0), time_buf);

        slot = i960_ld_u32(I960_WORKRAM, 0x20a798, 0);
        hud_draw(33u /* 31+2 */, y, slot, 7, 2);
        slot = i960_ld_u32(I960_WORKRAM, 0x20a798, 0);
        hud_draw(38u /* 31+7 */, y, slot, 2, 2);

        g3v = time_buf[1] / 10u;
        if (g3v != 0u) {
            slot = i960_ld_u32(I960_WORKRAM, 0x20a798, 0);
            hud_draw(29, y, slot, 0x3bu + g3v, 2);
        }
        g3v = time_buf[1] % 10u;
        slot = i960_ld_u32(I960_WORKRAM, 0x20a798, 0);
        hud_draw(31, y, slot, 0x3bu + g3v, 2);

        g3v = time_buf[2] / 10u;
        slot = i960_ld_u32(I960_WORKRAM, 0x20a798, 0);
        hud_draw(34u /* 31+3 */, y, slot, 0x3bu + g3v, 2);
        g3v = time_buf[2] % 10u;
        slot = i960_ld_u32(I960_WORKRAM, 0x20a798, 0);
        hud_draw(36u /* 31+5 */, y, slot, 0x3bu + g3v, 2);

        g3v = time_buf[3] / 10u;
        slot = i960_ld_u32(I960_WORKRAM, 0x20a798, 0);
        hud_draw(39u /* 31+8 */, y, slot, 0x3bu + g3v, 2);
        g3v = time_buf[3] % 10u;
        slot = i960_ld_u32(I960_WORKRAM, 0x20a798, 0);
        hud_draw(41u /* 31+10 */, y, slot, 0x3bu + g3v, 2);

        slot = i960_ld_u32(I960_WORKRAM, 0x20a79c, 0);
        g3v = i960_ld_u16(I960_ABS, r14, 0);
        hud_draw(45u /* 31+14 */, y, slot, g3v, 2);

        r9++;
        r14 += 12u;
        r12 += 12u;
        r8 += 12u;
        r13 += 12u;
        r10 += 12u;
        if ((i32)r9 >= (i32)(r11 + 8u))
            break;
    }
}

void comm_attract_scene_hud_alt(u32 arg0, u32 arg1, u32 arg2)
{
    u32 board;
    u32 flag;
    u32 r11;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    board = i960_ld_u32(I960_WORKRAM, 0x20a530, 0);
    if (board == 3u)
        return;

    i960_st_u16(I960_WORKRAM, 0x20b914, 0, 0x8000u);
    i960_st_u16(I960_WORKRAM, 0x20b91c, 0, 0x4000u);

    flag = i960_ld_u32(I960_WORKRAM, 0x20a790, 0);
    if (flag != 0u) {
        r11 = 9;
        ranking_digit_loop(r11);
        goto epilogue;
    }

    g0 = 0x01002000u;
    i960_call_rom(0x26b60);
    g0 = 15;
    g1 = 30;
    g2 = 35;
    g3 = 6;
    i960_call_rom(0x27260);
    g0 = 7;
    g1 = 5;
    g2 = 55;
    g3 = 21;
    i960_call_rom(0x27260);

    comm_attract_hud_scroll_fill(0, 0, 0);

    g0 = 0;
    g1 = 0;
    g2 = 0x028af104u;
    g3 = 0;
    g4 = 4;
    g0 = catalog_draw_setup((u32)g0, (u32)g1, (u32)g2);
    i960_st_u32(I960_WORKRAM, 0x20a79c, 0, (u32)g0);

    g0 = 0;
    g1 = 0;
    g2 = 0x028ccaf8u;
    g3 = 0;
    g4 = 4;
    g0 = catalog_draw_setup((u32)g0, (u32)g1, (u32)g2);
    i960_st_u32(I960_WORKRAM, 0x20a7b0, 0, (u32)g0);

    /*
     * Desert CGM @ 0x28AF104 / 0x28CCAF8: leading @ 0x2A050 enqueues via
     * 0x26918; irq @ 0x26980 drains with 0x268B0 into scratch rows at
     * 0x01800000+(slot<<5). Host may present before vsync — drain now.
     * GEO colorbase words @ 0x01802000 are only filled by boot @ 0x26758
     * (and palram_geo_colorbase_refresh @ 0x333C8) from maincpu 0x5FB89E — not by 0x26918.
     */
    boot_tile_splash_frame(0, 0, 0);

    g2 = i960_ld_u32(I960_WORKRAM, 0x20a7b4, 0);
    g0 = 20;
    g1 = 3;
    g3 = 0;
    g4 = 2;
    draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

    g2 = i960_ld_u32(I960_WORKRAM, 0x20a79c, 0);
    g0 = 7;
    g1 = 11;
    g3 = 8;
    g4 = 2;
    draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

    g2 = i960_ld_u32(I960_WORKRAM, 0x20a79c, 0);
    g0 = 17;
    g1 = 11;
    g3 = 7;
    g4 = 2;
    draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

    g2 = i960_ld_u32(I960_WORKRAM, 0x20a79c, 0);
    g0 = 29;
    g1 = 11;
    g3 = 6;
    g4 = 2;
    draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

    g2 = i960_ld_u32(I960_WORKRAM, 0x20a79c, 0);
    g0 = 45;
    g1 = 11;
    g3 = 4;
    g4 = 2;
    draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

    r11 = 1;
    ranking_digit_loop(r11);

epilogue:
    comm_attract_hud_copyright_stamps(0, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20a78c, 0, 0x005b2340u);
}
