/* Course renderer core (code/render/course_render.c): the course pack's instances seen through the race camera,
 * culled, transformed and projected, handed to a backend batch by batch. Portable C: the Dreamcast backend submits
 * to the PVR (code/render/pvr_backend.c), the host preview rasterises to an image (tests/native_run/preview.c). */
#ifndef SR_RENDER_COURSE_RENDER_H
#define SR_RENDER_COURSE_RENDER_H
#include <stddef.h>
#include <stdint.h>
#include "course_pack.h"

/* Triangles of one chunk drawn with one material: a texture (or SR_NO_TEXTURE), the cut-out flag and a layer. */
typedef struct {
    uint16_t texture, flags; /* SR_TRI_* */
    uint32_t first, count;   /* triangles in the chunk's index list */
} sr_course_batch_t;

/* Mesh triangles regrouped at load: nearby triangles with their shared vertices, a bounding sphere, and batches
 * ordered by list, texture and layer. */
typedef struct sr_course_chunk {
    uint32_t n_verts, n_tris, n_batches;
    float bounds[4];
    uint32_t lists; /* bit 0: has opaque batches, bit 1: has cut-out batches */
    sr_course_vertex_t *verts;
    /* the vertices' distinct positions (n_pos x 3 floats) and each vertex's one: quads sharing a corner with their
     * own texture coordinates transform it once (the Dreamcast's path) */
    uint32_t n_pos;
    float *pos;
    uint16_t *vpos;
    uint16_t *index; /* n_tris * 3 */
    /* pair[t] = 1: triangles t and t+1 are a fanned quad (a,b,c),(a,c,d), drawn as the strip b,a,c,d */
    uint8_t *pair;
    uint16_t *src_tri; /* each triangle's index in its mesh (the pack's order: the light file's) */
    sr_course_batch_t *batches; /* with the arrays above, one block per chunk (course_render.c pack_chunk) */
} sr_course_chunk_t;

/* One triangle's lighting terms (render/actors.h sr_actors_light_load, from the course export's .light file):
 * its normal in the mesh's space and the GEO's luma terms; the cars are lit per frame as the arcade does. */
typedef struct sr_tri_light {
    float n[3]; /* the unit normal in the mesh */
    float d;    /* n . a corner: the plane's offset (which side of it the eye is) */
    float ambient, diffuse, spec_scale;
    uint8_t spec_ctrl, spec_mode;
    uint16_t table; /* into sr_light_tables: brightness k(luma) of the colours (baked at luma 255) */
} sr_tri_light_t;

/* k(L) at L = 0, 16, .. 256, per table */
extern const float (*sr_light_tables)[17];
/* The GEO's light vector in view space (the camera's, race/render_hooks.h), set by the caller each frame. */
extern float sr_light_view[3];
/* The GEO's polygon luma this frame (MAME geo_parse: |N.L| x diffuse + ambient, + specular in the GEO's specular
 * mode, 0 when the light and the eye are on either side of the face), N and a point of the face in view space,
 * through its colours' table: the factor on colours baked at luma 255. */
float sr_light_factor(const struct sr_tri_light *l, const float n[3], const float p[3]);

typedef struct {
    uint32_t n_textures, n_meshes, n_instances;
    sr_course_texture_t *textures;
    sr_course_mesh_t *meshes;
    sr_course_instance_t *instances;
    float (*instance_bounds)[4]; /* filled at load: each instance's bounding sphere in the world */
} sr_course_t;

/* Parse a pack held in memory (it must outlive the course: textures and meshes point into it). 0 on success. */
int sr_course_parse(sr_course_t *course, const uint8_t *data, size_t size);

/* One projected vertex: 32 bytes, a cache line. */
typedef struct {
    float x, y;   /* screen pixels, y down */
    float invz;   /* 1 / view depth, the PVR's depth value (decal layers already biased) */
    float z;      /* view depth */
    float u, v;
    uint32_t argb;
    uint32_t behind; /* in front of the near plane: 0 */
} sr_screen_vertex_t;

/* The arcade's projection: GEO focus 350 on a 496x384 window centred at (248, 192), so at 640x480 the focal lengths
 * are 350 * 640 / 496 across and 350 * 480 / 384 down, centred on the screen. */
#define SR_FOCAL_X (350.f * 640.f / 496.f)
#define SR_FOCAL_Y (350.f * 480.f / 384.f)

