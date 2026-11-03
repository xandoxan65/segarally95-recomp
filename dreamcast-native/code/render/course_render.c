/* Course renderer core: see include/render/course_render.h.
 *
 * At load the pack's triangles are cut into chunks of nearby triangles (a cell grid over each large mesh), each with
 * its own bounding sphere and shared vertices, its triangles ordered into batches by list, texture and decal layer:
 * the course comes as a few dozen large world-space segments, so culling whole meshes keeps most of them, and each
 * vertex is otherwise transformed once per triangle using it. Per frame and pass, per instance and chunk: sphere
 * test, transform the chunk's vertices once, then the batches' triangles from indices, clipped at the near plane
 * only (the PVR handles x/y off screen within its guard band).
 *
 * Decal layers: the arcade paints coplanar faces in order; a z-buffer needs them apart, so a layer-n face's 1/z is
 * raised by n * 2^-12 of itself (Daytona's port uses a step of the same size). */
#include "render/course_render.h"
#include "sr_alloc.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#ifdef _arch_dreamcast
#include <dc/matrix.h>
#include <dc/perfctr.h>
#include <dc/pvr.h>
#include "render/ta.h"
/* CPU cycles, the low word of PRFC0 (it runs in elapsed-time mode for the KOS timer): read in place, as
 * perf_cntr_count's call per chunk cost about 1% of the frame; differences are taken modulo 2^32 */
#define CYCLES() (*(volatile uint32_t *)0xff100008u)
/* The frame's hot path (render_instance, render_chunk, the course's and the cars' triangle loops) in one section:
 * contiguous, so it fits the SH-4's 8 KB direct-mapped instruction cache without evicting itself (laid out at the
 * linker's whim, the triangle loop and render_chunk once shared cache lines: +2 ms a frame in I-cache misses). */
#define SR_HOT __attribute__((section(".text.sr_hot")))
#else
#define CYCLES() 0
#define SR_HOT
#endif
#ifdef SR_RENDER_PROF
#define FREEZE() (*(volatile uint32_t *)0xff100010u) /* PRFC1's low word */
#define PROF_BEGIN() const uint32_t prof_t0 = CYCLES()
#define PROF_END(field) stats->field += (uint32_t)(CYCLES() - prof_t0)
#else
#define FREEZE() 0
#define PROF_BEGIN() (void)0
#define PROF_END(field) (void)0
#endif

#define CHUNK_CELL 96.f  /* world units: chunk cell size (larger cells: fewer, bigger chunks, less per-chunk work) */
#define CHUNK_SPLIT 64.f /* meshes with a larger bounding radius are split */
#define LAYER_STEP (1.f / 4096.f)

/* ---- load: chunks, shared vertices, batches ---- */

static uint32_t vertex_hash(const sr_course_vertex_t *v) {
    uint32_t w[6], h = 2166136261u, i;
    sr_copy(w, v, 24);
    for(i = 0; i < 6; i++) h = (h ^ w[i]) * 16777619u;
    return h;
}

static uint32_t tri_key(const sr_course_tri_t *t) {
    /* list, then texture, then layer: the batch order */
    return ((uint32_t)(t->flags & SR_TRI_CUTOUT) << 31) | ((uint32_t)t->texture << 8) |
           ((t->flags & SR_TRI_LAYER_MASK) >> SR_TRI_LAYER_SHIFT);
}

static void sort_by(uint32_t *order, uint32_t n, const uint32_t *key) {
    uint32_t t;
    for(t = 1; t < n; t++) { /* insertion sort: stable, input mostly ordered */
        const uint32_t o = order[t];
        uint32_t j = t;
        while(j > 0 && key[order[j - 1]] > key[o]) {
            order[j] = order[j - 1];
            j--;
        }
        order[j] = o;
    }
}

/* Move a built chunk's arrays into one block, in the order a frame reads them: batches, positions (the transform),
 * then indices, quad pairs, position indices, vertices and source triangles (the triangle loop). Contiguous, a
 * chunk's data takes the fewest cache lines and one allocation. */
static int pack_chunk(sr_course_chunk_t *c) {
#define PART(n) (((size_t)(n) + 3u) & ~(size_t)3u)
    const size_t b_bat = PART(c->n_batches * sizeof(*c->batches)), b_pos = PART(c->n_pos * 3u * sizeof(float));
    const size_t b_idx = PART(c->n_tris * 3u * sizeof(uint16_t)), b_pair = PART(c->n_tris ? c->n_tris : 1u);
    const size_t b_vpos = PART(c->n_verts * sizeof(uint16_t)), b_verts = PART(c->n_verts * sizeof(*c->verts));
    const size_t b_src = PART(c->n_tris * sizeof(uint16_t));
#undef PART
    const size_t total = b_bat + b_pos + b_idx + b_pair + b_vpos + b_verts + b_src;
    uint8_t *block = sr_alloc(total), *p = block;
    if(!block) return -1;
#define MOVE(field, bytes, count)                                                                                  \
    do {                                                                                                           \
        sr_copy(p, c->field, (count));                                                                             \
        free(c->field);                                                                                            \
        c->field = (void *)p;                                                                                      \
        p += (bytes);                                                                                              \
    } while(0)
    MOVE(batches, b_bat, c->n_batches * sizeof(*c->batches));
    MOVE(pos, b_pos, c->n_pos * 3u * sizeof(float));
    MOVE(index, b_idx, c->n_tris * 3u * sizeof(uint16_t));
    MOVE(pair, b_pair, c->n_tris ? c->n_tris : 1u);
    MOVE(vpos, b_vpos, c->n_verts * sizeof(uint16_t));
    MOVE(verts, b_verts, c->n_verts * sizeof(*c->verts));
    MOVE(src_tri, b_src, c->n_tris * sizeof(uint16_t));
#undef MOVE
    return 0;
}

