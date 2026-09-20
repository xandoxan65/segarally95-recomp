/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_03be30_278.asm */
// @rom 0x3be30 +0x278 geo_attract_course_view_bind

/*
 * Called every game_start_race_frame (@ 0x1BBA0) as call 0x3be30, post-START
 * race. Binds the still-active course view matrix to GEO (0x0B via TGP 0x05)
 * and derives the in-front/behind-course direction flag (0x2020a4) that the
 * two catalog dispatch callees below gate on.
 *
 * Args: g0=course index (0x214354), g1=0x2140c8 value, g2=accumulated
 * placement count (0x214344). g1/g2 are not read as view inputs — they only
 * feed the record-select halfword lookup (g1) and the tail calls (g1/g2).
 *
 * 1. list = COURSE_HANDLER_TABLE-style table @ 0x5dcac8[course*3] (LEA*3,
 *    matches comm_attract_course_init's 0x5dcac8 lookup). dir_va = *(list+0xc).
 *    g1<0 (defensive/never in practice): rec=0, dir_va unchanged.
 *    g1>=0: rec = sign-extended halfword at *(list+8) + (g1*3)*4 (12-byte
 *    stride record table — RE gap: no xref yet on the record layout itself,
 *    only this read site); dir_va += g1*8.
 * 2. TGP 0x20 (push/save current matrix) then immediately 0x26 (12-float
 *    readback of that same still-current matrix) — snapshots R+T into local
 *    fp+0x50..0x7c, then the low 3 words (T) are overwritten with 0 before
 *    use (0x27/point test always runs at the record's local origin).
 * 3. TGP 0x22 loads R (9 floats, unchanged) + T=(0,0,0) as the working
 *    matrix, 0x2c transforms (dir_va[0], 0, dir_va[4]) through it, then 0x21
 *    restores the matrix saved by 0x20 — the R/T=0 matrix is scratch only,
 *    never bound to GEO.
 * 4. Sign of the 3rd transformed component selects 0x2020a4: <=0 → 1,
 *    >0 → 0 (course_prg_mode's mode_2 gate reads this same cell).
 * 5. GEO 0x800160=0, PRG far-Z push (0x48c35000u), then bind #5: hand the
 *    current GEO write cursor (0x802008) to the *(0x20a290) slot, TGP 0x05
 *    (emits the *restored*, i.e. caller's course-view, matrix to GEO),
 *    advance 0x20a290, and stash the next cursor (+0x34) to 0x801008. This
 *    is the "course view matrix" bind referenced by the caller comment.
 * 6. Tail calls are already-lifted siblings — call directly, not via
 *    i960_call_rom: geo_attract_course_prg_mode(course, rec, count) then
 *    geo_attract_course_catalog_span(&courseTable[course], rec, sign) with
 *    globals g3=count, g4=1 (that callee reads budget/flag from g3/g4).
 */

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_memory.h"
#include "model2_rom.h"

#include "lift_syms.h"

#include <stdio.h>
#include <string.h>

