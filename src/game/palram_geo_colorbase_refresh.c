/* GEO colorbase table refresh @ 0x333C8 — irq mode > 6 path. */
// @rom 0x333c8 +0x8c palram_geo_colorbase_refresh

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_rom.h"

#define COLORBASE_COUNT_VADDR  0x005fb89cu
#define COLORBASE_SRC_VADDR    0x005fb89eu
#define COLORBASE_DEST_BASE    0x01802000u
#define COLORBASE_CHUNK        0x1f3u

/*
 * Disasm @ 0x333C8–0x33450 (from geo_palette_irq_frame when 0x213850 > 6):
 * ldos count @ 0x5FB89C; copy u16s from 0x5FB89E[cursor] → 0x01802000[cursor]
 * up to 0x1F3 per call; when cursor >= count, clear mode @ 0x213850.
 * Same ROM table as boot_palette_splash_upload @ 0x267B0.
 */
void palram_geo_colorbase_refresh(u32 arg0, u32 arg1, u32 arg2)
{
    u32 count;
    u32 cursor;
    u32 copied;
    u32 src;
    u32 dest;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    count = (u32)model2_workram_mirror_u16(COLORBASE_COUNT_VADDR);
    cursor = i960_ld_u32(I960_WORKRAM, 0x213834, 0);
    copied = 0u;
    src = COLORBASE_SRC_VADDR + (cursor << 1);
    dest = COLORBASE_DEST_BASE + (cursor << 1);

    /* @0x333F0: cmpi cursor, count; bge → done chunk */
    if ((i32)cursor < (i32)count) {
        do {
            i960_st_u16(I960_ABS, dest, 0, model2_workram_mirror_u16(src));
            src += 2u;
            dest += 2u;
            copied++;
            cursor++;
            /* @0x33414 / 0x33424: stop after 0x1f3 or cursor >= count */
            if (copied > COLORBASE_CHUNK)
                break;
        } while ((i32)cursor < (i32)count);
    }

    /* @0x33438: if cursor >= count, clear mode (g14==0 in host ABI). */
    if (cursor >= count) {
        cursor = 0u;
        i960_st_u32(I960_WORKRAM, 0x213850, 0, 0);
    }
    i960_st_u32(I960_WORKRAM, 0x213834, 0, cursor);
}