static int build_chunk(sr_course_chunk_t *c, const sr_course_tri_t *src, uint32_t *tris, uint32_t n_tris) {
    const uint32_t table_size = 8192;
    int32_t *table = sr_alloc(table_size * sizeof(*table));
    uint32_t *key = NULL, t, k, n = 0, cap_b = 4, max = 0;
    float lo[3] = {1e30f, 1e30f, 1e30f}, hi[3] = {-1e30f, -1e30f, -1e30f}, r2 = 0.f;
    c->verts = sr_alloc(n_tris * 3 * sizeof(*c->verts));
    c->index = sr_alloc(n_tris * 3 * sizeof(*c->index));
    c->batches = sr_alloc(cap_b * sizeof(*c->batches));
    if(!table || !c->verts || !c->index || !c->batches) return -1;

    /* Batch order within the chunk. */
    for(t = 0; t < n_tris; t++)
        if(tris[t] > max) max = tris[t];
    key = sr_alloc((max + 1) * sizeof(*key));
    if(!key) return -1;
    for(t = 0; t < n_tris; t++) key[tris[t]] = tri_key(&src[tris[t]]);
    sort_by(tris, n_tris, key);

    sr_fill(table, 0xff, table_size * sizeof(*table));
    c->n_batches = 0;
    c->lists = 0;
    for(t = 0; t < n_tris; t++) {
        const sr_course_tri_t *tri = &src[tris[t]];
        if(c->n_batches == 0 || key[tris[t]] != key[tris[t - 1]]) {
            if(c->n_batches == cap_b) c->batches = realloc(c->batches, (cap_b *= 2) * sizeof(*c->batches));
            if(!c->batches) return -1;
            c->batches[c->n_batches].texture = tri->texture;
            c->batches[c->n_batches].flags = tri->flags;
            c->batches[c->n_batches].first = t;
            c->batches[c->n_batches].count = 0;
            c->n_batches++;
            c->lists |= tri->flags & SR_TRI_CUTOUT ? 2u : 1u;
        }
        c->batches[c->n_batches - 1].count++;
        for(k = 0; k < 3; k++) {
            const sr_course_vertex_t *v = &tri->v[k];
            uint32_t slot = vertex_hash(v) & (table_size - 1);
            while(table[slot] >= 0 && memcmp(&c->verts[table[slot]], v, sizeof(*v))) slot = (slot + 1) & (table_size - 1);
            if(table[slot] < 0) {
                if(n >= table_size / 2) { /* table full: stop sharing for the rest */
                    c->verts[n] = *v;
                    c->index[t * 3 + k] = (uint16_t)n++;
                    continue;
                }
                table[slot] = (int32_t)n;
                c->verts[n++] = *v;
            }
            c->index[t * 3 + k] = (uint16_t)table[slot];
        }
    }
    free(table);
    free(key);
    c->n_verts = n;
    c->n_tris = n_tris;
    /* distinct positions: the same table again, keyed by x, y, z */
    c->pos = sr_alloc(n * 3 * sizeof(*c->pos));
    c->vpos = sr_alloc(n * sizeof(*c->vpos));
    table = sr_alloc(table_size * sizeof(*table));
    if(!c->pos || !c->vpos || !table) return -1;
    sr_fill(table, 0xff, table_size * sizeof(*table));
    c->n_pos = 0;
    for(k = 0; k < n; k++) {
        const sr_course_vertex_t *v = &c->verts[k];
        uint32_t w[3], h = 2166136261u, slot, j;
        sr_copy(w, &v->x, 12);
        for(j = 0; j < 3; j++) h = (h ^ w[j]) * 16777619u;
        slot = h & (table_size - 1);
        while(table[slot] >= 0 && memcmp(&c->pos[table[slot] * 3], &v->x, 12)) slot = (slot + 1) & (table_size - 1);
        if(table[slot] < 0) {
            table[slot] = (int32_t)c->n_pos;
            sr_copy(&c->pos[c->n_pos * 3], &v->x, 12);
            c->n_pos++;
        }
        c->vpos[k] = (uint16_t)table[slot];
    }
    free(table);
    /* Fanned quads within a batch: (a,b,c) then (a,c,d). */
    c->src_tri = sr_alloc(n_tris * sizeof(*c->src_tri));
    if(!c->src_tri) return -1;
    for(t = 0; t < n_tris; t++) c->src_tri[t] = (uint16_t)tris[t];
    c->pair = sr_alloc_zero(n_tris ? n_tris : 1);
    if(!c->pair) return -1;
    {
        uint32_t bi;
        for(bi = 0; bi < c->n_batches; bi++) {
            const sr_course_batch_t *bt = &c->batches[bi];
            for(t = bt->first; t + 1 < bt->first + bt->count; t++) {
                const uint16_t *p0 = &c->index[t * 3], *p1 = &c->index[t * 3 + 3];
                if(p1[0] == p0[0] && p1[1] == p0[2]) {
                    c->pair[t] = 1;
                    t++;
                }
            }
        }
    }
    for(k = 0; k < n; k++) {
        int a;
        const float *p = &c->verts[k].x;
        c->verts[k].argb |= 0xff000000u; /* opaque vertex colour */
        for(a = 0; a < 3; a++) {
            if(p[a] < lo[a]) lo[a] = p[a];
            if(p[a] > hi[a]) hi[a] = p[a];
        }
    }
    for(k = 0; k < 3; k++) c->bounds[k] = 0.5f * (lo[k] + hi[k]);
    for(k = 0; k < n; k++) {
        const float dx = c->verts[k].x - c->bounds[0], dy = c->verts[k].y - c->bounds[1], dz = c->verts[k].z - c->bounds[2];
        const float d = dx * dx + dy * dy + dz * dz;
        if(d > r2) r2 = d;
    }
    c->bounds[3] = sqrtf(r2);
    return pack_chunk(c);
}

/* Cut a mesh into chunks: by an XZ cell grid of the triangles' centroids when it is large, else one chunk. A chunk
 * keeps at most 2048 triangles (16-bit indices with room to spare). */
