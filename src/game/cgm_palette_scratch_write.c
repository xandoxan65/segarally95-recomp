/* Scratch pen stripe write @ 0x2A490.
 * source: decomp/disasm/maincpu/maincpu_02a490_80.asm */
// @rom 0x2a490 +0x48 cgm_palette_scratch_write

#include "i960_lift.h"
#include "i960_mem.h"

#define SCRATCH_COLORBASE_BASE  0x01800000u
#define CGM_G13_MASK            0x0020C958u

/*
 * ABI @ 0x2A490 (leaf_bx via 0x5C94D8): g0 = pen 0..15, g1 = color15.
 *
 *   cmpibl 15,g0 → ret if g0 > 15
 *   row = (0x20C958 >> 7) << 5
 *   ea  = 0x01800000 + row + g0*2     (lda, not ld)
 *   for 24 stores: stos g1,(ea); ea += 0x20
 *
 * Writes the same color15 into one pen across 24 consecutive 32-byte scratch
 * rows. Does not touch the GEO colorbase table @ 0x01802000 (that is only
 * boot_palette_splash_upload @ 0x26758 / palram_geo_colorbase_refresh @ 0x333C8).
 */
void cgm_palette_scratch_write(u32 pen, u32 color15)
{
    u32 g13;
    u32 ea;
    u32 remain;

    /* @0x2A4A0: cmpibl 15,g0 — reject pen > 15. */
    if (pen > 15u)
        return;

    g13 = i960_ld_u32(I960_WORKRAM, CGM_G13_MASK, 0);
    /* @0x2A4A4–0x2A4B8: lda 0x1800000((g13>>7)<<5)[g0*2] → pen cell EA. */
    ea = SCRATCH_COLORBASE_BASE
        + (((g13 >> 7) & 0x7ffu) << 5)
        + (pen << 1);
    /* @0x2A4B4 mov 23,g5; @0x2A4C0–0x2A4D0: 24 stores, ea += 0x20 each. */
    remain = 24u;
    while (remain > 0u) {
        i960_st_u16(I960_ABS, ea, 0, (u16)(color15 & 0xffffu));
        ea += 0x20u;
        remain--;
    }
}