void geo_attract_course_view_bind(u32 arg0, u32 arg1, u32 arg2)
{
    u32 course = arg0;
    u32 sel = arg1;
    u32 count = arg2;
    u32 list3 = course + (course << 1);
    u32 list = i960_ld_u32(I960_WORKRAM, 0x5dcac8u, list3 << 2);
    u32 dir_va;
    i32 rec;
    float r_mtx[9];
    float sign_val;
    u32 slot, wr_ptr;
    int i;
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    u8 frame[0x80];

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    sp = sp + 0x40;

    /* Table @ 0x5dcac8 yields main_data list heads (e.g. 0x02003e00). */
    if (list == 0u) {
        fp = fp_save;
        sp = sp_save;
        return;
    }
    dir_va = i960_ld_u32(I960_WORKRAM, list, 0xc);

    if ((i32)sel < 0) {
        rec = 0;
    } else {
        u32 rec_base = i960_ld_u32(I960_WORKRAM, list, 0x8);
        u32 sel3 = sel + (sel << 1);
        u16 half = (u16)i960_ld_u16(I960_WORKRAM, rec_base, sel3 << 2);

        dir_va = dir_va + sel * 8u;
        rec = (half & 0x8000u) ? (i32)(half | 0xffff0000u) : (i32)half;
    }

    /* TGP 0x20: save the caller's current (course-view) matrix. */
    i960_mmio_write_u32(0x884000, 0x10002020u);

    {
        u32 dword0 = i960_ld_u32(I960_WORKRAM, dir_va, 0);
        u32 dword1 = i960_ld_u32(I960_WORKRAM, dir_va, 4);

        i960_st_u64(I960_WORKRAM, 0x214360u, 0, (u64)dword0);
        i960_st_u32(I960_WORKRAM, 0x214368u, 0, dword1);

        /* TGP 0x26: 12-float readback of the matrix just saved (unchanged). */
        i960_mmio_write_u32(0x884000, 0x13002626u);
        for (i = 0; i < 8; i++)
            r_mtx[i] = (float)i960_u32_to_f64(i960_ld_u32(I960_MMIO, 0x884000, 0));
        r_mtx[8] = (float)i960_u32_to_f64(i960_ld_u32(I960_MMIO, 0x884000, 0));
        /* Last 3 readback words (T) are discarded — T forced to 0 below. */
        (void)i960_ld_u32(I960_MMIO, 0x884000, 0);
        (void)i960_ld_u32(I960_MMIO, 0x884000, 0);
        (void)i960_ld_u32(I960_MMIO, 0x884000, 0);

        /* TGP 0x22: load R (9 floats) + T=(0,0,0) as the working matrix. */
        i960_mmio_write_u32(0x884000, 0x11002222u);
        for (i = 0; i < 9; i++)
            i960_mmio_write_u32(0x884000, i960_f64_to_u32((double)r_mtx[i]));
        i960_mmio_write_u32(0x884000, 0u);
        i960_mmio_write_u32(0x884000, 0u);
        i960_mmio_write_u32(0x884000, 0u);

        /* TGP 0x2c: transform (dword0, 0, dword1) through that matrix. */
        i960_mmio_write_u32(0x884000, 0x16002c2cu);
        i960_mmio_write_u32(0x884000, dword0);
        i960_mmio_write_u32(0x884000, 0u);
        i960_mmio_write_u32(0x884000, dword1);
        (void)i960_ld_u32(I960_MMIO, 0x884000, 0);
        (void)i960_ld_u32(I960_MMIO, 0x884000, 0);
        sign_val = (float)i960_u32_to_f64(i960_ld_u32(I960_MMIO, 0x884000, 0));
    }

    /* TGP 0x21: restore the matrix saved by 0x20 (course view, untouched). */
    i960_mmio_write_u32(0x884000, 0x10802121u);

    i960_st_u32(I960_WORKRAM, 0x2020a4u, 0, (sign_val <= 0.0f) ? 1u : 0u);

    i960_mmio_write_u32(0x800160u, 0u);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x48c35000u); /* far-Z push */

    /*
     * Same GEO bind as attract: TGP 0x05 → GEO 0x0B from the current matrix
     * (desert/chase cam built it above), then catalog_span / prg_mode. Disasm
     * has no identity gate — host skip+clear_draw here blocked the shared
     * draw path after START.
     */

    /* Bind #5: current GEO write cursor → *(0x20a290) slot, TGP 0x05, advance. */
    slot = i960_ld_u32(I960_WORKRAM, 0x20a290u, 0);
    wr_ptr = i960_mmio_read_u32(0x802008u);
    if (slot != 0u && (slot & 3u) == 0u
        && slot >= WORKRAM_BASE && slot < WORKRAM_BASE + WORKRAM_SIZE)
        i960_st_u32(I960_WORKRAM, slot, 0, wr_ptr);
    i960_mmio_write_u32(0x884000, 0x02800505u);
    i960_st_u32(I960_WORKRAM, 0x20a290u, 0, slot + 4u);
    i960_mmio_write_u32(0x801008u, wr_ptr + 0x34u);

    /* Tail: already-lifted siblings, called directly (not i960_call_rom —
     * both 0x3b9b0/0x3b7c0 already have C symbols; see lift_syms.h). */
    geo_attract_course_prg_mode(course, (u32)rec, count);

    g3 = count;
    g4 = 1u;
    geo_attract_course_catalog_span(
        0x5dca00u + (list3 << 2), (u32)rec,
        (sign_val <= 0.0f) ? (u32)-1 : 1u);

    fp = fp_save;
    sp = sp_save;
}
