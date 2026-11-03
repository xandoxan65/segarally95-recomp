/* The cars' dust and debris (code/race/particles.c): the race frame's object pens (0x24C50 walk, 0x248B0
 * descriptor, 0x247D0 pen, 0x23A80 / 0x24560 / 0x240C0 emit) that draw the cars on Model 2 and, as they go,
 * spawn particles (0x317B0) into the pool at 0x20cd90 (36 bytes each: 0x20d600 live indices, 0x20d6f0 free list,
 * counts 0x20d7e0..0x20d7f4); then the particle submit (0x31FC0 / 0x32150) that steps, draws and retires them.
 *
 * Every RAM write and TGP command is the original's. GEO writes go to the desert and course hooks
 * (race/desert_hooks.h, race/course_hooks.h) and sr_hook_pen_push_a; what the submit would draw is kept in a
 * list the renderer reads after the tick (sr_particles_draws). */
#ifndef SR_RACE_PARTICLES_H
#define SR_RACE_PARTICLES_H
#include <stdint.h>

/* geo_fifo_bootstrap (0x4AC0) at the start of the race frame: its RAM part (0x202270/74 <- 350.0, the polygon
 * count 0x20b940 <- 0). */
void sr_geo_frame_begin(void);
/* geo_fifo_preset (0x3EC78) after the frame: the texture staging cursor 0x214374 <- 0, 0x214370 <- 0x800100. */
void sr_geo_frame_end(void);

void sr_object_pen_walk(uint32_t flag_bit);                         /* 0x24C50 */
void sr_object_pen_desc(uint32_t desc);                             /* 0x248B0 */
void sr_object_pen(uint32_t obj, uint32_t scale, uint32_t budget);  /* 0x247D0 */
void sr_glyph_emit(uint32_t pen, uint32_t obj, uint32_t budget);    /* 0x23A80 */
void sr_object_pen_far(uint32_t pen, uint32_t obj, uint32_t budget); /* 0x24560 */
void sr_object_pen_mid(uint32_t pen, uint32_t obj, uint32_t budget); /* 0x240C0 */
void sr_particle_spawn(uint32_t obj);                               /* 0x317B0 geo_attract_object_extra */
uint32_t sr_particle_rng(void);                                     /* 0x33DC8 geo_rng_next */
void sr_particles_submit(void);                                     /* 0x31FC0 geo_attract_copro_submit */
uint32_t sr_particle_step(uint32_t word0, uint32_t ix);             /* 0x32150 geo_attract_copro_object */

/* The race frame's car pens and particles in the original's order around the course section
 * (code/race/race_tick.c): before it the walk, after it the submit and, in the behind view, the followed car. */
void sr_race_pens_before_course(void);
void sr_race_pens_after_course(void);

/* One particle as the submit draws it: TGP 0x55 places GEO object `oba` (one textured quad, catalog
 * 0x28655c0 = 0x82c95b dust, 0x28655d0 = 0x82c96c debris) at the world point pos, transformed by the TGP's
 * working matrix (sr_particles_view), with no rotation of its own and the view-space scale (sx, sy, sz) = (1.5, size, 1.0): a quad
 * that always faces the screen, 1.5 wide and `size` tall in its model units. */
typedef struct {
    float pos[3];
    float size;
    uint32_t oba;
    uint32_t kind; /* 0 dust (grows, fades by its 8-bit life in word 0), 1 debris (falls under gravity) */
} sr_particle_draw_t;

#define SR_PARTICLE_DRAWS_MAX 64
/* What the last race tick's submit drew. */
uint32_t sr_particles_draws(const sr_particle_draw_t **out);
/* The matrix the points are transformed by: the TGP's working matrix at the submit (not always the camera's
 * latched view: in the behind view it carries the course section's scale). */
void sr_particles_view(float out[12]);

/* geo_attract_pen_prg_push_a (0x3EF70): the texture pen for a flagged pen mode (GEO and the 0x214380 staging
 * only). */
void sr_hook_pen_push_a(uint32_t pen_mode);

#endif
