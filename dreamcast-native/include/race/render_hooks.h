/* Where gameplay code hands values to the renderer. In the differential checks these write the Model 2 ports as
 * the lifted code does (tests/diff/wrap_camera.c); on the Dreamcast the native renderer takes them
 * (code/hooks/render_hooks.c). */
#ifndef SR_RACE_RENDER_HOOKS_H
#define SR_RACE_RENDER_HOOKS_H
#include <stdint.h>

/* geo_view_matrix_dir_emit (0x29120): GEO register 0x8000a0 <- `reg`, then the view-space direction xyz (float
 * bits) to the GEO program FIFO 0x804000 — the light direction for the course and cars. */
void sr_hook_geo_light_dir(uint32_t reg, uint32_t x, uint32_t y, uint32_t z);

/* Alternate camera (0x1FF60): GEO register 0x800090 <- `reg`, then two floats to 0x804000 (both 300.0). */
void sr_hook_geo_view_range(uint32_t reg, uint32_t a, uint32_t b);

#endif