static int build_chunks(sr_course_mesh_t *m) {
    const uint32_t max_tris = 2048;
    uint32_t *order = sr_alloc(m->n_tris * sizeof(*order)), *cell = sr_alloc(m->n_tris * sizeof(*cell));
    uint32_t t, start, cap = 8;
    if(!order || !cell) return -1;
    for(t = 0; t < m->n_tris; t++) {
        const sr_course_vertex_t *v = m->tris[t].v;
        const float cx = (v[0].x + v[1].x + v[2].x) / 3.f, cz = (v[0].z + v[1].z + v[2].z) / 3.f;
        order[t] = t;
        cell[t] = m->bounds[3] > CHUNK_SPLIT
                      ? ((uint32_t)(int32_t)floorf(cx / CHUNK_CELL) & 0xffffu) << 16 |
                            ((uint32_t)(int32_t)floorf(cz / CHUNK_CELL) & 0xffffu)
                      : 0;
    }
    sort_by(order, m->n_tris, cell);
    m->n_chunks = 0;
    m->chunks = sr_alloc(cap * sizeof(*m->chunks));
    for(start = 0; start < m->n_tris;) {
        uint32_t end = start + 1;
        while(end < m->n_tris && cell[order[end]] == cell[order[start]] && end - start < max_tris) end++;
        if(m->n_chunks == cap) m->chunks = realloc(m->chunks, (cap *= 2) * sizeof(*m->chunks));
        if(!m->chunks || build_chunk(&m->chunks[m->n_chunks++], m->tris, order + start, end - start)) return -1;
        start = end;
    }
    free(order);
    free(cell);
    return 0;
}

int sr_course_parse(sr_course_t *course, const uint8_t *data, size_t size) {
    size_t off = 20;
    uint32_t i;
    if(size < 20 || memcmp(data, SR_COURSE_MAGIC, 8)) return -1;
    sr_copy(&course->n_textures, data + 8, 4);
    sr_copy(&course->n_meshes, data + 12, 4);
    sr_copy(&course->n_instances, data + 16, 4);
    course->textures = sr_alloc_zero((course->n_textures ? course->n_textures : 1) * sizeof(*course->textures));
    course->meshes = sr_alloc_zero((course->n_meshes ? course->n_meshes : 1) * sizeof(*course->meshes));
    course->instances = sr_alloc((course->n_instances ? course->n_instances : 1) * sizeof(*course->instances));
    if(!course->textures || !course->meshes || !course->instances) return -1;
    for(i = 0; i < course->n_textures; i++) {
        sr_course_texture_t *t = &course->textures[i];
        if(off + 8 > size) return -1;
        sr_copy(&t->w, data + off, 2);
        sr_copy(&t->h, data + off + 2, 2);
        sr_copy(&t->flags, data + off + 4, 4);
        t->texels = (const uint16_t *)(data + off + 8);
        off += 8 + (size_t)sr_course_texture_bytes(t->w, t->h, t->flags);
        off = (off + 3) & ~(size_t)3;
        if(off > size) return -1;
    }
    for(i = 0; i < course->n_meshes; i++) {
        sr_course_mesh_t *m = &course->meshes[i];
        if(off + 28 > size) return -1;
        sr_copy(&m->oba, data + off, 4);
        sr_copy(&m->obc, data + off + 4, 4);
        sr_copy(&m->n_tris, data + off + 8, 4);
        sr_copy(m->bounds, data + off + 12, 16);
        off += 28;
        m->tris = (const sr_course_tri_t *)(data + off);
        off += (size_t)m->n_tris * sizeof(sr_course_tri_t);
        if(off > size || build_chunks(m)) return -1;
    }
    if(off + (size_t)course->n_instances * sizeof(sr_course_instance_t) > size) return -1;
    sr_copy(course->instances, data + off, (size_t)course->n_instances * sizeof(sr_course_instance_t));
    course->instance_bounds = sr_alloc((course->n_instances ? course->n_instances : 1) * sizeof(*course->instance_bounds));
    if(!course->instance_bounds) return -1;
    for(i = 0; i < course->n_instances; i++) {
        const sr_course_instance_t *in = &course->instances[i];
        const float *b = in->mesh < course->n_meshes ? course->meshes[in->mesh].bounds : (const float[4]){0, 0, 0, 0};
        float *w = course->instance_bounds[i], s2 = 0.f;
        uint32_t k;
        for(k = 0; k < 3; k++) {
            const float l = in->m[k * 3] * in->m[k * 3] + in->m[k * 3 + 1] * in->m[k * 3 + 1] + in->m[k * 3 + 2] * in->m[k * 3 + 2];
            if(l > s2) s2 = l;
            w[k] = in->m[k] * b[0] + in->m[3 + k] * b[1] + in->m[6 + k] * b[2] + in->m[9 + k];
        }
        w[3] = b[3] * sqrtf(s2);
    }
    return 0;
}

/* ---- per frame ---- */

static uint32_t s_argb; /* SR_LIST_TRANSLUCENT: every vertex's colour (view->argb); 0: the pack's */

int sr_render_cull = -1; /* the GEO's front faces are clockwise on screen (y down) */

/* The triangle faces away (its screen winding is the one culled). */
static inline int back_facing(const sr_screen_vertex_t *a, const sr_screen_vertex_t *b, const sr_screen_vertex_t *c,
                              float sign) {
    return ((b->x - a->x) * (c->y - a->y) - (b->y - a->y) * (c->x - a->x)) * sign > 0.f;
}

SR_HOT void sr_m2_compose(float out[12], const float a[12], const float b[12]) {
    float r[12];
    int c;
    for(c = 0; c < 4; c++) {
        const float x = b[c * 3], y = b[c * 3 + 1], z = b[c * 3 + 2];
        r[c * 3] = a[0] * x + a[3] * y + a[6] * z;
        r[c * 3 + 1] = a[1] * x + a[4] * y + a[7] * z;
        r[c * 3 + 2] = a[2] * x + a[5] * y + a[8] * z;
    }
    r[9] += a[9];
    r[10] += a[10];
    r[11] += a[11];
    for(c = 0; c < 12; c++) out[c] = r[c]; /* out may be a or b; 12 moves, not a general copy's call */
}

static void xform(const float m[12], float x, float y, float z, float out[3]) {
    out[0] = m[0] * x + m[3] * y + m[6] * z + m[9];
    out[1] = m[1] * x + m[4] * y + m[7] * z + m[10];
    out[2] = m[2] * x + m[5] * y + m[8] * z + m[11];
}

