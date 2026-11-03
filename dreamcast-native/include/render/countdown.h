/* The countdown's 3D digits (race/race_phases.h, sr_countdown_draws) as screen triangles (code/render/countdown.c):
 * each drawn object's mesh from the course pack through the matrix the countdown gave the TGP, projected in the
 * start lights' GEO window (focus 512, centre (248, 192) of the 496 x 384 screen, view +y up), scaled to
 * width x height. The arcade draws that window over the course: the backends put them in front. */
#ifndef SR_RENDER_COUNTDOWN_H
#define SR_RENDER_COUNTDOWN_H
#include <stdint.h>
#include "render/course_render.h"

typedef struct {
    uint16_t texture, flags; /* the pack triangle's */
    float x[3], y[3], invz[3], u[3], v[3];
    uint32_t argb[3];
} sr_countdown_tri_t;

uint32_t sr_countdown_tris(const sr_course_t *course, float width, float height, sr_countdown_tri_t *out,
                           uint32_t max);

#endif
