/* Scene glyph erase sibling of draw_scene_dispatch @ 0x29d60.
 * Clears tile-map cells with blank (0) instead of blitting pixmap rows.
 * source: disasm/maincpu/maincpu_029d60_128.asm */
// @rom 0x29d60 +0x128 erase_scene_dispatch

#include "i960_lift.h"
#include "i960_fp.h"
#include "model2_memory.h"
#include "model2_rom.h"
#include "i960_mem.h"

#include <stdio.h>
#include <stdlib.h>

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret  callee r4 */
/* abi: u32 arg0=g0, u32 arg1=g1, u32 arg2=g2 → u32 g0 */

extern void tile_cursor_seed(u32 x, u32 y);
extern void scene_view_scale(u32 arg0, u32 arg1, u32 arg2);
extern u32 scene_stream_lookup(u32 index, u32 base);
extern void libc_printf(const char * fmt, u32 arg1, u32 arg2);

#define ERASE_FMT_CELL_VADDR  0x005C8D40u
#define L2_TILE_BASE          0x01000000u
#define L1_TILE_BASE          0x01004000u

static u16 record_u16(u32 vaddr)
{
    if (vaddr >= WORKRAM_BASE && vaddr < WORKRAM_BASE + WORKRAM_SIZE)
        return i960_ld_u16(I960_WORKRAM, vaddr, 0);
    {
        const u8 *p = model2_rom_at(vaddr);

        if (!p)
            return 0;
        return (u16)p[0] | ((u16)p[1] << 8);
    }
}

static void tile_store(u32 map_base, u32 offset, u16 value)
{
    i960_st_u16(I960_ABS, map_base, offset, value);
}

u32 erase_scene_dispatch(u32 arg0, u32 arg1, u32 arg2)
{
    u32 batch_idx = arg2;
    u32 batch_limit;
    u32 batch_sum;
    u32 mode_bits;
    u32 record;
    u32 fmt_ptr;
    u32 stream;
    u32 row_stride;

    r4 = batch_idx;
    if ((signed)batch_idx < 0)
        goto L_00029d84;
    batch_limit = i960_ld_u32(I960_WORKRAM, 0x20c954, 0);
    batch_sum = batch_idx + (u32)g3;
    if (batch_sum >= batch_limit)
        goto L_00029d84;
    mode_bits = (u32)g4 & 7u;
    if (mode_bits <= 3u)
        goto L_00029dc4;

L_00029d84:
    g0 = 1;
    g1 = 1;
    tile_cursor_seed((u32)g0, (u32)g1);
    g0 = 8;
    scene_view_scale((u32)g0, 0, 0);
    stream = scene_stream_lookup(batch_idx, 0);
    fmt_ptr = model2_workram_mirror_u32(ERASE_FMT_CELL_VADDR);
    if (fmt_ptr == 0)
        fmt_ptr = stream;
    g2 = (uintptr_t)r5;
    libc_printf((const char *)(uintptr_t)fmt_ptr, stream, (u32)g2);
    g0 = 0u - 1u;
    return (u32)g0;

L_00029dc4:
    if (!(g4 & 1))
        goto L_00029ddc;
    g6 = (((uintptr_t)arg1 << 6) + (uintptr_t)arg0) * 2u + L1_TILE_BASE;
    goto L_00029dec;

L_00029ddc:
    g6 = (((uintptr_t)arg1 << 6) + (uintptr_t)arg0) * 2u + L2_TILE_BASE;

L_00029dec:
    if ((g4 >> 1) & 1)
        g6 = g6 + 0x2000u;
    r4 = batch_idx + (u32)g3;
    {
        u32 node = (u32)(r4 * 8u + 0x20b950u);

        record = i960_ld_u32(I960_WORKRAM, node, 4);
    }
    if (record == 0)
        goto L_00029e58;
    g1 = record + 18u;
    g4 = (u32)(i32)(signed short)record_u16((u32)g1);
    g0 = 0;
    if ((i32)g4 <= 0)
        goto L_00029e58;
    g7 = record + 16u;
    /* @0x29E18: setbit 6,0,g13 — fixed tile-map row stride. */
    row_stride = 1u << 6;

L_00029e1c:
    g4 = (u32)(i32)(signed short)record_u16((u32)g7);
    g5 = 0;
    if ((i32)g4 <= 0)
        goto L_00029e3c;
    do {
        /* @0x29E28: stos g14,(g6) — blank cell (callers keep g14==0). */
        tile_store((u32)g6, 0, 0);
        g4 = (u32)(i32)(signed short)record_u16((u32)g7);
        g6 = g6 + 2u;
        g5 = g5 + 1u;
    } while ((i32)g5 < (i32)g4);

L_00029e3c:
    g4 = (u32)(i32)(signed short)record_u16((u32)g7);
    g5 = (u32)(i32)(signed short)record_u16((u32)g1);
    g0 = g0 + 1u;
    g4 = row_stride - (u32)g4;
    g6 = g6 + ((u32)g4 << 1);
    if ((i32)g0 < (i32)g5)
        goto L_00029e1c;

L_00029e58:
    g0 = r4;
    return (u32)g0;
}
