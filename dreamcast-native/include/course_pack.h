/* Course pack: the world the Dreamcast draws, exported from the host reference (dreamcast-native/tools,
 * export_course.sh / segamod2 --export-course).
 *
 * Version 2 (textured). Little-endian, every section 4-byte aligned:
 *   char     magic[8]      "SRCRS02\0"
 *   uint32_t n_textures, n_meshes, n_instances
 *   n_textures x texture:
 *     uint16_t w, h        powers of two, 8..1024
 *     uint32_t flags       SR_TEX_* below
 *     uint16_t texels[w*h] ARGB1555, twiddled (PVR order); alpha 0 is a cut-out texel
 *                          or, with SR_TEX_VQ, the PVR's VQ form: uint16_t codebook[256][4] (2x2 texels each, in
 *                          twiddled order) then uint8_t index[w*h/4], the entry of each 2x2 block in twiddled order
 *   n_meshes x mesh:
 *     uint32_t oba          polygon ROM object address (with obc, the key the game draws it by)
 *     uint32_t obc
 *     uint32_t n_tris
 *     float    bounds[4]    bounding sphere in the mesh's own space: centre xyz, radius
 *     n_tris x triangle:
 *       uint16_t texture    index into the textures, SR_NO_TEXTURE for flat colour
 *       uint16_t flags      SR_TRI_* below
 *       3 x vertex:         float x, y, z, u, v; uint32_t argb (vertex colour; modulates the texture)
 *   n_instances x instance:
 *     uint32_t mesh         index into the meshes
 *     uint32_t flags        SR_INST_* below
 *     float    m[12]        world matrix, Model 2 / TGP column layout: rotation 0..8 (columns), translation 9..11
 *
 * u, v are in texture units (0..1 across the texture); values outside repeat or mirror per the texture's flags.
 * Mesh vertices are in the object's own space; an instance's matrix places it in the world the race camera (the TGP
 * view it latches) looks at. Model 2 view space: x right, y up, z forward. */
#ifndef SR_COURSE_PACK_H
#define SR_COURSE_PACK_H
#include <stdint.h>

#define SR_COURSE_MAGIC "SRCRS02"

#define SR_INST_STATIC 1u /* drawn at the same world matrix in every captured frame: course scenery */

#define SR_NO_TEXTURE 0xffffu

#define SR_TEX_REPEAT_U 1u /* u wraps (else clamps) */
#define SR_TEX_REPEAT_V 2u
#define SR_TEX_MIRROR_U 4u /* u mirrors on each repeat */
#define SR_TEX_MIRROR_V 8u
#define SR_TEX_VQ       16u /* stored vector-quantised (tools/vq_textures.py) */

/* Bytes of a texture's texel data in the pack (before the 4-byte alignment). */
static inline uint32_t sr_course_texture_bytes(uint32_t w, uint32_t h, uint32_t flags) {
    return (flags & SR_TEX_VQ) ? 2048u + w * h / 4u : w * h * 2u;
}

#define SR_TRI_CUTOUT     1u  /* has cut-out texels: punch-through list */
#define SR_TRI_TWO_SIDED  2u  /* drawn from both sides (GEO attr bit 17); others only when facing the eye */
#define SR_TRI_LAYER_MASK 0xf0u /* decal layer 0..15: drawn on top of coplanar faces of lower layers in the */
#define SR_TRI_LAYER_SHIFT 4    /* same object (the arcade's painter order); the renderer biases depth by it */

typedef struct {
    float x, y, z, u, v;
    uint32_t argb;
} sr_course_vertex_t;

typedef struct {
    uint16_t texture, flags;
    sr_course_vertex_t v[3];
} sr_course_tri_t;

typedef struct {
    uint16_t w, h;
    uint32_t flags;
    const uint16_t *texels;
} sr_course_texture_t;

typedef struct {
    uint32_t oba, obc, n_tris;
    float bounds[4];
    const sr_course_tri_t *tris;
    /* Filled at load by the renderer (code/render/course_render.c). */
    uint32_t n_chunks;
    struct sr_course_chunk *chunks;
    const struct sr_tri_light *light; /* per triangle, the lit meshes only (render/actors.h) */
} sr_course_mesh_t;

typedef struct {
    uint32_t mesh, flags;
    float m[12];
} sr_course_instance_t;

#endif
