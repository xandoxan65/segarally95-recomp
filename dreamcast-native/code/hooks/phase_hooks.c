/* Race phase presentation hooks on the Dreamcast: nothing yet (include/race/phase_hooks.h). */
#include "race/phase_hooks.h"

uint32_t sr_hook_catalog_draw(uint32_t x, uint32_t y, uint32_t catalog, uint32_t glyph, uint32_t layer) {
    (void)x;
    (void)y;
    (void)catalog;
    (void)glyph;
    (void)layer;
    return 0;
}

void sr_hook_analog_idle(uint32_t all) { (void)all; }
