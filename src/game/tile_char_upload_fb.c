/* 1bpp source → fg/bg 4bpp rows @ 0x26C40 (body @ 0x26C44).
 *
 * Disasm: decomp/disasm/maincpu/maincpu_026c40_100.asm
 *   g13 = 7 … 0: eight ldob/st per glyph @ 0x26C68
 *   Per byte: g7 = 7 … 0 bit walk with shlo 4; rotate → g8; st g8 @ 0x26CB0
 */
// @rom 0x26c40 +0x90 tile_char_upload_fb

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_rom.h"

/* MAME i960 core @ i960.cpp case 0xd: rotl_32(src, count & 0x1f). */
static u32 i960_rotate32(u32 v, u32 count)
{
    count &= 31u;
    if (count == 0u)
        return v;
    return (v << count) | (v >> (32u - count));
}

static u32 tile_pack_fb_byte(u8 byte, u32 fg_nibble, u32 bg_nibble)
{
    u32 fg;
    u32 bg;
    u32 g5;
    u32 g6;
    i32 g7;

    fg = fg_nibble & 0xfu;
    bg = bg_nibble & 0xfu;
    g6 = (u32)byte;
    g5 = 0;

    /* @0x26C78 mov 7,g7 — eight nibbles */
    for (g7 = 7; g7 != -1; g7--) {
        g5 <<= 4;
        if (g6 & 0x80u)
            g5 += fg;
        else
            g5 += bg;
        g6 += g6;
    }

    return i960_rotate32(g5, 0u - 16u); /* @0x26CA4 subo 16,0,g8; rotate g8,g5,g8 */
}

void tile_char_upload_fb(u32 arg0, u32 arg1, u32 arg2, u32 arg3, u32 arg4)
{
    u32 src;
    u32 dest;
    i32 rows_left;
    u32 fg;
    u32 bg;
    i32 g13;
    u8 byte;

    src = arg0;
    dest = arg1;
    rows_left = (i32)arg2;
    fg = arg3;
    bg = arg4;

    if (rows_left == 0)
        return;
    rows_left--;

    for (;;) {
        if (rows_left == -1)
            break;

        /* @0x26C68 mov 7,g13 */
        for (g13 = 7; g13 != -1; g13--) {
            byte = model2_workram_mirror_u8(src);
            src++;
            i960_st_u32(I960_ABS, dest, 0, tile_pack_fb_byte(byte, fg, bg));
            dest += 4u;
        }
        rows_left--;
    }
}
