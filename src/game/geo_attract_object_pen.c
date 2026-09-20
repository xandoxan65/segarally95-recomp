/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_0247d0_70.asm */
// @rom 0x247d0 +0x70 geo_attract_object_pen

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "lift_syms.h"
#include "model2_rom.h"

/*
 * arg0 = host pointer to attract object (fp shadow from string_draw).
 * arg1 = float bits (scale / distance), typically 0x3f800000.
 *
 * Disasm: lda 0x5c2780[pen_ix*3*16], g0 — address of pen record, not a load.
 */
void geo_attract_object_pen(void * arg0, u32 arg1, u32 arg2)
{
    u8 *obj;
    u32 pen_ix;
    u32 pen_addr;
    u32 pen_limit;
    u8 flags;

    (void)arg2;

    if (arg0 == 0)
        return;

    obj = (u8 *)arg0;

    /* cmprl: continue when scale <= 500.0 (rifl 0 / 0x407f4000); else ret. */
    fp0 = i960_u32_to_f64(arg1);
    fp1 = i960_rifl_read(0, 0x407f4000);
    if (fp0 > fp1)
        return;

    pen_limit = i960_ld_u32(I960_WORKRAM, 0x213970, 0);
    if (i960_u32_to_f64(arg1) >= i960_u32_to_f64(pen_limit))
        return;

    flags = obj[0x50];
    pen_ix = flags & 15u;
    obj[0x50] = (u8)(flags & 0x9fu);

    /* lda (pen_ix)[pen_ix*2] → pen_ix*3; lda 0x5c2780[that*16] → address. */
    pen_addr = 0x5c2780u + ((pen_ix + (pen_ix << 1)) << 4);
    g0 = pen_addr;
    g1 = (uintptr_t)arg0;
    geo_attract_glyph_emit((void *)(uintptr_t)pen_addr, arg0, arg2);
}