/* Bounding sphere against the view frustum; scale is the matrix's longest column. */
static SR_HOT int sphere_visible(const float mv[12], const float bounds[4], float scale, const sr_view_t *view) {
    float c[3];
    const float r = bounds[3] * scale;
    xform(mv, bounds[0], bounds[1], bounds[2], c);
    if(c[2] + r < view->znear) return 0;
    if(c[0] * view->focal_x - r * view->focal_x > (view->width - view->cx) * (c[2] + r)) return 0;
    if(-c[0] * view->focal_x - r * view->focal_x > view->cx * (c[2] + r)) return 0;
    if(c[1] * view->focal_y - r * view->focal_y > view->cy * (c[2] + r)) return 0;
    if(-c[1] * view->focal_y - r * view->focal_y > (view->height - view->cy) * (c[2] + r)) return 0;
    return 1;
}

typedef struct {
    float p[3], u, v;
    uint32_t argb;
} clip_vertex_t;

static uint32_t lerp_argb(uint32_t a, uint32_t b, float t) {
    uint32_t out = 0;
    int sh;
    for(sh = 0; sh < 32; sh += 8) {
        const float ca = (float)((a >> sh) & 255), cb = (float)((b >> sh) & 255);
        out |= (uint32_t)(ca + (cb - ca) * t + 0.5f) << sh;
    }
    return out;
}

static void project(const sr_view_t *view, const clip_vertex_t *c, sr_screen_vertex_t *out) {
    const float invz = 1.f / c->p[2];
    out->x = view->cx + c->p[0] * view->focal_x * invz;
    out->y = view->cy - c->p[1] * view->focal_y * invz; /* Model 2 view space: x right, y up, z forward */
    out->invz = invz;
    out->z = c->p[2];
    out->u = c->u;
    out->v = c->v;
    out->argb = c->argb;
}

/* ---- lighting (the cars): the GEO's polygon luma per frame ---- */

const float (*sr_light_tables)[17];
float sr_light_view[3];

/* The instance being drawn when its mesh is lit (render_instance), else NULL. */
static const sr_tri_light_t *s_lit;

/* dotl = N.L, dotp = N.P (only its sign is used), nz = N's z, all in view space */
static inline float light_k(const sr_tri_light_t *l, float dotl, float dotp, float nz) {
    const float *L = sr_light_view;
    const float *k = sr_light_tables[l->table];
    float lum = dotl * dotp < 0.f ? 0.f : (dotl < 0.f ? -dotl : dotl), f;
    int i;
    lum = lum * l->diffuse + l->ambient;
    if(l->spec_mode) {
        float spec = 2.f * dotl * nz - L[2];
        if(spec < 0.f || l->spec_ctrl == 0) spec = 0.f;
        if(l->spec_ctrl >> 1) spec *= spec;
        if(l->spec_ctrl >> 2) spec *= spec;
        if((l->spec_ctrl + 1) >> 3) spec *= spec;
        lum += spec * l->spec_scale;
    }
    if(lum < 0.f) lum = 0.f;
    if(lum > 255.f) lum = 255.f;
    f = lum * (1.f / 16.f);
    i = (int)f;
    if(i > 15) i = 15;
    return k[i] + (k[i + 1] - k[i]) * (f - (float)i);
}

float sr_light_factor(const sr_tri_light_t *l, const float n[3], const float p[3]) {
    const float *L = sr_light_view;
    return light_k(l, n[0] * L[0] + n[1] * L[1] + n[2] * L[2], n[0] * p[0] + n[1] * p[1] + n[2] * p[2], n[2]);
}

/* The lit instance's view terms in its mesh's space, for a mesh normal n: N.L = n . s_lit_l, N's view z =
 * n . s_lit_z, and N.P has the sign of n . (corner + s_lit_g) (the model-view is a rotation with a uniform scale
 * s: N.P = s^2 (n . corner + n . M^T t / s^2)). Set by render_instance. */
static float s_lit_l[3], s_lit_z[3], s_lit_g[3];

static void lit_instance(const float mv[12]) {
    const float *L = sr_light_view;
    const float s2 = mv[0] * mv[0] + mv[1] * mv[1] + mv[2] * mv[2], inv = s2 > 0.f ? 1.f / s2 : 0.f;
    int j;
    for(j = 0; j < 3; j++) {
        const float *col = &mv[j * 3];
        s_lit_l[j] = col[0] * L[0] + col[1] * L[1] + col[2] * L[2];
        s_lit_z[j] = col[2];
        s_lit_g[j] = (col[0] * mv[9] + col[1] * mv[10] + col[2] * mv[11]) * inv;
    }
}

/* Triangle l's luma factor under the lit instance (s_lit_*), x 256 */
static inline uint32_t lit_scale(const sr_tri_light_t *l) {
    const float *n = l->n;
    const float dotl = n[0] * s_lit_l[0] + n[1] * s_lit_l[1] + n[2] * s_lit_l[2];
    const float dotp = l->d + n[0] * s_lit_g[0] + n[1] * s_lit_g[1] + n[2] * s_lit_g[2];
    const float k = light_k(l, dotl, dotp, n[0] * s_lit_z[0] + n[1] * s_lit_z[1] + n[2] * s_lit_z[2]) * 256.f;
    return k >= 1024.f ? 1024u : (uint32_t)k;
}

static inline uint32_t scale_argb_ki(uint32_t argb, uint32_t ki) {
    const uint32_t r = (((argb >> 16) & 255u) * ki) >> 8, g = (((argb >> 8) & 255u) * ki) >> 8, b = ((argb & 255u) * ki) >> 8;
    return (argb & 0xff000000u) | (r > 255u ? 255u : r) << 16 | (g > 255u ? 255u : g) << 8 | (b > 255u ? 255u : b);
}


#ifdef _arch_dreamcast
/* Position from the projected vertex; texture coordinates and colour from the pack's (src), or from the projected
 * vertex when src is NULL (near-plane clips make new ones). */
static inline void put_vertex(pvr_vertex_t *d, const sr_screen_vertex_t *s, const sr_course_vertex_t *src,
                              uint32_t flags, float bias) {
    d->flags = flags;
    d->x = s->x;
    d->y = s->y;
    d->z = s->invz * bias;
    d->u = src ? src->u : s->u;
    d->v = src ? src->v : s->v;
    d->argb = s_argb ? s_argb : src ? src->argb : s->argb;
    d->oargb = 0;
}

