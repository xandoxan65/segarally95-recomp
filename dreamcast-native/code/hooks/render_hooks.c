/* Render hooks on the Dreamcast (include/race/render_hooks.h). */
#include "race/render_hooks.h"

uint32_t sr_light_dir[3];

void sr_hook_geo_light_dir(uint32_t reg, uint32_t x, uint32_t y, uint32_t z) {
    (void)reg;
    sr_light_dir[0] = x;
    sr_light_dir[1] = y;
    sr_light_dir[2] = z;
}

uint32_t sr_view_range[2];

void sr_hook_geo_view_range(uint32_t reg, uint32_t a, uint32_t b) {
    (void)reg;
    sr_view_range[0] = a;
    sr_view_range[1] = b;
}
