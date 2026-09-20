/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_029eb0_190.asm */
// @rom 0x29eb0 +0x190 catalog_draw_setup

#include "i960_lift.h"
#include "i960_fp.h"
#include "model2_rom.h"
#include "i960_mem.h"
#include "model2_memory.h"
#include "cgm_format.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

extern u32 cgm_dispatch_record_build(u32 handler_rom);

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret  callee r11,r12,r14,r4 */
/* abi: u32 arg0=g0, u32 arg1=g1, u32 arg2=g2 → u32 g0 */

extern void tile_cursor_seed(u32 x, u32 y);
extern u32 draw_scene_dispatch(u32 arg0, u32 arg1, u32 arg2);
extern void cgm_leading_colorbase(void * arg0, void * arg1, u32 arg2);
extern void cgm_record_dispatch(u32 arg0, u32 arg1);
extern void * libc_scanf_setup(void * arg0, void * arg1, u32 arg2);
extern void libc_printf(const char * fmt, u32 arg1, u32 arg2);

/* Static CGM blocks store ``CGM `` @ +0; runtime patches word0 to block vaddr. */
#define CGM_BLOCK_HEAD_SIG  0x204d4743u

/*
 * @0x29F88: lda 0x20b950[idx*8],g4 — effective address, not a load.
 * Treating the first node word as a pointer rebased later glyph stores when
 * the colorbase word looked like a workram address, so INSERT COIN lost
 * glyphs 2/3 (COIN) while glyph 0 (INSERT) still drew.
 */
static u32 batch_node_vaddr(u32 batch_idx)
{
    return 0x20b950u + batch_idx * 8u;
}

static u16 rom_u16_at(u32 vaddr)
{
    if (vaddr >= WORKRAM_BASE && vaddr < WORKRAM_BASE + WORKRAM_SIZE)
        return i960_ld_u16(I960_WORKRAM, vaddr, 0);
    const u8 *p = model2_rom_at(vaddr);

    if (!p)
        return 0;
    return (u16)p[0] | ((u16)p[1] << 8);
}

static u8 rom_u8_at(u32 vaddr)
{
    if (vaddr >= WORKRAM_BASE && vaddr < WORKRAM_BASE + WORKRAM_SIZE)
        return (u8)i960_ld_u8(I960_WORKRAM, vaddr, 0);
    {
        const u8 *p = model2_rom_at(vaddr);

        return p ? p[0] : 0;
    }
}

/* @0x29EDC: 8-byte compare vs template @ 0x5C8E60 ("CGM 1.0 "). */
static int cgm_prefix_match(u32 catalog_vaddr)
{
    static const u8 tmpl[8] = { 'C', 'G', 'M', ' ', '1', '.', '0', ' ' };
    u32 i;

    for (i = 0; i < 8u; i++) {
        if (rom_u8_at(catalog_vaddr + i) != tmpl[i])
            return -1;
    }
    return 0;
}