/* put_vertex for the batch loop: the pack's vertex always given, the vertex's colour passed in. */
static inline __attribute__((always_inline)) void put_vertex_fast(pvr_vertex_t *d, const sr_screen_vertex_t *s,
                                                                  const sr_course_vertex_t *src, uint32_t flags,
                                                                  float bias, uint32_t argb) {
    d->flags = flags;
    d->x = s->x;
    d->y = s->y;
    d->z = s->invz * bias;
    d->u = src->u;
    d->v = src->v;
    d->argb = argb;
    d->oargb = 0;
}

/* One triangle (n = 3) or strip of two (n = 4): to the TA through the store queues, or to the deferred list. */
static inline void emit_dc(sr_deferred_t *defer, const sr_screen_vertex_t *const *v,
                           const sr_course_vertex_t *const *src, int n, float bias) {
    int k;
    if(defer) {
        pvr_vertex_t *d = sr_deferred_alloc(defer, (uint32_t)n * sizeof(pvr_vertex_t));
        if(!d) return;
        for(k = 0; k < n; k++)
            put_vertex(&d[k], v[k], src ? src[k] : 0, k == n - 1 ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX, bias);
        return;
    }
#ifdef SR_RENDER_NULL
    /* RENDER_NULL=1: everything but the TA writes (timing the CPU side) */
    {
        static pvr_vertex_t sink[4] __attribute__((aligned(32)));
        for(k = 0; k < n; k++)
            put_vertex(&sink[k], v[k], src ? src[k] : 0, k == n - 1 ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX, bias);
    }
#else
    for(k = 0; k < n; k++) {
        pvr_vertex_t *pv = sr_ta_target();
        put_vertex(pv, v[k], src ? src[k] : 0, k == n - 1 ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX, bias);
        sr_ta_commit(pv);
    }
#endif
}
#endif

static inline void emit(const sr_view_t *view, sr_deferred_t *defer, const sr_screen_vertex_t *a, const sr_screen_vertex_t *b,
                 const sr_screen_vertex_t *c, float bias) {
#ifdef _arch_dreamcast
    if(!view->tri) {
        const sr_screen_vertex_t *v[3] = {a, b, c};
        emit_dc(defer, v, 0, 3, bias);
        return;
    }
#endif
    (void)defer;
    {
        sr_screen_vertex_t t[3] = {*a, *b, *c};
        int k;
        for(k = 0; k < 3; k++) {
            t[k].invz *= bias;
            if(s_argb) t[k].argb = s_argb;
        }
        view->tri(view->user, t);
    }
}

/* A triangle crossing the near plane: keep the part in front (one or two triangles). */
static void clip_near(const sr_view_t *view, sr_deferred_t *defer, const clip_vertex_t in[3], float bias) {
    clip_vertex_t poly[4];
    sr_screen_vertex_t s[4];
    int n = 0, i;
    for(i = 0; i < 3; i++) {
        const clip_vertex_t *a = &in[i], *b = &in[(i + 1) % 3];
        const int ain = a->p[2] >= view->znear, bin = b->p[2] >= view->znear;
        if(ain) poly[n++] = *a;
        if(ain != bin) {
            const float t = (view->znear - a->p[2]) / (b->p[2] - a->p[2]);
            clip_vertex_t *c = &poly[n++];
            c->p[0] = a->p[0] + (b->p[0] - a->p[0]) * t;
            c->p[1] = a->p[1] + (b->p[1] - a->p[1]) * t;
            c->p[2] = view->znear;
            c->u = a->u + (b->u - a->u) * t;
            c->v = a->v + (b->v - a->v) * t;
            c->argb = lerp_argb(a->argb, b->argb, t);
        }
    }
    if(n < 3) return;
    for(i = 0; i < n; i++) project(view, &poly[i], &s[i]);
    emit(view, defer, &s[0], &s[1], &s[2], bias);
    if(n == 4) emit(view, defer, &s[0], &s[2], &s[3], bias);
}

/* Scratch for one chunk's transformed vertices, one 32-byte cache line each. */
#define MAX_CHUNK_VERTS 6144
static sr_screen_vertex_t s_scratch[MAX_CHUNK_VERTS] __attribute__((aligned(32)));

#ifdef _arch_dreamcast
/* 1 / sqrt(x), the SH-4's approximation (about 2^-21 relative) */
static inline float fast_rsqrt(float x) {
    __asm__("fsrra %0" : "+f"(x));
    return x;
}

/* Projection and view x world in XMTRX: ftrv gives (fx x + cx z, -fy y + cy z, z, z) in view terms. */
static void load_projection(const float mv[12], const sr_view_t *view) {
    matrix_t m;
    float row[4][4];
    int r, c;
    for(c = 0; c < 4; c++) {
        const float vx = mv[c * 3], vy = mv[c * 3 + 1], vz = mv[c * 3 + 2];
        row[0][c] = view->focal_x * vx + view->cx * vz;
        row[1][c] = -view->focal_y * vy + view->cy * vz;
        row[2][c] = vz;
        row[3][c] = vz;
    }
    for(r = 0; r < 4; r++)
        for(c = 0; c < 4; c++) m[c][r] = row[r][c];
    mat_load(&m);
}
#endif

/* A triangle crossing the near plane (a few a frame): back to view space for the clip. */
static void clip_tri(const sr_course_chunk_t *c, uint32_t i, const float mv[12], const sr_view_t *view,
                     sr_deferred_t *defer, float bias, sr_render_stats_t *stats) {
    clip_vertex_t t[3];
    int k;
    for(k = 0; k < 3; k++) {
        const sr_course_vertex_t *src = &c->verts[c->index[i * 3 + k]];
        xform(mv, src->x, src->y, src->z, t[k].p);
        t[k].u = src->u;
        t[k].v = src->v;
        t[k].argb = src->argb;
    }
    stats->tris_clipped++;
    clip_near(view, defer, t, bias);
}

#ifdef _arch_dreamcast
/* The Dreamcast's triangle loop for one batch, kept small so it runs from registers: cull back faces, send fanned
 * quads as strips and the rest as triangles, to the TA (deferred NULL) or the deferred list. Triangles crossing the
 * near plane are listed in clip[] for the caller's slow path. Returns the triangles drawn. */
#define CLIP_MAX 256
/* Where batch_dc's vertex colours come from (a constant in each instance of it): */
#define COL_PACK 0  /* the pack's */
#define COL_FLAT 1  /* s_argb, the translucent pass's */
#define COL_LIT 2   /* the pack's under the GEO's lighting this frame (lit_scale) */

