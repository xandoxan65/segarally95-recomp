/* GAME ASSIGNMENTS @ 0x6F80 — menu slot 5 (country / options).
 * source: disasm/maincpu/maincpu_006f80_6a0.asm */
// @rom 0x6f80 +0x6a0 test_menu_game_assign

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>
#include <string.h>

static void printf_guest(u32 fmt_va)
{
    uintptr_t fp_save = fp;

    fp = 0;
    libc_printf((const char *)(uintptr_t)fmt_va, (u32)g1, (u32)g2);
    fp = fp_save;
}

static void frame_ldq(u8 *frame, u32 dst_off, u32 src_va)
{
    u32 i;

    for (i = 0; i < 4u; i++)
        *(u32 *)(frame + dst_off + (i << 2)) = i960_ld_u32(I960_WORKRAM, src_va + (i << 2), 0);
}

static u32 pack_opt(u8 *frame, u32 off, u32 shift, u32 val, u32 mask_clear, u32 maxv)
{
    u32 word = *(u32 *)(frame + off);

    if (val > maxv)
        val = maxv;
    word = (word & mask_clear) | ((val & 0xffu) << shift);
    *(u32 *)(frame + off) = word;
    return val;
}

static void bump_opt(u8 *frame, u32 byte_off, u32 maxv)
{
    u8 v = frame[0x130 + byte_off];

    v++;
    if (v > maxv)
        v = 0;
    frame[0x130 + byte_off] = v;
    i960_st_u32(I960_WORKRAM, 0x20a400, 0, 1u);
}