u32 catalog_draw_setup(u32 arg0, u32 arg1, u32 arg2)
{
    /* Credit catalogs (INSERT COIN @ 0x288B01C, etc.) are real CGM 1.0 headers
     * in main_data @ MAIN_DATA_A — not mid-stream pointers. */
    u32 catalog_vaddr = (u32)arg2;
    u32 entry_layer = (u32)g4;
    /* @0x29ED4 0x40(fp) — stream root dword, not host frame pointer. */
    u32 stream_vaddr;

    g0 = (uintptr_t)arg0;
    g2 = catalog_vaddr;

    sp = sp + 16;
    /* @0x29EB4: movl g0,r12 — saves x:y pair in r12:r13. */
    r12 = (uintptr_t)arg0;
    r13 = (uintptr_t)arg1;
    r11 = entry_layer;
    g4 = i960_ld_u32(I960_WORKRAM, 0x20c954, 0);
    r4 = catalog_vaddr;
    r14 = g3;
    g0 = catalog_vaddr;
    g6 = 0x1ff;
    stream_vaddr = catalog_vaddr;
    cgm_catalog_stage(catalog_vaddr);
    if (g4 > 0x1ffu)
        goto L_0002a004;
    /* Ensure template is present in workram (ROM mirror @ 0x5C8E60). */
    if (i960_ld_u8(I960_WORKRAM, CGM_HEADER_TEMPLATE, 0) == 0) {
        static const u8 tmpl[8] = { 'C', 'G', 'M', ' ', '1', '.', '0', ' ' };
        u32 i;

        for (i = 0; i < 8u; i++)
            i960_st_u8(I960_WORKRAM, CGM_HEADER_TEMPLATE, i, tmpl[i]);
    }
    if (cgm_prefix_match(catalog_vaddr) != 0)
        goto L_0002a01c;
    /* @0x29EF0: r5=&fp[0x40]; @0x29EF4 loads the catalog vaddr stored there. */
    g4 = stream_vaddr + 8u;
    g0 = i960_ld_u32(I960_WORKRAM, 0x20c950, 0);
    r9 = rom_u16_at(g4);
    g4 = g4 + 2u;
    stream_vaddr = g4;
    g6 = 0xffff;
    r8 = g6 & r9;
    g6 = g6 & ~(1u << 7);
    g4 = r8 + g0;
    if (g4 > g6)
        goto L_0002a004;
    r10 = g0 << 7;
    /* @0x29F2C passes the address of the saved stream pointer. */
    g1 = (uintptr_t)&stream_vaddr;
    g2 = r9;
    cgm_leading_colorbase((void *)(uintptr_t)g0, (void *)(uintptr_t)g1, g2);
    g5 = stream_vaddr;
    g4 = i960_ld_u32(I960_WORKRAM, 0x20c950, 0);
    r6 = 0;
    {
        u32 return_batch = i960_ld_u32(I960_WORKRAM, 0x20c954, 0);
        u32 draw_mode = entry_layer & 7u;
        u32 batch_count;
        u16 saved_r10 = (u16)r10;

        r7 = return_batch;
        batch_count = rom_u16_at((u32)g5);
        g4 = (u32)g4 + r8;
        g5 = (u32)g5 + 2u;
        i960_st_u32(I960_WORKRAM, 0x20c950, 0, (u32)g4);
        stream_vaddr = (u32)g5;
        if (getenv("I960_TRACE_SPLASH")) {
            fprintf(stderr,
                    "lift: catalog batch_count=%#x stream=%#x slot=%u\n",
                    (unsigned)batch_count, (unsigned)stream_vaddr,
                    (unsigned)g4);
        }

        if ((unsigned char)batch_count == 0)
            goto L_00029fd0;
        g6 = 0x1ff;
        r15 = 0x1ff;
        if (return_batch > g6)
            goto L_00029fd0;

        L_00029f7c:
            g4 = i960_ld_u32(I960_WORKRAM, 0x20c954, 0);
            {
                u32 node = batch_node_vaddr((u32)g4);

                i960_st_u16(I960_WORKRAM, node, 0, saved_r10);
                i960_st_u8(I960_WORKRAM, node, 2, (u8)g14);
                i960_st_u8(I960_WORKRAM, node, 3, (u8)g14);
                /* @0x29F9C: node+4 = fp+0x40 stream record (draw_scene @ 0x29CC4). */
                i960_st_u32(I960_WORKRAM, node, 4, stream_vaddr);
            }
            /* @0x29F84: dispatch record uses stream cursor in g0, not catalog. */
            i960_st_u32(I960_WORKRAM, CGM_STAGE_BASE, 0, stream_vaddr);
            i960_st_u32(I960_WORKRAM, CGM_STAGE_BASE, 4, 0);
            g0 = stream_vaddr;
            g4 = entry_layer;
            cgm_record_dispatch((u32)g0, (u32)g2);
            {
                u32 next_stream = (u32)g0;

                /*
                 * @0x29FC4: st g0,0x40(fp) — next record EA from record_dispatch
                 * lda. Reject prepare_batch's sentinel -1 / null so a broken
                 * handler cannot freeze the cursor on glyph 0.
                 */
                if (next_stream == 0u || next_stream == (u32)(0u - 1u))
                    next_stream = i960_ld_u32(I960_WORKRAM, CGM_STAGE_BASE, 0);
                if (next_stream != 0u && next_stream != (u32)(0u - 1u)
                    && (next_stream >= MAIN_DATA_A
                        || (next_stream >= CGM_STAGE_DATA
                            && next_stream < CGM_STAGE_DATA + 0x2000u)))
                    stream_vaddr = next_stream;
            }
            g4 = i960_ld_u32(I960_WORKRAM, 0x20c954, 0);
            batch_count--;
            /*
             * @0x29FB0: ``lda 0x1(r6),r6`` — plain r6+1 (effective-address-as-add
             * idiom), NOT a linked-list dereference. r6 is the per-batch record
             * count and is stored as the node+3 "span" byte below (@0x29FE0).
             * Mislifting this as a memory-walk left r6 permanently 0, so
             * cgm_1111_flush's ``span == 0`` early-out (cgm_1111_flush.c) never
             * released batch-table slots: the shared 512-slot node table
             * (0x20B950 / 0x20C954) only ever grew across every HUD/credit/
             * splash catalog draw for the life of the process, and once it hit
             * its 0x1FF cap every later catalog_draw_setup call (including the
             * attract splash rebinds in comm_attract_inner_2/4/6) failed
             * outright — tile_map_banks_clear() had already blanked the tile
             * map, so the splash held solid black forever after.
             */
            r6 = r6 + 1u;
            g4 = g4 + 1;
            i960_st_u32(I960_WORKRAM, 0x20c954, 0, (u32)g4);
            if (stream_vaddr < MAIN_DATA_A
                && stream_vaddr < CGM_STAGE_DATA
                && batch_count != 0u)
                goto L_00029fd0;
            if (batch_count == 0)
                goto L_00029fd0;
            if (g4 <= r15)
                goto L_00029f7c;

        L_00029fd0:
            {
                u32 node = batch_node_vaddr(return_batch);

                i960_st_u8(I960_WORKRAM, node, 2, (u8)r9);
                i960_st_u8(I960_WORKRAM, node, 3, (u8)r6);
            }
            if (3u < draw_mode)
                goto L_00029ffc;
            /* @0x29FE8: movl r12,g0 — restore caller x:y, not colorbase. */
            g0 = r12;
            g1 = r13;
            g2 = return_batch;
            g3 = r14;
            g4 = entry_layer;
            draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

        L_00029ffc:
            g0 = return_batch;
            return (u32)g0;
    }

    L_0002a004:
        g0 = 1;
        g1 = 1;
        tile_cursor_seed((u32)g0, (u32)g1);
    g0 = (uintptr_t)(model2_workram + 0xc8e70);
    goto L_0002a030;

    L_0002a01c:
        g0 = 1;
        g1 = 1;
        tile_cursor_seed((u32)g0, (u32)g1);
    g0 = (uintptr_t)(model2_workram + 0xc8e90);

    L_0002a030:
        g1 = r4;
        libc_printf((const char *)(uintptr_t)g0, g1, g2);
    g0 = 0u - 1u;
    return (u32)g0;
}