static inline __attribute__((always_inline)) uint32_t batch_dc(const sr_course_chunk_t *c, uint32_t first,
                                                               uint32_t end, float bias, float cull,
                                                               sr_deferred_t *defer, uint32_t *culled,
                                                               uint32_t *clip, uint32_t *n_clip, int colours) {
    const uint16_t *index = c->index;
    const uint8_t *pair = c->pair;
    const sr_course_vertex_t *verts = c->verts;
    const uint16_t *vpos = c->vpos;
    const uint32_t argb = s_argb; /* the translucent pass's colour: read once, not per vertex */
    uint32_t i, drawn = 0;
#ifndef SR_RENDER_NULL
    uint32_t sq = sr_ta_sq; /* the store queue address (render/ta.h): kept in a register */
#endif
    for(i = first; i < end; i++) {
        const uint16_t *ix = &index[i * 3];
        const sr_screen_vertex_t *a = &s_scratch[vpos[ix[0]]], *b = &s_scratch[vpos[ix[1]]], *d = &s_scratch[vpos[ix[2]]];
        const uint32_t p = pair[i];
        const sr_screen_vertex_t *v[4];
        const sr_course_vertex_t *src[4];
        uint32_t n = 3, k, col[4];
        pvr_vertex_t *out;
        if(a->behind | b->behind | d->behind) {
            if(!(a->behind & b->behind & d->behind) && *n_clip < CLIP_MAX) clip[(*n_clip)++] = i;
            continue;
        }
        if(cull != 0.f && back_facing(a, b, d, cull)) {
            *culled += 1u + p; /* a fanned quad is planar: both halves face the same way */
            i += p;
            continue;
        }
        if(p && !s_scratch[vpos[ix[5]]].behind) {
            /* (a,b,d),(a,d,e) as the strip b,a,d,e */
            v[0] = b, v[1] = a, v[2] = d, v[3] = &s_scratch[vpos[ix[5]]];
            src[0] = &verts[ix[1]], src[1] = &verts[ix[0]], src[2] = &verts[ix[2]], src[3] = &verts[ix[5]];
            n = 4;
            i++;
        } else {
            v[0] = a, v[1] = b, v[2] = d;
            src[0] = &verts[ix[0]], src[1] = &verts[ix[1]], src[2] = &verts[ix[2]];
        }
        drawn += n - 2;
        if(colours == COL_LIT) {
            /* only the triangles drawn are lit; a quad's halves are one GEO polygon */
            const uint32_t ki = lit_scale(&s_lit[c->src_tri[i - (n - 3)]]);
            col[0] = scale_argb_ki(src[0]->argb, ki);
            for(k = 1; k < n; k++) col[k] = src[k]->argb == src[k - 1]->argb ? col[k - 1] : scale_argb_ki(src[k]->argb, ki);
        } else if(colours == COL_FLAT)
            for(k = 0; k < n; k++) col[k] = argb;
        if(defer) {
            out = sr_deferred_alloc(defer, n * sizeof(pvr_vertex_t));
            if(!out) continue;
            for(k = 0; k < n; k++) {
                /* allocate the line without reading it from RAM: every byte of it is written next */
                __asm__ volatile("movca.l r0,@%1" : "=m"(out[k].flags) : "r"(&out[k]));
                put_vertex_fast(&out[k], v[k], src[k], k == n - 1 ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX, bias,
                                colours == COL_PACK ? src[k]->argb : col[k]);
            }
            continue;
        }
#ifdef SR_RENDER_NULL
        {
            static pvr_vertex_t sink[4] __attribute__((aligned(32)));
            for(k = 0; k < n; k++)
                put_vertex_fast(&sink[k], v[k], src[k], k == n - 1 ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX, bias,
                                colours == COL_PACK ? src[k]->argb : col[k]);
        }
#else
        for(k = 0; k < n; k++) {
            out = (pvr_vertex_t *)sq;
            sq += 32u;
            put_vertex_fast(out, v[k], src[k], k == n - 1 ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX, bias,
                                colours == COL_PACK ? src[k]->argb : col[k]);
            /* send this store queue: the asm reads the vertex, so its stores come first; no "memory" clobber (KOS's
             * sq_flush has one, and GCC then reloads every value the loop keeps after each vertex) */
            __asm__ volatile("pref @%1" : : "m"(*out), "r"(out));
        }
#endif
    }
#ifndef SR_RENDER_NULL
    sr_ta_sq = sq;
#endif
    return drawn;
}

/* batch_dc's instances: to the TA (defer NULL) or the deferred list, by where the colours come from */
#define BATCH_DC_ARGS                                                                                              \
    const sr_course_chunk_t *c, uint32_t first, uint32_t end, float bias, float cull, sr_deferred_t *defer,       \
        uint32_t *culled, uint32_t *clip, uint32_t *n_clip
#define BATCH_DC(name, colours, attr)                                                                              \
    static __attribute__((noinline)) attr uint32_t name(BATCH_DC_ARGS) {                                                \
        return defer ? batch_dc(c, first, end, bias, cull, defer, culled, clip, n_clip, colours)                   \
                     : batch_dc(c, first, end, bias, cull, NULL, culled, clip, n_clip, colours);                   \
    }
BATCH_DC(batch_dc_pack, COL_PACK, SR_HOT)
BATCH_DC(batch_dc_flat, COL_FLAT, ) /* the shadows' (the host draws them) */
BATCH_DC(batch_dc_lit, COL_LIT, SR_HOT)
#endif

#ifdef _arch_dreamcast
/* The batch's header from view->headers (see sr_view_t): to the TA through the store queue like a vertex, or for
 * the cut-out list in SR_LIST_BOTH to the deferred list. */
