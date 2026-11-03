/* PVR backend: textures uploaded once (ARGB1555 twiddled or VQ, as the pack stores them), a header per texture for the
 * opaque and the punch-through list (texture modulated by the vertex colour, gouraud), and a colour-only header for
 * flat faces. The core writes the triangles to the TA itself through the store queues. With SR_LIST_BOTH one
 * traversal in the opaque-list callback draws the opaque triangles and builds the punch-through list in RAM
 * (headers and vertices as the TA takes them), which sr_pvr_flush_deferred sends from the punch-through callback. */
#include "render/pvr_backend.h"
#include "render/ta.h"
#include "render/countdown.h"
#include "render/twiddle.h"
#include "sr_alloc.h"
#include <dc/pvr.h>
#include <stdio.h>
#include <math.h>
#include <malloc.h>
#include <stdlib.h>
#include "race/particles.h"
#include <string.h>

/* [list + 2 * actor][texture]; index n_textures is the flat colour one. The course's headers (list 0, 1) receive the
 * cars' shadows (cheap-shadow modifier: darkened inside a volume of the opaque modifier list); the cars' own (2, 3)
 * do not. */
static pvr_poly_hdr_t *s_headers[4];
static pvr_mod_hdr_t s_mod_headers[2] __attribute__((aligned(32))); /* a volume's other triangles, its last */
static uint32_t s_n_textures;

#define DEFERRED_BYTES (1024u * 1024u)

static struct {
    int list;
    float view[12];
    sr_render_stats_t *stats;
    uint16_t current[2]; /* header last sent, per list */
    int actor;           /* drawing the cars: their headers */
    sr_deferred_t deferred;
} s;

/* The particles: quads of the pack's particle meshes in the punch-through list with their cut-out texture (the
 * plain actor header, point sampled: uses_texture below), the texture once across a quad (render/actors.h): a puff
 * of single-texel grains, as the reference draws it. */
static uint32_t s_particle_quads; /* sent so far (sr_pvr_particle_report) */
static uint64_t s_deferred_bytes; /* the deferred lists' sizes, summed (sr_pvr_particle_report) */
static uint32_t s_deferred_flushes;

void sr_pvr_particle_report(void) {
    printf("sr: particle quads sent %lu\n", (unsigned long)s_particle_quads);
    if(s_deferred_flushes)
        printf("sr: deferred punch-through list a frame: %lu bytes\n",
               (unsigned long)(s_deferred_bytes / s_deferred_flushes));
}

static void particles_submit(const sr_actors_t *actors, const sr_course_t *course, const sr_view_t *v) {
    static sr_particle_quad_t quads[SR_PARTICLE_DRAWS_MAX];
    const uint32_t n = sr_particles_quads(actors, course, v, quads, SR_PARTICLE_DRAWS_MAX);
    uint32_t i, k;
    if(s.list != SR_LIST_BOTH) return;
    s_particle_quads += n;
    for(i = 0; i < n; i++) {
        pvr_poly_hdr_t *h;
        pvr_vertex_t *out;
        if(quads[i].texture >= s_n_textures) continue;
        h = sr_deferred_alloc(&s.deferred, sizeof(*h));
        if(!h) break;
        *h = s_headers[3][quads[i].texture];
        out = sr_deferred_alloc(&s.deferred, 4 * sizeof(*out));
        if(!out) break;
        for(k = 0; k < 4; k++) {
            const sr_particle_vertex_t *q = &quads[i].v[k];
            out[k].flags = k == 3 ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;
            out[k].x = q->x;
            out[k].y = q->y;
            out[k].z = q->invz;
            out[k].u = q->u;
            out[k].v = q->v;
            out[k].argb = quads[i].argb;
            out[k].oargb = 0;
        }
    }
    s.current[SR_LIST_CUTOUT] = 0xffff; /* the next punch-through batch sends its header again */
}

/* The shadow volumes' triangles of this frame (sr_pvr_shadows_prepare), sent from the modifier list's callback. */
#define SHADOW_TRIS_MAX 512
static struct {
    float v[3][3];
    uint32_t last;
} s_shadow_tris[SHADOW_TRIS_MAX];
static uint32_t s_n_shadow_tris;