u32 test_menu_game_assign(u32 arg0, u32 arg1, u32 arg2)
{
    u8 frame[0x140];
    uintptr_t fp_save = fp;
    u32 cursor;
    u32 row;
    u32 flags;
    u32 bits;
    u32 g8_off;
    u32 r12;
    u32 label;
    u32 val;
    u32 idx;

    (void)arg1;
    (void)arg2;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;

    /* String / value tables (ldq/ld from ROM mirror). */
    frame_ldq(frame, 0x40, 0x5a5d50u);
    frame_ldq(frame, 0x50, 0x5a5d60u);
    frame_ldq(frame, 0x60, 0x5a5d70u);
    frame_ldq(frame, 0x70, 0x5a5d80u);
    frame_ldq(frame, 0x80, 0x5a5d90u);
    frame_ldq(frame, 0x90, 0x5a5da0u);
    frame_ldq(frame, 0xa0, 0x5a5db0u);
    frame_ldq(frame, 0xb0, 0x5a5dc0u);
    frame_ldq(frame, 0xc0, 0x5a5dd0u);
    frame_ldq(frame, 0xd0, 0x5a5de0u);
    frame_ldq(frame, 0xe0, 0x5a5df0u);
    frame_ldq(frame, 0xf0, 0x5a5e00u);
    frame_ldq(frame, 0x100, 0x5a5f30u);
    frame_ldq(frame, 0x110, 0x5a5f40u);
    *(u32 *)(frame + 0x120) = i960_ld_u32(I960_WORKRAM, 0x5a5f50u, 0);

    if (arg0 != 0) {
        i960_st_u32(I960_WORKRAM, 0x20a2d8, 0, 0);
        i960_st_u32(I960_WORKRAM, 0x20a2d4, 0, 8u);
    }

    /* Pack live options into frame+0x130 (clamped). */
    pack_opt(frame, 0x130, 0, i960_ld_u8(I960_WORKRAM, 0x202018, 0), 0xffffff00u, 1u);
    pack_opt(frame, 0x130, 8, i960_ld_u8(I960_WORKRAM, 0x202019, 0), 0xffff00ffu, 2u);
    pack_opt(frame, 0x130, 16, i960_ld_u8(I960_WORKRAM, 0x20201a, 0), 0xff00ffffu, 1u);
    pack_opt(frame, 0x130, 24, i960_ld_u8(I960_WORKRAM, 0x20201b, 0), 0x00ffffffu, 5u);
    pack_opt(frame, 0x134, 0, i960_ld_u8(I960_WORKRAM, 0x20201c, 0), 0xffffff00u, 3u);
    pack_opt(frame, 0x134, 8, i960_ld_u8(I960_WORKRAM, 0x20201d, 0), 0xffff00ffu, 3u);
    pack_opt(frame, 0x134, 16, i960_ld_u8(I960_WORKRAM, 0x20201e, 0), 0xff00ffffu, 3u);

    tile_cursor_seed(18u, 9u);
    printf_guest(0x5a5f60u);
    tile_cursor_seed(18u, 14u);

    g8_off = 0;
    r12 = 0;
    for (row = 0; row <= 8u; row++) {
        if (i960_ld_u32(I960_WORKRAM, 0x20a554, 0) == 0
            && (row == 2u || row == 3u || row == 6u || row == 7u))
            goto row_next;

        cursor = i960_ld_u32(I960_WORKRAM, 0x20a2d4, 0);
        scene_view_scale(0, 0, 0);
        label = *(u32 *)(frame + 0x100 + r12);
        if (cursor == row) {
            /* Same marker path as main TEST MENU (glyph_pair @ 0x5CC0). */
            g0 = label;
            g1 = 1;
            geo_scene_glyph_pair((u32)g0, (u32)g1, (u32)g2);
        } else {
            boot_tile_script_run(0x5a4e14u);
            boot_tile_script_run(label);
        }

        if (row != 8u) {
            idx = (row == 7u) ? 6u : row;
            boot_tile_script_run(0x5a5f78u);
            scene_view_scale(6, 0, 0);
            val = frame[0x130 + idx];
            boot_tile_script_run(*(u32 *)(frame + 0x40 + g8_off + (val << 2)));
        }
        boot_tile_opcode_dispatch(10u);
        boot_tile_opcode_dispatch(10u);

row_next:
        g8_off += 0x18u;
        r12 += 4u;
    }

    boot_tile_script_run(0x5a4e18u);
    scene_view_scale(0, 0, 0);
    g0 = 2;
    geo_scene_row_print((u32)g0, (u32)g1, (u32)g2);

    /* SERVICE1 → advance cursor */
    flags = i960_ld_u32(I960_WORKRAM, 0x202074, 0);
    if (flags & 2u)
        goto move_cursor;
    flags = i960_ld_u8(I960_WORKRAM, 0x202064, 0);
    if ((flags >> 3) & 1u)
        goto move_cursor;

    bits = 0;
    flags = i960_ld_u8(I960_WORKRAM, 0x202064, 0);
    bits |= (flags >> 5) & 1u;
    bits |= (flags >> 5) & 2u;
    bits |= (flags >> 5) & 4u;
    bits |= (i960_ld_u8(I960_WORKRAM, 0x202065, 0) & 1u) << 3;
    if (bits != 0)
        goto move_cursor;

    /* Second packed check (same mask) — if clear, skip move */
    bits = 0;
    flags = i960_ld_u8(I960_WORKRAM, 0x202064, 0);
    bits |= (flags >> 5) & 1u;
    bits |= (flags >> 5) & 2u;
    bits |= (flags >> 5) & 4u;
    bits |= (i960_ld_u8(I960_WORKRAM, 0x202065, 0) & 1u) << 3;
    if (bits == 0)
        goto check_test;

move_cursor:
    cursor = i960_ld_u32(I960_WORKRAM, 0x20a2d4, 0) + 1u;
    i960_st_u32(I960_WORKRAM, 0x20a2d4, 0, cursor);
    if ((i32)cursor > 8)
        i960_st_u32(I960_WORKRAM, 0x20a2d4, 0, 0);
    cursor = i960_ld_u32(I960_WORKRAM, 0x20a2d4, 0);
    if (i960_ld_u32(I960_WORKRAM, 0x20a554, 0) == 0) {
        if (cursor == 2u || cursor == 3u)
            i960_st_u32(I960_WORKRAM, 0x20a2d4, 0, 4u);
        else if (cursor == 6u || cursor == 7u)
            i960_st_u32(I960_WORKRAM, 0x20a2d4, 0, 8u);
    }

check_test:
    flags = i960_ld_u32(I960_WORKRAM, 0x202074, 0);
    if (flags & 1u)
        goto do_test;
    flags = i960_ld_u8(I960_WORKRAM, 0x202064, 0);
    if ((flags >> 2) & 1u)
        goto do_test;
    flags = i960_ld_u8(I960_WORKRAM, 0x202064, 0);
    if ((flags >> 4) & 1u)
        goto do_test;
    g0 = 0;
    fp = fp_save;
    return 0;

do_test:
    cursor = i960_ld_u32(I960_WORKRAM, 0x20a2d4, 0);
    if (cursor > 8u)
        goto writeback;
    switch (cursor) {
    case 0:
        bump_opt(frame, 0, 1u);
        i960_st_u32(I960_WORKRAM, 0x20a2d8, 0, 1u);
        break;
    case 1:
        bump_opt(frame, 1, 2u);
        i960_st_u32(I960_WORKRAM, 0x20a2d8, 0, 1u);
        break;
    case 2:
    case 3:
        bump_opt(frame, (cursor == 2u) ? 2u : 3u, (cursor == 2u) ? 1u : 5u);
        i960_st_u32(I960_WORKRAM, 0x20a2d8, 0, 1u);
        break;
    case 4:
    case 5:
        bump_opt(frame, cursor, 3u);
        i960_st_u32(I960_WORKRAM, 0x20a2d8, 0, 1u);
        break;
    case 6:
        bump_opt(frame, 6, 3u);
        i960_st_u32(I960_WORKRAM, 0x20a2d8, 0, 1u);
        break;
    case 7:
        bump_opt(frame, 6, 2u); /* JOYPOLIS shares byte path in ROM */
        i960_st_u32(I960_WORKRAM, 0x20a2d8, 0, 1u);
        break;
    case 8:
        /* EXIT row — persist if dirty, return to main menu */
        if (i960_ld_u32(I960_WORKRAM, 0x20a2d8, 0) != 0) {
            i960_st_u32(I960_WORKRAM, 0x20a2d8, 0, 0);
            i960_call_rom(0x2fa0);
        }
        g0 = 1;
        fp = fp_save;
        return 1;
    default:
        break;
    }

writeback:
    i960_st_u32(I960_WORKRAM, 0x202020, 0, 0x3f99999au);
    i960_st_u8(I960_WORKRAM, 0x202018, 0, frame[0x130]);
    i960_st_u8(I960_WORKRAM, 0x202019, 0, frame[0x131]);
    i960_st_u8(I960_WORKRAM, 0x20201a, 0, frame[0x132]);
    i960_st_u8(I960_WORKRAM, 0x20201b, 0, frame[0x133]);
    i960_st_u8(I960_WORKRAM, 0x20201c, 0, frame[0x134]);
    i960_st_u8(I960_WORKRAM, 0x20201d, 0, frame[0x135]);
    i960_st_u8(I960_WORKRAM, 0x20201e, 0, frame[0x136]);
    g0 = 0;
    fp = fp_save;
    return 0;
}