static inline void material_dc(const sr_view_t *view, uint16_t texture, uint16_t flags) {
    const uint32_t ix = texture < view->n_headers ? texture : view->n_headers;
    const int list = view->list == SR_LIST_BOTH ? ((flags & SR_TRI_CUTOUT) ? SR_LIST_CUTOUT : SR_LIST_OPAQUE) : view->list;
    const uint32_t *h = (const uint32_t *)view->headers[list] + ix * 8u;
    uint32_t *d;
    if(ix == view->current[list]) return;
    view->current[list] = (uint16_t)ix;
    if(view->list == SR_LIST_BOTH && list == SR_LIST_CUTOUT) {
        d = sr_deferred_alloc(view->deferred, 32u);
        if(!d) {
            view->current[list] = 0xffff;
            return;
        }
        d[0] = h[0], d[1] = h[1], d[2] = h[2], d[3] = h[3], d[4] = h[4], d[5] = h[5], d[6] = h[6], d[7] = h[7];
        return;
    } else {
#ifdef SR_RENDER_NULL
        return;
#else
        d = sr_ta_target();
#endif
    }
    d[0] = h[0], d[1] = h[1], d[2] = h[2], d[3] = h[3], d[4] = h[4], d[5] = h[5], d[6] = h[6], d[7] = h[7];
    __asm__ volatile("pref @%1" : : "m"(*(const pvr_poly_hdr_t *)d), "r"(d)); /* reads all 32 bytes: stores first */
}
#endif

/* The triangle loop through view->tri (the host's previews and tests), one triangle at a time. */
static __attribute__((noinline)) uint32_t batch_generic(const sr_course_chunk_t *c, uint32_t first, uint32_t end,
                                                        const float mv[12], const sr_view_t *view, sr_deferred_t *defer,
                                                        float bias, float cull, sr_render_stats_t *stats,
                                                        uint32_t *culled) {
    uint32_t i, drawn = 0;
    for(i = first; i < end; i++) {
        const uint16_t *ix = &c->index[i * 3];
        const sr_screen_vertex_t *a = &s_scratch[ix[0]], *b2 = &s_scratch[ix[1]], *d = &s_scratch[ix[2]];
        const uint32_t behind = a->behind + b2->behind + d->behind;
        if(!behind) {
            if(cull != 0.f && back_facing(a, b2, d, cull)) {
                *culled += 1u + c->pair[i];
                i += c->pair[i];
                continue;
            }
            drawn++;
            if(s_lit) { /* a lit mesh: the same luma as the Dreamcast's batch_dc, on copies */
                const uint32_t ki = lit_scale(&s_lit[c->src_tri[i]]);
                sr_screen_vertex_t la = *a, lb = *b2, ld = *d;
                la.argb = scale_argb_ki(la.argb, ki), lb.argb = scale_argb_ki(lb.argb, ki), ld.argb = scale_argb_ki(ld.argb, ki);
                emit(view, defer, &la, &lb, &ld, bias);
            } else
                emit(view, defer, a, b2, d, bias);
        } else if(behind < 3) {
            clip_tri(c, i, mv, view, defer, bias, stats);
        }
    }
    return drawn;
}

static SR_HOT void render_chunk(const sr_course_chunk_t *c, const float mv[12], const sr_view_t *view,
                                sr_render_stats_t *stats) {
    const uint32_t want = view->list == SR_LIST_TRANSLUCENT ? 3u
                          : view->list == SR_LIST_BOTH  ? 3u
                          : view->list == SR_LIST_CUTOUT ? 2u
                                                         : 1u;
    const float znear = view->znear;
    uint32_t i, b, drawn = 0, culled = 0;
    uint32_t t0, t1, f0, f1;
    if(!(c->lists & want) || c->n_verts > MAX_CHUNK_VERTS) return;
    f0 = FREEZE();
    t0 = CYCLES();
    stats->chunks_drawn++;
#ifdef _arch_dreamcast
    stats->verts_transformed += c->n_pos;
    /* Two vertices a step (FTRV into fv8 and fv12) so one's FTRV, square and FSRRA overlap the other's: one at a
     * time the FPU waits on each result. Prefetch a few cache lines ahead: the vertices stream from RAM. */
    for(i = 0; i + 1 < c->n_pos; i += 2) {
        const float *s0 = &c->pos[i * 3], *s1 = s0 + 3;
        sr_screen_vertex_t *v0 = &s_scratch[i], *v1 = v0 + 1;
        register float x0 __asm__("fr8") = s0[0], y0 __asm__("fr9") = s0[1], z0 __asm__("fr10") = s0[2];
        register float w0 __asm__("fr11") = 1.f;
        register float x1 __asm__("fr12") = s1[0], y1 __asm__("fr13") = s1[1], z1 __asm__("fr14") = s1[2];
        register float w1 __asm__("fr15") = 1.f;
        float r0, r1;
        __builtin_prefetch(s0 + 16);
        __asm__("ftrv xmtrx,fv8\n\tftrv xmtrx,fv12"
                : "+f"(x0), "+f"(y0), "+f"(z0), "+f"(w0), "+f"(x1), "+f"(y1), "+f"(z1), "+f"(w1));
        r0 = fast_rsqrt(w0 * w0); /* 1/|w|: vertices behind the eye are clipped by w anyway */
        r1 = fast_rsqrt(w1 * w1);
        v0->x = x0 * r0;
        v1->x = x1 * r1;
        v0->y = y0 * r0;
        v1->y = y1 * r1;
        v0->invz = r0;
        v1->invz = r1;
        v0->z = w0;
        v1->z = w1;
        v0->behind = w0 < znear;
        v1->behind = w1 < znear;
    }
    for(; i < c->n_pos; i++) {
        sr_screen_vertex_t *v = &s_scratch[i];
        float x = c->pos[i * 3], y = c->pos[i * 3 + 1], z = c->pos[i * 3 + 2], w;
        mat_trans_single3_nodivw(x, y, z, w);
        v->z = w;
        v->invz = fast_rsqrt(w * w);
        v->x = x * v->invz;
        v->y = y * v->invz;
        v->behind = w < znear;
    }
#else
    stats->verts_transformed += c->n_verts;
    for(i = 0; i < c->n_verts; i++) {
        sr_screen_vertex_t *v = &s_scratch[i];
        const sr_course_vertex_t *src = &c->verts[i];
        clip_vertex_t cv;
        xform(mv, src->x, src->y, src->z, cv.p);
        v->z = cv.p[2];
        if(cv.p[2] >= znear) {
            cv.u = src->u, cv.v = src->v, cv.argb = src->argb;
            project(view, &cv, v);
        }
        v->behind = v->z < znear;
        v->u = src->u;
        v->v = src->v;
        v->argb = src->argb;
    }
#endif
    t1 = CYCLES();
    f1 = FREEZE();
    stats->cycles_xform += t1 - t0;
    stats->freeze_xform += f1 - f0;
    for(b = 0; b < c->n_batches; b++) {
        const sr_course_batch_t *bt = &c->batches[b];
        const float bias = (1.f + LAYER_STEP * (float)((bt->flags & SR_TRI_LAYER_MASK) >> SR_TRI_LAYER_SHIFT)) *
                           (view->list == SR_LIST_TRANSLUCENT ? view->depth_bias : 1.f);
        sr_deferred_t *const defer = (bt->flags & SR_TRI_CUTOUT) ? view->deferred : NULL;
        const float cull = (bt->flags & SR_TRI_TWO_SIDED) ? 0.f : (float)sr_render_cull;
        const uint32_t end = bt->first + bt->count;
        if(!(((bt->flags & SR_TRI_CUTOUT) ? 2u : 1u) & want)) continue;
        {
            PROF_BEGIN();
#ifdef _arch_dreamcast
            if(view->headers[0]) material_dc(view, bt->texture, bt->flags);
            else
#endif
                view->material(view->user, bt->texture, bt->flags);
            PROF_END(cycles_material);
        }
        stats->batches++;
#ifdef _arch_dreamcast
        if(!view->tri) {
            uint32_t clip[CLIP_MAX], n_clip = 0, k;
            PROF_BEGIN();
            drawn += (s_lit ? batch_dc_lit : s_argb ? batch_dc_flat : batch_dc_pack)(c, bt->first, end, bias, cull,
                                                                                     defer, &culled, clip, &n_clip);
            if(defer) PROF_END(cycles_deferred);
            else PROF_END(cycles_direct);
            for(k = 0; k < n_clip; k++) clip_tri(c, clip[k], mv, view, defer, bias, stats);
            continue;
        }
#endif
        drawn += batch_generic(c, bt->first, end, mv, view, defer, bias, cull, stats, &culled);
    }
    stats->tris_drawn += drawn;
    stats->tris_culled += culled;
    stats->cycles_emit += (uint32_t)(CYCLES() - t1);
    stats->freeze_emit += (uint32_t)(FREEZE() - f1);
    (void)f0, (void)f1;
}

