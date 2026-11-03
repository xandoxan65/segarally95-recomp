/* The palette interrupt's per-frame work (code/race/palette_irq.c): geo_palette_irq_frame (0x33180) from the vblank,
 * the fade state machine at 0x213840 (queue) / 0x213850 (active: mode, scale, target, step).
 *
 * Mode 4 washes the GEO colour tables toward white by scale / 256 (0x100 = all white): the start's white fade-in
 * and the fly-in's wash. The tables themselves (0x01810000..) are GEO hardware, not RAM images here; their CRX work
 * buffers (0x210830..) and the state words are kept exactly. The renderer reads sr_palette_wash(). */
#ifndef SR_RACE_PALETTE_IRQ_H
#define SR_RACE_PALETTE_IRQ_H
#include <stdint.h>

void sr_palette_irq_frame(void);        /* 0x33180 */
void sr_palette_mode0_promote(void);    /* 0x33864 */
void sr_palette_mode4_fade(void);       /* 0x335B4 */
void sr_palette_fade_step(void);        /* 0x33038 */
void sr_palette_fade_commit(void);      /* 0x330A0 */

/* The white wash of the 3D view now, 0 (none) .. 256 (white): the active mode-4 fade's scale, else 0. */
uint32_t sr_palette_wash(void);

#endif
