/* Moving objects drawn from live RAM (code/render/actors.c): the course pack's car and helicopter meshes placed by
 * the attach file (build/course/desert.attach, "SRATT01": mesh, pose block address, local matrix). */
#ifndef SR_RENDER_ACTORS_H
#define SR_RENDER_ACTORS_H
#include <stddef.h>
#include <stdint.h>
#include "render/course_render.h"

typedef struct {
    uint32_t mesh, pose_va;
    float local[12];
} sr_attach_t;

#define SR_SHADOW_OUTLINE_MAX 24

/* The particle quads (race/particles.h): GEO objects of the dust and the debris. */
#define SR_OBA_DUST   0x82c95bu
#define SR_OBA_DEBRIS 0x82c96cu

typedef struct {
    uint32_t n;
    sr_attach_t *a;
    /* the shadow plate's boundary in its own plane (x, z), found at load */
    uint32_t shadow_n;
    float shadow_outline[SR_SHADOW_OUTLINE_MAX][2];
    /* the particle meshes in the course pack (dust, debris), n_meshes when the pack has none */
    uint32_t particle_mesh[2];
} sr_actors_t;

int sr_actors_parse(sr_actors_t *actors, const sr_course_t *course, const uint8_t *data, size_t size);

/* The cars' lighting (the course export's .light file, "SRLIT01"): attaches each record's triangle terms to the
 * course mesh with its oba and triangle count (mesh->light), normals from the triangles, and the brightness tables
 * (sr_light_tables). The pack's car colours must be the ones baked at luma 255 by the same export. The number of meshes lit, -1 on a bad file. */
int sr_actors_light_load(sr_course_t *course, const uint8_t *data, size_t size);

/* Draw the actors' batches of view->list (SR_LIST_TRANSLUCENT: their shadows). The player's own car (pose 0x213b40)
 * is left out in the bumper view. */
void sr_actors_render(const sr_actors_t *actors, const sr_course_t *course, const float view_matrix[12],
                      const sr_view_t *view, sr_render_stats_t *stats);

/* The last race tick's dust and debris (race/particles.h) as screen quads: each the pack's particle mesh (one quad,
 * cut-out texture) scaled to 1.5 wide and `size` tall at its point in the TGP's view, parallel to the screen.
 * Corners in strip order, with the pack's texture coordinates (the texture once across the quad). */
typedef struct {
    float x, y, invz, u, v;
} sr_particle_vertex_t;

typedef struct {
    uint16_t texture;
    uint32_t argb; /* its brightness (the pack's .light: lit per frame; else white) */
    sr_particle_vertex_t v[4];
} sr_particle_quad_t;

uint32_t sr_particles_quads(const sr_actors_t *actors, const sr_course_t *course, const sr_view_t *view,
                            sr_particle_quad_t *out, uint32_t max);

/* The cars' shadows as closed volumes for the PVR's modifier list: each shadow plate's outline, posed as the car
 * draws it, extruded 1 unit up and down along the world's vertical into a prism, projected like the course (x, y in
 * pixels, 1/z). tri gets every triangle, last = 1 for the one that closes its volume. Volumes crossing the near
 * plane, entirely off screen, or the player's in the bumper view (the eye is inside it) are left out. Returns the
 * volumes sent. */
uint32_t sr_actors_shadow_volumes(const sr_actors_t *actors, const sr_course_t *course, const float view_matrix[12],
                                  const sr_view_t *view, void (*tri)(void *user, const float v[3][3], int last),
                                  void *user);

#endif
