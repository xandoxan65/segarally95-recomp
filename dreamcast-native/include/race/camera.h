/* The race camera (code/race/camera.c). It builds the view on the TGP and latches it; the cars' updates later in
 * the tick start from that latched view, so the camera is part of the gameplay tick, not only of rendering. */
#ifndef SR_RACE_CAMERA_H
#define SR_RACE_CAMERA_H
#include <stdint.h>

#define SR_CAM_SLOT       0x20ab4cu /* installed camera handler (staged ROM address) */
#define SR_CAM_SENTINEL   0x5be1d0u /* seeds the camera when installed (0x1F1D0) */
#define SR_CAM_DESERT     0x5bea20u /* opening desert fly-in (0x1FA20) */
#define SR_CAM_CHASE      0x5bedc0u /* behind the car (0x1FDC0) */
#define SR_CAM_CHASE_ALT  0x5bef60u /* the other view (0x1FF60), selected by 0x2140c0 */

/* 0x20310. handler != 0: install it (seeding for the sentinel); 0: run the installed one. `g14` is the value the
 * original stores to 0x2139d8 when the view toggle (0x202064 bit 7) switches to the alternate view. */
void sr_cam_bind(uint32_t handler, uint32_t g14);
void sr_cam_rebind(void);       /* 0x202E0 */
void sr_cam_chase(void);        /* 0x1FDC0 */
void sr_cam_chase_alt(void);    /* 0x1FF60 */
void sr_cam_desert(void);       /* 0x1FA20 */
/* 0x33C00: point on the uniform cubic B-spline through the four xyz control points at table_va (float bits),
 * at t (float bits). */
void sr_spline_point(uint32_t table_va, uint32_t t, uint32_t out[3]);
/* 0x33A50: swap a 3x4 matrix's off-diagonal rotation terms (transpose), and re-express its translation. */
void sr_matrix_row_swap(uint8_t *m);
/* 0x29120: transform (x, y, z) by the current view's rotation and hand it to the renderer as the light. */
void sr_cam_light_dir(uint32_t x, uint32_t y, uint32_t z, uint32_t g14);

#endif