/* point: nearest texel (the particles' grains stay single pixels, as on the arcade) */
static void compile(pvr_poly_hdr_t *hdr, int set, const sr_course_texture_t *t, pvr_ptr_t vram, int point) {
    pvr_poly_cxt_t cxt;
    const pvr_list_t pl = (set & 1) == SR_LIST_CUTOUT ? PVR_LIST_PT_POLY : PVR_LIST_OP_POLY;
    if(t) {
        pvr_poly_cxt_txr(&cxt, pl,
                         PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_TWIDDLED | ((t->flags & SR_TEX_VQ) ? PVR_TXRFMT_VQ_ENABLE : 0),
                         t->w, t->h, vram, point ? PVR_FILTER_NONE : PVR_FILTER_BILINEAR);
        cxt.txr.env = PVR_TXRENV_MODULATE;
        cxt.txr.uv_flip = ((t->flags & SR_TEX_MIRROR_U) ? PVR_UVFLIP_U : 0) | ((t->flags & SR_TEX_MIRROR_V) ? PVR_UVFLIP_V : 0);
        cxt.txr.uv_clamp = ((t->flags & SR_TEX_REPEAT_U) ? 0 : PVR_UVCLAMP_U) | ((t->flags & SR_TEX_REPEAT_V) ? 0 : PVR_UVCLAMP_V);
    } else {
        pvr_poly_cxt_col(&cxt, pl);
    }
    cxt.gen.culling = PVR_CULLING_NONE;
    if(set < 2) {
        cxt.fmt.modifier = PVR_MODIFIER_ENABLE;
        cxt.gen.modifier_mode = PVR_MODIFIER_CHEAP_SHADOW;
    }
    pvr_poly_compile(hdr, &cxt);
}

/* A mesh of this object draws with texture tex. */
static int uses_texture(const sr_course_t *course, uint32_t oba, uint32_t tex) {
    uint32_t m, t;
    for(m = 0; m < course->n_meshes; m++)
        if(course->meshes[m].oba == oba)
            for(t = 0; t < course->meshes[m].n_tris; t++)
                if(course->meshes[m].tris[t].texture == tex) return 1;
    return 0;
}

int sr_pvr_course_init(const sr_course_t *course) {
    uint32_t i, bytes = 0;
    int l;
    s_n_textures = course->n_textures;
    pvr_mod_compile(&s_mod_headers[0], PVR_LIST_OP_MOD, PVR_MODIFIER_OTHER_POLY, PVR_CULLING_NONE);
    pvr_mod_compile(&s_mod_headers[1], PVR_LIST_OP_MOD, PVR_MODIFIER_INCLUDE_LAST_POLY, PVR_CULLING_NONE);
    pvr_set_shadow_scale(true, 0.5f); /* inside a shadow volume a receiver is drawn at half brightness */
    s.deferred.data = memalign(32, DEFERRED_BYTES);
    s.deferred.cap = s.deferred.data ? DEFERRED_BYTES : 0;
    /* The background plane sits at 1/z = 0.0001 by default (KOS), so nothing beyond 10,000 units would draw; the
     * sky meshes reach 212,000. */
    pvr_set_zclip(1e-7f);
    for(l = 0; l < 4; l++) {
        s_headers[l] = memalign(32, (s_n_textures + 1) * sizeof(pvr_poly_hdr_t));
        if(!s_headers[l]) return -1;
        compile(&s_headers[l][s_n_textures], l, NULL, NULL, 0);
    }
    for(i = 0; i < course->n_textures; i++) {
        const sr_course_texture_t *t = &course->textures[i];
        const uint32_t size = sr_course_texture_bytes(t->w, t->h, t->flags);
        pvr_ptr_t vram = pvr_mem_malloc(size);
        if(!vram) {
            printf("sr: VRAM full at texture %lu of %lu (%lu bytes so far)\n", (unsigned long)i,
                   (unsigned long)course->n_textures, (unsigned long)bytes);
            return -1;
        }
        pvr_txr_load(t->texels, vram, size);
        bytes += size;
        for(l = 0; l < 4; l++) compile(&s_headers[l][i], l, t, vram, uses_texture(course, SR_OBA_DUST, i) ||
                                                                       uses_texture(course, SR_OBA_DEBRIS, i));
    }
    printf("sr: %lu textures, %lu KiB of VRAM\n", (unsigned long)course->n_textures, (unsigned long)(bytes >> 10));
    return 0;
}

