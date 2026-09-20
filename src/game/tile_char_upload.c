/* 1bpp source → 4bpp tile rows @ 0x26B90 (body @ 0x26B94).
 *
 * Disasm: decomp/disasm/maincpu/maincpu_026b90_100.asm
 *   g2 = glyph count (boot @ 0x266C4 setbit 7,0 → 128)
 *   g7 = 7 … 0 inclusive → 8 rows/glyph, one ldob per row @ 0x26BBC
 *   Inner g6 = 1 then 0: two bit-quartets, rotate g5 by 16 between passes @ 0x26C00
 */
// @rom 0x26b90 +0xa0 tile_char_upload

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

static u32 tile_pack_fg_byte(u8 byte, u32 fg_nibble)
{
    u32 fg;
    u32 r5;
    u32 r4;
    u32 g13;
    u32 g4;
    u32 g5;
    u32 g6;
    u32 r6;

    fg = fg_nibble & 0xfu;
    r5 = fg << 12;
    r4 = fg << 8;
    g13 = fg << 4;
    g4 = (u32)byte;
    g5 = 0;
    g6 = 1;
    r6 = (u32)-1;

    for (;;) {
        /* @0x26BCC–0x26BF4: four bbc; addo g4 only after the first three (not after the 4th bbc). */
        if (g4 & 0x80u)
            g5 += r5;
        g4 += g4;
        if (g4 & 0x80u)
            g5 += r4;
        g4 += g4;
        if (g4 & 0x80u)
            g5 += g13;
        g4 += g4;
        if (g4 & 0x80u)
            g5 += fg;

        g6 -= 1u;
        /* @0x26BFC subo 16,0,g8 → g8 = -16; rotate uses count & 0x1f → 16 */
        g5 = i960_rotate32(g5, 0u - 16u);

        if (g6 == r6)
            break;
        g4 += g4; /* @0x26C0C before bne 0x26bcc */
    }
    return g5;
}

void tile_char_upload(u32 arg0, u32 arg1, u32 arg2, u32 arg3, u32 arg4)
{
    u32 src;
    u32 dest;
    i32 rows_left;
    u32 fg;
    i32 g7;
    u8 byte;

    (void)arg4;
    src = arg0;
    dest = arg1;
    rows_left = (i32)arg2;
    fg = arg3;

    if (rows_left == 0)
        return;
    rows_left--;

    for (;;) {
        if (rows_left == -1)
            break;

        /* @0x26BB8 mov 7,g7 — eight rows (7 down to 0) */
        for (g7 = 7; g7 != -1; g7--) {
            byte = model2_workram_mirror_u8(src);
            src++;
            i960_st_u32(I960_ABS, dest, 0, tile_pack_fg_byte(byte, fg));
            dest += 4u;
        }
        rows_left--;
    }
}
