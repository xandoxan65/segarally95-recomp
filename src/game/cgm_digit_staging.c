/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_02a2e0_128.asm */
// @rom 0x2a2e0 +0x128 cgm_digit_staging

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_memory.h"

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret */
/* abi: u32 arg0=g0, u32 arg1=g1, u32 arg2=g2 → u32 g0 */

#define L2_TILE_BASE  0x01000000u
#define L1_TILE_BASE  0x01004000u

static u8 stream_byte(u32 cursor)
{
    if (cursor >= WORKRAM_BASE && cursor < WORKRAM_BASE + WORKRAM_SIZE)
        return i960_ld_u8(I960_WORKRAM, cursor, 0);
    return *(const u8 *)(uintptr_t)cursor;
}

static u32 stream_word(u32 cursor)
{
    u32 b0 = stream_byte(cursor);
    u32 b1 = stream_byte(cursor + 1u);
    u32 b2 = stream_byte(cursor + 2u);
    u32 b3 = stream_byte(cursor + 3u);

    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

u32 cgm_digit_staging(u32 arg0, u32 arg1, u32 arg2)
{
    u32 map_offset;
    u16 l1_word;
    u16 l2_word;
    u32 tile_addr;
    u32 pen;

    g0 = (uintptr_t)arg0;
    g1 = (uintptr_t)arg1;
    g2 = (uintptr_t)arg2;

    r6 = 31 + 30;
    if ((signed char)(u8)g0 > (signed char)r6)
        goto L_0002a2f8;
    g5 = (u32)g0 << 3;
    i960_st_u32(I960_WORKRAM, 0x20c95c, 0, (u32)g5);
    goto L_0002a300;

    L_0002a2f8:
        i960_st_u32(I960_WORKRAM, 0x20c95c, 0, (u32)g14);

    L_0002a300:
        r6 = 31 + 16;
        if ((signed char)(u8)g1 > (signed char)r6)
            goto L_0002a318;
        g5 = (u32)g1 << 3;
        i960_st_u32(I960_WORKRAM, 0x20c960, 0, (u32)g5);
        goto L_0002a320;

    L_0002a318:
        i960_st_u32(I960_WORKRAM, 0x20c960, 0, (u32)g14);

    L_0002a320:
        r6 = 31 + 30;
        if ((signed char)(u8)g2 > (signed char)r6)
            goto L_0002a33c;
        g5 = (uintptr_t)g2 * 8u + 0x7u;
        i960_st_u32(I960_WORKRAM, 0x20c964, 0, (u32)g5);
        goto L_0002a348;

    L_0002a33c:
        r6 = 0x1ef;
        i960_st_u32(I960_WORKRAM, 0x20c964, 0, (u32)r6);

    L_0002a348:
        r6 = 31 + 16;
        if ((signed char)(u8)g3 > (signed char)r6)
            goto L_0002a364;
        g5 = (uintptr_t)g3 * 8u + 0x7u;
        i960_st_u32(I960_WORKRAM, 0x20c968, 0, (u32)g5);
        goto L_0002a370;

    L_0002a364:
        r6 = 0x17f;
        i960_st_u32(I960_WORKRAM, 0x20c968, 0, (u32)r6);

    L_0002a370:
        g13 = i960_ld_u16(I960_WORKRAM, 0x20c958, 0);
        if ((signed char)(u8)g1 > (signed char)(u8)g3)
            goto L_0002a404;
        map_offset = (u32)g0 << 1;

    L_0002a390:
        l2_word = i960_ld_u16(I960_ABS, L2_TILE_BASE, map_offset);
        l1_word = i960_ld_u16(I960_ABS, L1_TILE_BASE, map_offset);
        r4 = l1_word;
        if ((unsigned char)(u8)r4 == 0)
            goto L_0002a39c;
        if ((unsigned char)(u8)r4 == 1)
            goto L_0002a3b8;
        goto L_0002a3d0;

    L_0002a39c:
        g6 = ((u32)g1 << 7) + l2_word;
        g5 = ((u32)g1 << 6) + (u32)g13;
        pen = i960_ld_u16(I960_ABS, g5 + 0x8000u, (u32)g0 * 2u);
        goto L_0002a3d8;

    L_0002a3b8:
        g6 = ((u32)g1 << 7) + l1_word;
        g5 = ((u32)g1 << 6) + (u32)g13 + (u32)g0;
        pen = g5;
        goto L_0002a3d8;

    L_0002a3d0:
        g0 = 1;
        return (u32)g0;

    L_0002a3d8:
        tile_addr = (u32)g6;
        g7 = g0;
        if ((u32)g0 > (u32)g2)
            goto L_0002a3fc;
        do {
            g7 = g7 + 1u;
            i960_st_u16(I960_ABS, tile_addr, 0, (u16)pen);
            if ((unsigned char)(u8)r4 == 0)
                pen = i960_ld_u16(I960_ABS, g5 + 0x8000u, (u32)g7 * 2u);
            else
                pen = pen + 1u;
            tile_addr = tile_addr + 2u;
        } while ((u32)g7 <= (u32)g2);

    L_0002a3fc:
        g1 = g1 + 1u;
        if ((signed char)(u8)g1 <= (signed char)(u8)g3)
            goto L_0002a390;

    L_0002a404:
        return (u32)g0;
}