/* tested: the caller has already found the instance's sphere in view */
static SR_HOT void render_instance(const sr_course_mesh_t *mesh, const float mv[12], const sr_view_t *view,
                            sr_render_stats_t *stats, int tested) {
    float s2 = 0.f, scale;
    uint32_t k;
    for(k = 0; k < 3; k++) {
        const float l = mv[k * 3] * mv[k * 3] + mv[k * 3 + 1] * mv[k * 3 + 1] + mv[k * 3 + 2] * mv[k * 3 + 2];
        if(l > s2) s2 = l;
    }
    scale = sqrtf(s2);
    if(!tested && !sphere_visible(mv, mesh->bounds, scale, view)) {
        stats->instances_culled++;
        return;
    }
    stats->instances_drawn++;
#ifdef _arch_dreamcast
    load_projection(mv, view);
#endif
    s_lit = mesh->light && sr_light_tables ? mesh->light : NULL;
    if(s_lit) lit_instance(mv);
    for(k = 0; k < mesh->n_chunks; k++)
        if(sphere_visible(mv, mesh->chunks[k].bounds, scale, view))
            render_chunk(&mesh->chunks[k], mv, view, stats);
    s_lit = NULL;
}

/* The view frustum's side and near planes in the world, (n, d) with unit n: n . p + d >= -r keeps a sphere. */
static void world_planes(const float vm[12], const sr_view_t *view, float planes[5][4]) {
    const float view_planes[5][4] = {
        {0.f, 0.f, 1.f, -view->znear},
        {-view->focal_x, 0.f, view->width - view->cx, 0.f},
        {view->focal_x, 0.f, view->cx, 0.f},
        {0.f, -view->focal_y, view->cy, 0.f},
        {0.f, view->focal_y, view->height - view->cy, 0.f},
    };
    int p, j;
    for(p = 0; p < 5; p++) {
        const float *n = view_planes[p];
        const float len = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
        /* view p_v = R p_w + T: n . p_v + d = (R^T n) . p_w + (n . T + d) */
        for(j = 0; j < 3; j++) planes[p][j] = (n[0] * vm[j * 3] + n[1] * vm[j * 3 + 1] + n[2] * vm[j * 3 + 2]) / len;
        planes[p][3] = (n[0] * vm[9] + n[1] * vm[10] + n[2] * vm[11] + n[3]) / len;
    }
}

SR_HOT void sr_course_render(const sr_course_t *course, const float view_matrix[12], const sr_view_t *view,
                      sr_render_stats_t *stats) {
    float planes[5][4];
    uint32_t i;
    world_planes(view_matrix, view, planes);
    for(i = 0; i < course->n_instances; i++) {
        const sr_course_instance_t *inst = &course->instances[i];
        const float *b = course->instance_bounds[i];
        float mv[12];
        int p;
        if(!(inst->flags & SR_INST_STATIC) || inst->mesh >= course->n_meshes) continue;
        for(p = 0; p < 5; p++)
            if(planes[p][0] * b[0] + planes[p][1] * b[1] + planes[p][2] * b[2] + planes[p][3] < -b[3]) break;
        if(p < 5) {
            stats->instances_culled++;
            continue;
        }
        sr_m2_compose(mv, view_matrix, inst->m);
        render_instance(&course->meshes[inst->mesh], mv, view, stats, 1);
    }
}

void sr_mesh_render(const sr_course_t *course, uint32_t mesh, const float view_matrix[12], const float world[12],
                    const sr_view_t *view, sr_render_stats_t *stats) {
    float mv[12];
    if(mesh >= course->n_meshes) return;
    sr_m2_compose(mv, view_matrix, world);
    s_argb = view->list == SR_LIST_TRANSLUCENT ? view->argb : 0;
    render_instance(&course->meshes[mesh], mv, view, stats, 0);
    s_argb = 0;
}

void sr_mesh_render_mv(const sr_course_t *course, uint32_t mesh, const float mv[12], const sr_view_t *view,
                       sr_render_stats_t *stats) {
    if(mesh >= course->n_meshes) return;
    render_instance(&course->meshes[mesh], mv, view, stats, 0);
}