#define SR_LIST_OPAQUE 0
#define SR_LIST_CUTOUT 1
#define SR_LIST_BOTH   2 /* one traversal for both lists; with view->deferred, cut-out batches go there */
#define SR_LIST_TRANSLUCENT 3 /* every batch, in view->argb (the cars' shadows: actors.c) */

/* Dreamcast: TA data (32-byte headers and vertices) for the punch-through list, built while the opaque list is
 * submitted and sent after it, so each chunk is culled and transformed once a frame. */
typedef struct {
    uint8_t *data;
    uint32_t used, cap, dropped; /* dropped: primitives that did not fit */
} sr_deferred_t;

/* Room for `bytes` more, or NULL (counted as dropped) when full. */
static inline void *sr_deferred_alloc(sr_deferred_t *d, uint32_t bytes) {
    void *p;
    if(d->used + bytes > d->cap) {
        d->dropped++;
        return 0;
    }
    p = d->data + d->used;
    d->used += bytes;
    return p;
}

typedef struct {
    float width, height;    /* target in pixels */
    float cx, cy;           /* projection centre */
    float focal_x, focal_y; /* pixels per unit at depth 1 */
    float znear;
    int list;               /* SR_LIST_*: which batches this pass draws */
    /* A new material before its triangles. */
    void (*material)(void *user, uint16_t texture, uint16_t flags);
    /* Receives each triangle. NULL on the Dreamcast: the core writes them to the TA itself (store queues). */
    void (*tri)(void *user, const sr_screen_vertex_t v[3]);
    void *user;
    sr_deferred_t *deferred; /* Dreamcast, SR_LIST_BOTH: where cut-out triangles go (NULL: drawn directly) */
    uint32_t argb;           /* SR_LIST_TRANSLUCENT: the colour every vertex gets */
    float depth_bias;        /* SR_LIST_TRANSLUCENT: 1/z times this (lifts shadows off the ground they lie on) */
    /* Dreamcast, lists OPAQUE / CUTOUT / BOTH: the 32-byte TA headers by texture (index n_headers: untextured) for
     * the opaque [0] and cut-out [1] lists, which the core sends itself in place of calling material (no call per
     * batch, and its code stays with the triangle loop's in the instruction cache); current[list] is the header
     * last sent (0xffff: none), shared with the caller. headers[0] NULL: material is called. */
    const void *headers[2];
    uint32_t n_headers;
    uint16_t *current;
} sr_view_t;

typedef struct {
    uint32_t instances_drawn, instances_culled, tris_drawn, tris_clipped; /* clipped: crossed the near plane */
    uint32_t chunks_drawn, verts_transformed, batches, tris_culled; /* culled: back faces */
    uint64_t cycles_xform, cycles_emit; /* Dreamcast: CPU cycles transforming chunks' vertices, emitting triangles */
    uint64_t cycles_direct, cycles_deferred, cycles_material; /* RENDER_PROF=1: parts of cycles_emit */
    uint64_t freeze_xform, freeze_emit; /* RENDER_PROF=1: the PRFC1 event (main.c) in the transform, the emit */
} sr_render_stats_t;

/* Back faces: which screen winding to drop for triangles without SR_TRI_TWO_SIDED (+1 or -1; 0 draws both; default -1, the arcade's). */
extern int sr_render_cull;

/* Draw the static instances' batches of view->list through `view_matrix` (the race camera's TGP view). */
void sr_course_render(const sr_course_t *course, const float view_matrix[12], const sr_view_t *view,
                      sr_render_stats_t *stats);

/* Draw one mesh through view x world (dynamic objects: cars, the helicopter). */
void sr_mesh_render(const sr_course_t *course, uint32_t mesh, const float view_matrix[12], const float world[12],
                    const sr_view_t *view, sr_render_stats_t *stats);

/* Draw one mesh through a model-view matrix the caller built (the particles: no rotation, scaled in view space). */
void sr_mesh_render_mv(const sr_course_t *course, uint32_t mesh, const float mv[12], const sr_view_t *view,
                       sr_render_stats_t *stats);

/* Model 2 3x4 matrices: p' = R p + T, R's columns at 0..2, 3..5, 6..8, T at 9..11. out = a after b. */
void sr_m2_compose(float out[12], const float a[12], const float b[12]);

#endif
