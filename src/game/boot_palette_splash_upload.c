/* Boot splash palette stream → palram @ 0x26758. */
// @rom 0x26758 +0x98 boot_palette_splash_upload

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_rom.h"

#define PALETTE_SRC_VADDR  0x005C5670u  /* lda @ 0x26760; ldos (g7) — no static st in disasm */
#define PALETTE_BASE       0x01800000u
#define PALETTE_EXT_BASE   0x01802000u

void boot_palette_splash_upload(u32 arg0, u32 arg1, u32 arg2)
{
    u32 src;
    u32 dest;
    u32 slot;
    u32 pad;
    u32 ext_count;
    u32 ext_idx;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    src = PALETTE_SRC_VADDR;
    dest = PALETTE_BASE;
    for (slot = 0; slot <= 9u; slot++) {
        u16 color = model2_workram_mirror_u16(src);
        u32 pen;

        src += 2u;
        /* @0x26778–0x267A4: 16 words/slot; pen 0 transparent, pens 1–15 share ROM color. */
        i960_st_u16(I960_ABS, dest, 0, 0);
        dest += 2u;
        for (pen = 1u; pen <= 15u; pen++) {
            i960_st_u16(I960_ABS, dest, 0, color);
            dest += 2u;
        }
    }

    /* @0x267B0: ldos 0x005FB89C,g0 — runtime gate; static lift uses ROM mirror. */
    ext_count = model2_workram_mirror_u16(0x005fb89cu);
    if (ext_count == 0)
        return;

    /* @0x267C8–0x267E8: g6=0; load, g6++, store, loop while g6 < count. */
    dest = PALETTE_EXT_BASE;
    src = 0x005fb89eu;
    ext_idx = 0u;
    while (ext_idx < ext_count) {
        i960_st_u16(I960_ABS, dest, 0, model2_workram_mirror_u16(src));
        src += 2u;
        dest += 2u;
        ext_idx++;
    }
}
