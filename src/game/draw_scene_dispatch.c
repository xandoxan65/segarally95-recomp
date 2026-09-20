/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_029c10_128.asm */
// @rom 0x29c10 +0x128 draw_scene_dispatch

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

#define DRAW_FMT_CELL_VADDR  0x005C8BF0u
#define L2_TILE_BASE         0x01000000u
#define L1_TILE_BASE         0x01004000u

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

u32 draw_scene_dispatch(u32 arg0, u32 arg1, u32 arg2)
{
    u32 batch_idx = arg2;
    u32 batch_limit;
    u32 batch_sum;
    u32 mode_bits;
    u32 record;
    u32 fmt_ptr;
    u32 stream;

    if (getenv("I960_TRACE_SPLASH")) {
        fprintf(stderr,
                "lift: draw_scene enter batch=%u slot=%u g3=%u g4=%#x limit=%u\n",
                (unsigned)batch_idx, (unsigned)arg1, (unsigned)g3, (unsigned)g4,
                (unsigned)i960_ld_u32(I960_WORKRAM, 0x20c954, 0));
    }
    r4 = batch_idx;
    if ((signed)batch_idx < 0)
        goto L_00029c34;
    batch_limit = i960_ld_u32(I960_WORKRAM, 0x20c954, 0);
    batch_sum = batch_idx + (u32)g3;
    if (batch_sum >= batch_limit)
        goto L_00029c34;
    mode_bits = (u32)g4 & 7u;
    if (mode_bits <= 3u)
        goto L_00029c74;

    L_00029c34:
        g0 = 1;
        g1 = 1;
        g0 = 1;
        g1 = 1;
        tile_cursor_seed((u32)g0, (u32)g1);
        g0 = 8;
        scene_view_scale((u32)g0, 0, 0);
        stream = scene_stream_lookup(batch_idx, 0);
        fmt_ptr = model2_workram_mirror_u32(DRAW_FMT_CELL_VADDR);
        if (fmt_ptr == 0)
            fmt_ptr = stream;
        g2 = (uintptr_t)r5;
        /* libc_printf itself forces scratch/fp=0; keep call site explicit. */
        libc_printf((const char *)(uintptr_t)fmt_ptr, stream, (u32)g2);
        g0 = 0u - 1u;
        return (u32)g0;

    L_00029c74:
        if (!(g4 & 1))
            goto L_00029c90;
        /* @0x29C80: mov 0,g13 — L1 starts with a clean colorbase. */
        g6 = (((uintptr_t)arg1 << 6) + (uintptr_t)arg0) * 2u + L1_TILE_BASE;
        g13 = 0;
        goto L_00029ca4;

    L_00029c90:
        /* @0x29C98: setbit 15,0,g13 — L2 priority only; do not keep prior g13. */
        g6 = (((uintptr_t)arg1 << 6) + (uintptr_t)arg0) * 2u + L2_TILE_BASE;
        g13 = 1u << 15;

    L_00029ca4:
        if ((g4 >> 1) & 1)
            g6 = g6 + 0x2000u;
        if ((g4 >> 3) & 1)
            g13 = g13 | (1u << 14);
        r4 = batch_idx + (u32)g3;
        {
            u32 node = (u32)(r4 * 8u + 0x20b950u);

            record = i960_ld_u32(I960_WORKRAM, node, 4);
            g4 = i960_ld_u16(I960_WORKRAM, node, 0);
        }
        if (record == 0)
            goto L_00029d34;
        g2 = record + 18u;
        g5 = (u32)(i32)(signed short)record_u16(g2);
        if (getenv("I960_TRACE_SPLASH")) {
            fprintf(stderr,
                    "lift: draw_scene batch=%u record=%#x rows=%d node_flags=%#x\n",
                    (unsigned)batch_idx, (unsigned)record, (int)(signed short)g5,
                    (unsigned)g4);
        }
        g1 = 0;
        /* @0x29CD8: or g13,g4,g13 — node colorbase into fresh g13. */
        g13 = (u32)g4 | (u32)g13;
        /* @0x29CDC: lda 0x14(g0),g7 — pixmap u16 stream at record+0x14. */
        g7 = record + 0x14u;
        if ((i32)g5 <= 0)
            goto L_00029d34;
        g0 = record + 16u;
        /* @0x29CE8: setbit 6,0,r6 — fixed tile-map row stride. */
        r6 = 1u << 6;

    L_00029cec:
        g4 = (u32)(i32)(signed short)record_u16((u32)g0);
        g5 = 0;
        if ((i32)g4 <= 0)
            goto L_00029d18;
        do {
            u16 pixel = record_u16((u32)g7);

            /* @0x29CFC: addo g13,g4,g4 — colorbase (+ L2 bit15 from g13). */
            pixel = (u16)((u16)pixel + (u16)g13);
            tile_store((u32)g6, 0, pixel);
            g4 = (u32)(i32)(signed short)record_u16((u32)g0);
            g7 = g7 + 2u;
            g6 = g6 + 2u;
            g5 = g5 + 1u;
        } while ((i32)g5 < (i32)g4);

    L_00029d18:
        g4 = (u32)(i32)(signed short)record_u16((u32)g0);
        g5 = (u32)(i32)(signed short)record_u16(g2);
        g1 = g1 + 1u;
        g4 = (u32)r6 - (u32)g4;
        g6 = g6 + ((u32)g4 << 1);
        if ((i32)g1 < (i32)g5)
            goto L_00029cec;

    L_00029d34:
        g0 = r4;
        return (u32)g0;
}