static void material(void *user, uint16_t texture, uint16_t flags) {
    const uint32_t ix = texture < s_n_textures ? texture : s_n_textures;
    const int list = s.list == SR_LIST_BOTH ? ((flags & SR_TRI_CUTOUT) ? SR_LIST_CUTOUT : SR_LIST_OPAQUE) : s.list;
    const pvr_poly_hdr_t *hdr = &s_headers[list + (s.actor ? 2 : 0)][ix];
    (void)user;
    if(ix == s.current[list]) return;
    s.current[list] = (uint16_t)ix;
    if(s.list == SR_LIST_BOTH && list == SR_LIST_CUTOUT) {
        pvr_poly_hdr_t *h = sr_deferred_alloc(&s.deferred, sizeof(pvr_poly_hdr_t));
        if(h) *h = *hdr;
        else s.current[list] = 0xffff;
        return;
    }
#ifndef SR_RENDER_NULL
    {
        /* through the store queue like a vertex: pvr_prim's general copy costs several times more per header */
        uint32_t *d = (uint32_t *)sr_ta_target();
        const uint32_t *h = (const uint32_t *)hdr;
        d[0] = h[0], d[1] = h[1], d[2] = h[2], d[3] = h[3], d[4] = h[4], d[5] = h[5], d[6] = h[6], d[7] = h[7];
        sr_ta_commit(d);
    }
#endif
}

/* The view for s.list and s.actor's headers. */
static sr_view_t view_for(void) {
    sr_view_t v = {640.f, 480.f, 320.f, 240.f, SR_FOCAL_X, SR_FOCAL_Y, 0.1f, s.list, material, 0, 0,
                   s.list == SR_LIST_BOTH ? &s.deferred : 0, 0, 1.f};
    if(s.list == SR_LIST_OPAQUE || s.list == SR_LIST_CUTOUT || s.list == SR_LIST_BOTH) {
        v.headers[0] = s_headers[SR_LIST_OPAQUE + (s.actor ? 2 : 0)];
        v.headers[1] = s_headers[SR_LIST_CUTOUT + (s.actor ? 2 : 0)];
        v.n_headers = s_n_textures;
        v.current = s.current;
    }
    return v;
}

void sr_pvr_begin(int list, const float view_matrix[12], sr_render_stats_t *stats) {
    int i;
    s.list = list;
    for(i = 0; i < 12; i++) s.view[i] = view_matrix[i];
    s.stats = stats;
    s.current[0] = s.current[1] = 0xffff;
    if(list == SR_LIST_BOTH) s.deferred.used = 0;
}

uint32_t sr_pvr_flush_deferred(void) {
    s_deferred_bytes += s.deferred.used;
    s_deferred_flushes++;
#ifndef SR_RENDER_NULL
    if(s.deferred.used) pvr_prim(s.deferred.data, s.deferred.used);
#endif
    return s.deferred.dropped;
}

void sr_pvr_course(const sr_course_t *course) {
    sr_view_t v;
    s.actor = 0;
    s.current[0] = s.current[1] = 0xffff;
    v = view_for();
    sr_course_render(course, s.view, &v, s.stats);
}

void sr_pvr_mesh(const sr_course_t *course, uint32_t mesh, const float world[12]) {
    const sr_view_t v = view_for();
    sr_mesh_render(course, mesh, s.view, world, &v, s.stats);
}

void sr_pvr_actors(const sr_actors_t *actors, const sr_course_t *course) {
    sr_view_t v;
    s.actor = 1;
    s.current[0] = s.current[1] = 0xffff;
    v = view_for();
    sr_actors_render(actors, course, s.view, &v, s.stats);
    particles_submit(actors, course, &v);
}

static void shadow_tri(void *user, const float v[3][3], int last) {
    (void)user;
    if(s_n_shadow_tris == SHADOW_TRIS_MAX) return;
    sr_copy(s_shadow_tris[s_n_shadow_tris].v, v, sizeof(s_shadow_tris[0].v));
    s_shadow_tris[s_n_shadow_tris++].last = (uint32_t)last;
}

uint32_t sr_pvr_shadows_prepare(const sr_actors_t *actors, const sr_course_t *course, const float view_matrix[12]) {
    sr_view_t v = view_for();
    uint32_t volumes;
    s_n_shadow_tris = 0;
    volumes = sr_actors_shadow_volumes(actors, course, view_matrix, &v, shadow_tri, NULL);
    /* a volume cut short by the buffer would leave the list unclosed: drop back to the last complete one */
    while(s_n_shadow_tris && !s_shadow_tris[s_n_shadow_tris - 1].last) s_n_shadow_tris--;
    return s_n_shadow_tris ? volumes : 0;
}

