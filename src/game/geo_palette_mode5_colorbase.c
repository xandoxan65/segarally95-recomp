/* Palette irq mode 5 @ 0x33744 — blend boot colorbases @ 0x5FB89E → 0x01802000.
 *
 * Jump-table slot [5] @ 0x5D21BC. Uses scale @ 0x213854 and ROM LUT @ 0x5D1FD0
 * to rewrite each GEO colorbase word, then fade_step when the table is done.
 *
 * source: disasm/maincpu/maincpu_033744_120.asm */
// @rom 0x33744 +0x11c geo_palette_mode5_colorbase

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"
#include "model2_rom.h"

#define COLORBASE_COUNT_VADDR  0x005fb89cu
#define COLORBASE_SRC_VADDR    0x005fb89eu
#define COLORBASE_DEST_BASE    0x01802000u
#define COLORBASE_LUT_VADDR    0x005d1fd0u
#define COLORBASE_CHUNK        0x1f3u

void geo_palette_mode5_colorbase(u32 arg0, u32 arg1, u32 arg2)
{
    u32 cursor;
    u32 count;
    u32 scale;
    u32 copied;
    u32 src;
    u32 dest;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    cursor = i960_ld_u32(I960_WORKRAM, 0x213834, 0);
    count = (u32)model2_workram_mirror_u16(COLORBASE_COUNT_VADDR);
    scale = i960_ld_u32(I960_WORKRAM, 0x213854, 0);
    copied = 0u;
    src = COLORBASE_SRC_VADDR + (cursor << 1);
    dest = COLORBASE_DEST_BASE + (cursor << 1);

    /* @0x33770: cmpi cursor, count; bge → finish */
    if ((i32)cursor < (i32)count) {
        do {
            u16 raw = model2_workram_mirror_u16(src);
            u8 hi = (u8)(raw >> 8);
            u32 r = raw & 31u;
            u32 g = (raw >> 5) & 31u;
            u32 b = (hi >> 2) & 31u;
            u32 avg;
            u32 dr, dg, db;
            u32 out;

            /* @0x337B0–0x337E4: mean channel, then scale deltas toward mean. */
            avg = (r + g + b) / 3u;
            dr = ((scale * (avg - r)) >> 8) + r;
            dg = ((scale * (avg - g)) >> 8) + g;
            db = ((scale * (avg - b)) >> 8) + b;

            /* @0x337E8–0x33820: LUT @ 0x5D1FD0[chan*2]; pack B<<10|G<<5|R. */
            out = ((u32)model2_workram_mirror_u16(COLORBASE_LUT_VADDR + (db << 1)) << 10)
                | ((u32)model2_workram_mirror_u16(COLORBASE_LUT_VADDR + (dg << 1)) << 5)
                | (u32)model2_workram_mirror_u16(COLORBASE_LUT_VADDR + (dr << 1));
            out |= 0x8000u;
            i960_st_u16(I960_ABS, dest, 0, (u16)out);

            src += 2u;
            dest += 2u;
            copied++;
            cursor++;
            if (copied > COLORBASE_CHUNK)
                break;
        } while ((i32)cursor < (i32)count);
    }

    i960_st_u32(I960_WORKRAM, 0x213834, 0, cursor);
    /* @0x33850: if cursor >= count → clear cursor and fade_step. */
    if ((i32)cursor >= (i32)count) {
        i960_st_u32(I960_WORKRAM, 0x213834, 0, 0);
        geo_palette_fade_step(0, 0, 0);
    }
}
