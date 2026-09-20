/* Palette / colorxlat irq frame @ 0x33180 — called after splash each vblank. */
// @rom 0x33180 +0x3c geo_palette_irq_frame

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"

/*
 * Disasm @ 0x33180 jump table @ 0x5D21BC (maincpu 0x331BC via ROM mirror):
 *   [0] mode0 promote
 *   [1]/2] colorxlat→CRX stage then mode7
 *   [3] fade (not yet)  [4] mode4 fade (attract)
 *   [5] colorbase rewrite @ 0x33744 (LUT 0x5D1FD0)
 *   [6] CRX→colorxlat restore @ 0x332D0 (after mode1/2 snapshot)
 *   [>6] palram_geo_colorbase_refresh @ 0x333C8
 *
 * GEO colorbase writers: boot/refresh/mode5 only (table @ 0x5FB89E →
 * 0x01802000). Same path attract uses for desert course colour.
 *
 * F620 queues mode4 with scale=0,target=0x100 (fade toward wash). Splash then
 * queues mode6 to restore colorxlat from the mode1/2 CRX snapshot — do not
 * skip mode6 or the wash sticks.
 */
void geo_palette_irq_frame(u32 arg0, u32 arg1, u32 arg2)
{
    u16 tick;
    u32 mode;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    tick = i960_ld_u16(I960_WORKRAM, 0x213862, 0);
    tick = (u16)((tick + 2u) & 0xffu);
    i960_st_u16(I960_WORKRAM, 0x213862, 0, tick);
    i960_st_u16(I960_WORKRAM, 0x213860, 0, tick);

    mode = i960_ld_u32(I960_WORKRAM, 0x213850, 0);

    if (mode == 0u) {
        geo_palette_mode0_promote(0, 0, 0);
        mode = i960_ld_u32(I960_WORKRAM, 0x213850, 0);
    }

    if (mode == 1u) {
        geo_palette_mode2_stage(1u, 0, 0);
        return;
    }
    if (mode == 2u) {
        geo_palette_mode2_stage(0u, 0, 0);
        return;
    }

    if (mode == 4u) {
        geo_palette_mode4_fade(0, 0, 0);
        return;
    }

    if (mode == 5u) {
        geo_palette_mode5_colorbase(0, 0, 0);
        return;
    }

    if (mode == 6u) {
        geo_palette_mode6_commit(0, 0, 0);
        return;
    }

    if (mode > 6u)
        palram_geo_colorbase_refresh(0, 0, 0);
}