void sr_pvr_shadows_submit(void) {
    uint32_t i;
    for(i = 0; i < s_n_shadow_tris; i++) {
        const float(*v)[3] = s_shadow_tris[i].v;
        pvr_modifier_vol_t *first, *second;
        /* a volume's first triangle under OTHER_POLY, its last under INCLUDE_LAST_POLY (closes the volume) */
        const int first_of_volume = i == 0 || s_shadow_tris[i - 1].last;
        if(first_of_volume || s_shadow_tris[i].last) {
            pvr_mod_hdr_t *h = sr_ta_target();
            *h = s_mod_headers[s_shadow_tris[i].last ? 1 : 0];
            sr_ta_commit(h);
        }
        /* 64 bytes: two consecutive targets, each half sent once filled */
        first = sr_ta_target();
        second = (pvr_modifier_vol_t *)((uint8_t *)sr_ta_target() - 32);
        first->flags = PVR_CMD_VERTEX_EOL;
        first->ax = v[0][0], first->ay = v[0][1], first->az = v[0][2];
        first->bx = v[1][0], first->by = v[1][1], first->bz = v[1][2];
        first->cx = v[2][0];
        sr_ta_commit(first);
        second->cy = v[2][1], second->cz = v[2][2];
        sr_ta_commit((uint8_t *)second + 32);
    }
}

/* The countdown's digits (render/countdown.h): the pack's cut-out headers, in front of the course (1/z 100: the
 * arcade draws their GEO window over it). From the punch-through callback. */
void sr_pvr_countdown(const sr_course_t *course) {
    static sr_countdown_tri_t tris[16];
    const uint32_t n = sr_countdown_tris(course, 640.f, 480.f, tris, 16);
    uint32_t i, k, current = 0xffffffffu;
    for(i = 0; i < n; i++) {
        const uint32_t ix = tris[i].texture < s_n_textures ? tris[i].texture : s_n_textures;
        if(ix != current) {
            pvr_poly_hdr_t *h = sr_ta_target();
            *h = s_headers[3][ix];
            sr_ta_commit(h);
            current = ix;
        }
        for(k = 0; k < 3; k++) {
            pvr_vertex_t *v = sr_ta_target();
            v->flags = k == 2 ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;
            v->x = tris[i].x[k];
            v->y = tris[i].y[k];
            v->z = 100.f;
            v->u = tris[i].u[k];
            v->v = tris[i].v[k];
            v->argb = tris[i].argb[k];
            v->oargb = 0;
            sr_ta_commit(v);
        }
    }
    s.current[SR_LIST_CUTOUT] = 0xffff;
}

/* A white wash over the frame: alpha wash / 256 (0..256), from the translucent-list callback. */
void sr_pvr_wash(uint32_t wash) {
    static pvr_poly_hdr_t hdr __attribute__((aligned(32)));
    static int compiled;
    const float xy[4][2] = {{0.f, 0.f}, {640.f, 0.f}, {0.f, 480.f}, {640.f, 480.f}};
    const uint32_t a = wash >= 256u ? 255u : wash;
    uint32_t k;
    if(!compiled) {
        pvr_poly_cxt_t cxt;
        pvr_poly_cxt_col(&cxt, PVR_LIST_TR_POLY);
        cxt.gen.culling = PVR_CULLING_NONE;
        cxt.depth.comparison = PVR_DEPTHCMP_ALWAYS;
        cxt.depth.write = PVR_DEPTHWRITE_DISABLE;
        pvr_poly_compile(&hdr, &cxt);
        compiled = 1;
    }
    {
        pvr_poly_hdr_t *h = sr_ta_target();
        *h = hdr;
        sr_ta_commit(h);
    }
    for(k = 0; k < 4; k++) {
        pvr_vertex_t *v = sr_ta_target();
        v->flags = k == 3 ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;
        v->x = xy[k][0];
        v->y = xy[k][1];
        v->z = 1000.f;
        v->u = v->v = 0.f;
        v->argb = a << 24 | 0xffffffu;
        v->oargb = 0;
        sr_ta_commit(v);
    }
}
