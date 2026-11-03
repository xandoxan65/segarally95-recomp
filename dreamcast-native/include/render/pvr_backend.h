/* The course renderer's Dreamcast backend (code/render/pvr_backend.c): textures in VRAM, one compiled header per
 * texture and list, triangles to the TA through the store queues. */
#ifndef SR_RENDER_PVR_BACKEND_H
#define SR_RENDER_PVR_BACKEND_H
#include <stddef.h>
#include "render/actors.h"
#include "render/course_render.h"

/* Upload the course's textures and compile their headers (after pvr_init). 0 on success. */
int sr_pvr_course_init(const sr_course_t *course);

/* Draw the course's batches for one list (SR_LIST_OPAQUE from the opaque-list callback, SR_LIST_CUTOUT from the
 * punch-through one), plus the dynamic objects the caller places. SR_LIST_BOTH, from the opaque-list callback:
 * both at once, the punch-through list kept in RAM until sr_pvr_flush_deferred (from the punch-through callback;
 * returns the primitives dropped so far for want of room). */
void sr_pvr_begin(int list, const float view_matrix[12], sr_render_stats_t *stats);
void sr_pvr_course(const sr_course_t *course);
void sr_pvr_mesh(const sr_course_t *course, uint32_t mesh, const float world[12]);
void sr_pvr_actors(const sr_actors_t *actors, const sr_course_t *course);
uint32_t sr_pvr_flush_deferred(void);
/* The countdown's 3D digits (render/countdown.h), after sr_pvr_flush_deferred. */
void sr_pvr_countdown(const sr_course_t *course);
/* A white wash over the frame, wash / 256 (race/palette_irq.h): from the translucent-list callback. */
void sr_pvr_wash(uint32_t wash);

/* The cars' shadows as modifier volumes (actors.c): build this frame's in the updater (returns the volumes; add the
 * opaque modifier list only when there are some), send them from that list's callback. */
uint32_t sr_pvr_shadows_prepare(const sr_actors_t *actors, const sr_course_t *course, const float view_matrix[12]);
void sr_pvr_shadows_submit(void);
void sr_pvr_particle_report(void);

/* The race HUD (code/render/pvr_hud.c): characters and palette from the race pack (lockstep.h), then each frame the
 * tile map's cells, from the punch-through callback after the 3D. */
int sr_pvr_hud_init(const uint8_t *chars, const uint8_t *palette, const sr_course_t *course);
void sr_pvr_hud_submit(void);
void sr_pvr_hud_needle(void);
void sr_pvr_hud_report(void);
/* Forget which palette lines are in which PVR bank (another user of the palette RAM overwrote them). */
void sr_pvr_hud_reset_palette(void);

/* The title card before the countdown (code/render/pvr_title.c, file from tools/make_title_image.py): load returns
 * the frames to show it (0: none); begin loads its palette (again after a restart); draw from the opaque callback. */
uint32_t sr_pvr_title_load(const uint8_t *data, size_t size);
void sr_pvr_title_begin(void);
void sr_pvr_title_draw(void);

#endif
