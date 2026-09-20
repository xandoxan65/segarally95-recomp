/* Texture bank setup @ 0x3940; upload complete @ 0x39AC (same entry, g0<0). */
// @rom 0x3940 +0x24C texture_bank_select

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_geo.h"
#include "model2_memory.h"
#include "model2_rom.h"

#include <string.h>

#define TEX_UPLOAD_STATE     0x20227cu
#define TEX_UPLOAD_SRC       0x20224cu
#define TEX_UPLOAD_DST       0x202240u
#define TEX_UPLOAD_DST_ALT   0x202244u
#define TEX_UPLOAD_DST_SWAP  0x202248u

/*
 * Disasm 0x39AC–0x3B8C walks table @ 0x5A2880 with ldos/stq + dest swaps
 * (even/odd mip banks). Host GL get_texel matches tools/extract/textures.py:
 * main_data banks are already packed 2048×1024 sheets; a full bank copy into
 * the primary textureram is what boot/course select must leave for atlases.
 *
 * A literal ldos/stq/tex0_w lift left ~19% of each sheet ≠ main_data (mip
 * stripe placement), which dropped tree 0xF cutouts and scrambled landscape.
 * Keep setup from disasm; HLE the pump as a chunked bank copy.
 *
 * Multi-frame (disasm): inner loop @ 0x3A50 yields when 0x202004 != 0 (IRQ),
 * leaving 0x20227c busy. attract_hud_setup logo_path holds START while
 * countdown (seed 120) > 0 and 0x20227c != 0. Instant one-shot clear skipped
 * that hold. Chunk size ≈ bank/120 so busy spans the splash timeout when the
 * host has no mid-pump IRQ.
 */
#define TEX_PUMP_CURSOR      0x202250u /* disasm outer index scratch */
#define TEX_PUMP_CHUNK \
    ((TEXTURERAM_BANK_SIZE + 119u) / 120u)

static void texture_bank_pump(void)
{
    u32 state;
    u32 src_va;
    u32 dst_va;
    u32 cursor;
    u32 chunk;
    const u8 *src;
    u8 *dst;

    state = i960_ld_u32(I960_WORKRAM, TEX_UPLOAD_STATE, 0);
    if (state == 0u)
        return;

    /* @0x3A50: cmpibne 0,0x202004 → ret without clearing (IRQ yield). */
    if (i960_ld_u32(I960_WORKRAM, 0x202004, 0) != 0u)
        return;

    src_va = i960_ld_u32(I960_WORKRAM, TEX_UPLOAD_SRC, 0);
    dst_va = i960_ld_u32(I960_WORKRAM, TEX_UPLOAD_DST, 0);
    src = model2_rom_at(src_va);
    dst = model2_ram_mut(dst_va);
    if (!(src && dst
          && (dst_va == TEXTURERAM0_BASE || dst_va == TEXTURERAM1_BASE))) {
        i960_st_u32(I960_WORKRAM, TEX_UPLOAD_STATE, 0, 0);
        return;
    }

    /* First pump after setup: mark state 2 like @ 0x39C8–0x39D4. */
    if (state != 2u) {
        i960_st_u32(I960_WORKRAM, TEX_UPLOAD_STATE, 0, 2u);
        i960_st_u32(I960_WORKRAM, TEX_PUMP_CURSOR, 0, 0);
    }

    cursor = i960_ld_u32(I960_WORKRAM, TEX_PUMP_CURSOR, 0);
    if (cursor >= TEXTURERAM_BANK_SIZE) {
        i960_st_u32(I960_WORKRAM, TEX_UPLOAD_STATE, 0, 0);
        model2_geo_sheets_mark_dirty();
        return;
    }

    chunk = TEX_PUMP_CHUNK;
    if (cursor + chunk > TEXTURERAM_BANK_SIZE)
        chunk = TEXTURERAM_BANK_SIZE - cursor;
    memcpy(dst + cursor, src + cursor, chunk);
    cursor += chunk;
    i960_st_u32(I960_WORKRAM, TEX_PUMP_CURSOR, 0, cursor);

    if (cursor >= TEXTURERAM_BANK_SIZE) {
        i960_st_u32(I960_WORKRAM, TEX_UPLOAD_STATE, 0, 0);
        model2_geo_sheets_mark_dirty();
    }
}

void texture_bank_select(u32 arg0, u32 arg1, u32 arg2)
{
    u32 bank;
    u32 src_va;
    u32 dst_va;
    u32 dst_alt;

    (void)arg2;

    /* @0x3940: cmpibg 0,g0,0x39ac */
    if ((i32)arg0 < 0) {
        texture_bank_pump();
        return;
    }

    bank = arg0;
    src_va = TEXTURE_SHEET_BANK0_VADDR + (bank << 20);
    dst_va = TEXTURERAM0_BASE;
    dst_alt = TEXTURERAM0_BASE + 0x400000u;

    i960_st_u32(I960_WORKRAM, TEX_UPLOAD_SRC, 0, src_va);
    i960_st_u32(I960_WORKRAM, TEX_UPLOAD_STATE, 0, 1u);
    i960_st_u32(I960_WORKRAM, TEX_PUMP_CURSOR, 0, 0);

    if (arg1 != 0u) {
        /* @0x397C–0x3990: primary = tex1, swap flag stores 0x12000000 */
        i960_st_u32(I960_WORKRAM, TEX_UPLOAD_DST, 0, dst_alt);
        i960_st_u32(I960_WORKRAM, TEX_UPLOAD_DST_ALT, 0, dst_va);
        i960_st_u32(I960_WORKRAM, TEX_UPLOAD_DST_SWAP, 0, dst_va);
    } else {
        i960_st_u32(I960_WORKRAM, TEX_UPLOAD_DST, 0, dst_va);
        i960_st_u32(I960_WORKRAM, TEX_UPLOAD_DST_ALT, 0, dst_alt);
        /* be 0x3998 skips st to 0x202248 when g1==0 — leave swap as-is */
    }
}
