/* The palette interrupt's fade state machine (code/race/palette_irq.c) against the lifted originals
 * (ported/palette.list). The GEO colour tables and palette RAM it also writes are hardware, not compared. */
#include "wrap_common.h"
#include "race/palette_irq.h"

/* Compared where the port covers the frame: mode 4, or mode 0 promoting nothing or a mode-4 fade. Modes 1, 2, 6 and
 * 7 (boot and after the race: snapshots of the GEO colour tables, colour base uploads) run the original alone. */
void lifted_geo_palette_irq_frame(u32 a0, u32 a1, u32 a2);
void geo_palette_irq_frame(u32 a0, u32 a1, u32 a2) {
    const uint32_t mode = sr_ld32(0x213850u), queued = sr_ld32(0x213840u);
    if(!(mode == 4 || (mode == 0 && (queued == 0 || queued == 4)))) {
        lifted_geo_palette_irq_frame(a0, a1, a2);
        return;
    }
    SR_DIFF_BEGIN();
    lifted_geo_palette_irq_frame(a0, a1, a2);
    SR_DIFF_NATIVE();
    sr_palette_irq_frame();
    SR_DIFF_END("geo_palette_irq_frame");
}
SR_DIFF_WRAP3(geo_palette_mode0_promote, sr_palette_mode0_promote())
SR_DIFF_WRAP3(geo_palette_mode4_fade, sr_palette_mode4_fade())
SR_DIFF_WRAP3(geo_palette_fade_step, sr_palette_fade_step())
SR_DIFF_WRAP3(geo_palette_fade_commit, sr_palette_fade_commit())
