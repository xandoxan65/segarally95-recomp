/* Course export: capture the desert course's scenery and cars from the host reference while it drives the
 * practice race, and write a course pack (dreamcast-native/include/course_pack.h).
 *
 * Env (also --export-course FILE):
 *   I960_HOST_EXPORT_COURSE=FILE     enable; the pack is written at exit
 *   I960_HOST_EXPORT_AUTOPILOT=1     with no --inputs: steer the player's car along the path the road-following
 *                                    car (first car_frame node) drives, full throttle, automatic gears
 *   I960_HOST_EXPORT_AUTOPILOT_INPUTS=FILE  with the autopilot: also write the controls it chose as an --inputs
 *                                    script (row r: what race tick r's poll samples; row 0 the idle wheel), so a
 *                                    scripted run (--inputs FILE) replays the same drive
 *   I960_HOST_EXPORT_COUNTDOWN=1     capture from the race scene's start, the countdown before GO included (its 3D
 *                                    digits and start lights are drawn only then)
 *   I960_HOST_EXPORT_STATIC_FRAMES=N frames an instance must be drawn at one world matrix to count as scenery
 *                                    (default 6)
 *   I960_HOST_EXPORT_DEBUG=1         per-capture diagnostics on stderr
 *   I960_HOST_EXPORT_FRAMES="600 1200"  (works without the pack) at these race ticks (trace race_tick column: 0 is the
 *                                    first tick with 0x214120 > 0) write DIR/frame@N.ppm, 640x480, of the frame's
 *                                    decoded GEO mesh projected as the host GL viewer does, and DIR/frame@N.txt
 *                                    with focus, window, center and the latched view matrix
 *   I960_HOST_EXPORT_FRAMES_DIR=DIR  default build/course
 *   I960_HOST_EXPORT_DYNDUMP=FILE    (with the pack) per captured frame: the race tick, raw snapshots of the
 *                                    candidate pose blocks (8 x 0x58 bytes at 0x213b40) and nodes (4 x 0x100
 *                                    bytes at 0x500000), and every draw {oba, obc, n_tris, world[12]}, for fitting
 *                                    the moving objects to their poses (course_pack_info.py --attach)
 *
 * Frame projection (src/host/sys24_viewer.c geo_load_model2_projection, viewport = GEO window v0..v3, eye-0
 * center c): decoded verts are view space with focus already applied (x*focus.x, y*focus.y; per-object eye centers
 * baked), +z forward, clip w = z, near z = 0.01. Then, in pixels of the GEO window:
 *   column        = (c.x - v0) + x/z
 *   GL window row = (c.y - v1) + y/z           counted from the bottom (GL); the geo layer is not flipped, so
 *   row from top  = (v3 - c.y) - y/z           view-space +y is screen UP
 * scaled by 640/(v2-v0), 480/(v3-v1). Fill: flat colour per triangle (the pack colours), z-buffer on 1/z, sky
 * 90,140,200, all exportable polygons both sides (the viewer disables culling).
 *
 * Capture. Once per tick (i960_host_frame_present) while the race clock 0x214120 > 0, the latest latched GEO
 * display frame is decoded synchronously (model2_geo_render_capture_display: own buffers, the published mesh and
 * the decode worker are untouched). Every object command gives oba/obc/tpa/tha, the GEO mode and the model-view
 * matrix the GEO state applied (the last 0x0B/0x0C before it). The camera latches the TGP view each tick
 * (model2_hw_view_matrix); model-view = view * world, so world = inverse(view) * model-view. The GEO focus is a
 * separate state (applied after the matrix), so the matrices carry no projection scale.
 *
 * Classification. Draws are clustered per mesh by world matrix (translation within 0.05, rotation entries within
 * 2e-3). A cluster drawn in at least STATIC_FRAMES distinct frames is scenery (SR_INST_STATIC), unless most of the
 * mesh's draws are one-frame clusters (a moving object that happened to stand still: the cars at the start).
 * Static clusters of a mesh within 1.0 of each other are one site: an animated figure keeps its most-drawn pose,
 * a chain of more than 4 (something crawling to a stop) is dropped. Every
 * mesh without a static instance gets one instance with flags 0 (cars, wheels, camera-relative objects).
 *
 * Meshes. Each object is decoded once in its own space (model2_geo_dl_decode_object: identity matrix, unit focus).
 * Polygon ROM objects are keyed by (oba, obc, mode); polygon RAM objects (span_table / marker slots, re-uploaded
 * along the lap) by (oba, obc, mode, content hash), each content its own mesh. Quads are fanned into two
 * triangles. Pack v2 (dreamcast-native/include/course_pack.h):
 *   luma      each polygon's luminance as the game computed it for the draw that created the mesh (poly_luma of
 *             the frame's own decode, matched polygon by polygon), quantised to 16 steps for textures
 *   textures  the patch the host GL shader samples (sheet, patch origin/size, colorbase, lumabase), decoded with
 *             model2_palette_lookup_texel at that luma (baked: the vertex colour is white), texel 0xF of cut-out
 *             polygons -> alpha 0 + SR_TRI_CUTOUT, ARGB1555 in sr_twiddle_index order, deduped by content;
 *             flags REPEAT_U|V always (the GL shader always wraps inside the patch) + MIRROR_U/V from the header
 *   uv        patch-local pixels (pu/8, pv/8) divided by the patch size
 *   flat      SR_NO_TEXTURE, vertex colour model2_palette_lookup_solid(colorbase, luma)
 *   layers    a polygon coplanar with (normal dot > 0.999, plane distance < 0.01) and overlapping an earlier
 *             polygon of the same mesh is one decal layer above it (cap 15)
 * Textures and colours are resolved when a mesh is first decoded, so texture RAM (texture_sync uploads along the
 * lap) and palette are the ones the draw used.
 */

#include "i960_host.h"
#include "i960_mem.h"
#include "lift_log.h"
#include "model2_geo_dl.h"
#include "model2_geo_render.h"
#include "model2_geo_tex.h"
#include "model2_hw.h"
#include "model2_rom.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "../../dreamcast-native/include/course_pack.h"
#include "../../dreamcast-native/include/render/twiddle.h"

#define EXPORT_T_TOL   0.05f
#define EXPORT_R_TOL   2e-3f
#define EXPORT_CELL    0.25f

typedef struct {
    float p[9]; /* 3 corners xyz */
    u16 colorbase, lumabase, patch_x, patch_y, patch_w, patch_h;
    u8 sheet, tex_flags, luma, tex_valid;
    u32 argb;
    /* v2 */
    float uv[6];      /* texture units */
    float n[3];       /* polygon normal, object space, unit */
    float amb, dif;   /* texture parameter set of the polygon (MAME luminance terms) */
    u32 base_rgb;     /* flat: palette colour at full luma; textured: white */
    u32 prim;         /* source polygon (fanned quads share it) */
    u16 tex;          /* SR_NO_TEXTURE or texture index */
    u16 tflags;       /* SR_TRI_* */
    u8 two_sided;
    /* the lit meshes (lit_oba): baked at luma 255, lit at run time (the .light file) */
    u8 spec_ctrl, mode_spec;
    u16 light_table;
    float spec_scale;
} ex_tri_t;

/* Meshes lit per frame on the console: the cars (bodies, details, wheels) and the dust and debris quads. */
static int lit_oba(u32 oba)
{
    switch (oba) {
    case 0x80e8ddu: case 0x815430u: case 0x815574u: case 0x814e11u: case 0x8154b9u: case 0x808596u:
    case 0x82c95bu: case 0x82c96cu:
        return 1;
    default:
        return 0;
    }
}

/* k(L), L = 0, 16, ..., 256: how bright a colour baked at luma 255 shows at luma L, the least-squares factor
 * over the class's palette (the hardware's lumaram x luma / 256 through colorxlat is not linear in L). */
#define LIGHT_TABLES_MAX 256
static struct {
    u8 textured;
    u16 colorbase, lumabase;
    float k[17];
} s_light_tables[LIGHT_TABLES_MAX];
static unsigned s_n_light_tables;

static u16 light_table_for(int textured, u16 colorbase, u16 lumabase)
{
    unsigned i, li;

    for (i = 0; i < s_n_light_tables; i++)
        if (s_light_tables[i].textured == (u8)textured && s_light_tables[i].colorbase == colorbase
            && s_light_tables[i].lumabase == lumabase)
            return (u16)i;
    if (s_n_light_tables == LIGHT_TABLES_MAX)
        return 0;
    i = s_n_light_tables++;
    s_light_tables[i].textured = (u8)textured;
    s_light_tables[i].colorbase = colorbase;
    s_light_tables[i].lumabase = lumabase;
    for (li = 0; li <= 16u; li++) {
        const u32 L = li * 16u > 255u ? 255u : li * 16u;
        double sab = 0, sbb = 0;
        unsigned t;
        int ch;

        for (t = 0; t < (textured ? 15u : 1u); t++) {
            u8 a[4], b[4];

            if (textured) {
                model2_palette_lookup_texel(t, colorbase, lumabase, L, 0, a);
                model2_palette_lookup_texel(t, colorbase, lumabase, 255, 0, b);
            } else {
                model2_palette_lookup_solid(colorbase, L, a);
                model2_palette_lookup_solid(colorbase, 255, b);
            }
            for (ch = 0; ch < 3; ch++) {
                sab += (double)a[ch] * b[ch];
                sbb += (double)b[ch] * b[ch];
            }
        }
        s_light_tables[i].k[li] = sbb > 0 ? (float)(sab / sbb) : (float)L / 255.f;
    }
    return (u16)i;
}

typedef struct {
    u16 w, h;
    u32 flags;
    u16 *texels; /* twiddled ARGB1555 */
    u32 hash;
    int has_cut;
} ex_tex_t;

/* The frame's decoded polygons of the object being captured (same parse, same order as its own-space decode):
 * their poly_luma is the luminance the game computed for this draw (MAME geo_parse). */
static const model2_mesh_prim_t *s_cur_fprims;
static unsigned s_cur_fleft;
static unsigned s_luma_from_frame, s_luma_fallback;

static ex_tex_t *s_tex;
static unsigned s_n_tex, s_cap_tex;
static unsigned s_tex_clamped, s_tex_no_sheet;


typedef struct {
    u32 oba, obc, hash;
    int mode;
    int is_ram;
    ex_tri_t *tris;
    unsigned n_tris;
    float bounds[4];
    unsigned draws;      /* total draws */
    unsigned single;     /* draws in clusters that stayed one-frame (computed at the end) */
    int first_cluster;   /* linked list of clusters */
    int out_index;
    const float *rep;    /* the first exported instance's world matrix */
    unsigned n_prims_all; /* polygons in the decode, exportable or not (walks the frame's polygon list) */
} ex_mesh_t;

typedef struct {
    float w[12];
    u32 mesh;
    u32 frames;
    u32 last_frame;
    int next_in_mesh;
    int next_in_cell;
    int cx, cy, cz;
} ex_cluster_t;

typedef struct {
    u16 colorbase, lumabase, patch_x, patch_y, patch_w, patch_h;
    u8 sheet, flags, luma, tex;
    u32 argb;
} colour_cache_t;

static colour_cache_t *s_cc;
static unsigned s_cc_n, s_cc_cap;

static int s_enabled = -1;
static int s_pack;
#define FRAMES_MAX 64
static int s_frame_ticks[FRAMES_MAX];
static unsigned s_n_frame_ticks;
static char s_frames_dir[1024] = "build/course";
static int s_rtick = -1;      /* race tick, as lift_race_trace.c counts it */
static int s_render_tick = -1; /* render this capture to a PPM */
static char s_path[1024];
static int s_debug;
static int s_autopilot;
static unsigned s_static_frames = 6;

static ex_mesh_t *s_meshes;
static unsigned s_n_meshes, s_cap_meshes;
static ex_cluster_t *s_clusters;
static unsigned s_n_clusters, s_cap_clusters;

/* Cluster spatial hash: (mesh, cell) -> head index. */
static int *s_cell_head;
static u32 *s_cell_key_mesh;
static int *s_cell_key_xyz; /* 3 per slot */
static unsigned s_cell_cap;
static unsigned s_cell_used;

static u32 s_frame;          /* captured frames */
static int s_race_done;
static FILE *s_dyn;
static float s_view[12];
static unsigned s_draws_total;
static double s_ortho_err_max;
static double s_scale_min = 1e30, s_scale_max = 0;

/* ---------------------------------------------------------------- matrices */

static void mat_mul(float out[12], const float a[12], const float b[12])
{
    /* out = a * b; columns: R[:,c] = m[3c..3c+2], T = m[9..11]. */
    float r[12];
    int c, i;

    for (c = 0; c < 4; c++) {
        for (i = 0; i < 3; i++) {
            float v = a[i] * b[3 * c] + a[3 + i] * b[3 * c + 1] + a[6 + i] * b[3 * c + 2];

            if (c == 3)
                v += a[9 + i];
            r[3 * c + i] = v;
        }
    }
    memcpy(out, r, sizeof(r));
}

static int mat_inverse(float out[12], const float m[12])
{
    /* R element (row i, col c) = m[3c+i]. */
    double a = m[0], b = m[3], c = m[6];
    double d = m[1], e = m[4], f = m[7];
    double g = m[2], h = m[5], k = m[8];
    double A = e * k - f * h, B = -(d * k - f * g), C = d * h - e * g;
    double det = a * A + b * B + c * C;
    double inv[9];
    int i;

    if (fabs(det) < 1e-20)
        return -1;
    /* inverse rows */
    inv[0] = A / det;                 /* (0,0) */
    inv[1] = -(b * k - c * h) / det;  /* (0,1) */
    inv[2] = (b * f - c * e) / det;   /* (0,2) */
    inv[3] = B / det;                 /* (1,0) */
    inv[4] = (a * k - c * g) / det;   /* (1,1) */
    inv[5] = -(a * f - c * d) / det;  /* (1,2) */
    inv[6] = C / det;                 /* (2,0) */
    inv[7] = -(a * h - b * g) / det;  /* (2,1) */
    inv[8] = (a * e - b * d) / det;   /* (2,2) */
    /* back to column layout: out[3c+i] = inv(row i, col c) */
    for (i = 0; i < 3; i++) {
        int cc;

        for (cc = 0; cc < 3; cc++)
            out[3 * cc + i] = (float)inv[3 * i + cc];
    }
    for (i = 0; i < 3; i++)
        out[9 + i] = -(float)(inv[3 * i] * m[9] + inv[3 * i + 1] * m[10] + inv[3 * i + 2] * m[11]);
    return 0;
}

/* -------------------------------------------------------------- mesh table */

static u32 hash_mesh(const model2_mesh_collector_t *m)
{
    u32 h = 2166136261u;
    const u8 *p;
    size_t n, i;
    unsigned pi;

    p = (const u8 *)m->verts;
    n = (size_t)m->n_verts * sizeof(*m->verts);
    for (i = 0; i < n; i++)
        h = (h ^ p[i]) * 16777619u;
    for (pi = 0; pi < m->n_prims; pi++) {
        u32 a = m->prims[pi].attr;

        h = (h ^ a) * 16777619u;
    }
    return h ? h : 1u;
}

static u8 param_luma(const model2_geo_state_t *st, u32 attr)
{
    const model2_texture_param_t *tp = &st->texture_parameters[(attr >> 18) & 0x1fu];
    float l = tp->ambient + 0.75f * tp->diffuse;

    if (l < 0.f)
        l = 0.f;
    if (l > 255.f)
        l = 255.f;
    return (u8)l;
}

/* ---------------------------------------------------------------- textures */

typedef struct {
    u16 px, py, pw, ph, cb, lb;
    u8 sheet, flags, luma;
    u32 frame;
    int index;
} tex_key_t;

static tex_key_t s_tk[512];
static unsigned s_tk_n;

static u16 to1555(const u8 rgba[4], int cut)
{
    return (u16)((cut ? 0u : 0x8000u) | ((u32)(rgba[0] >> 3) << 10) | ((u32)(rgba[1] >> 3) << 5)
                 | (u32)(rgba[2] >> 3));
}

static u16 tex_dim(u16 d)
{
    u16 r = 8;

    if (d > 1024) {
        s_tex_clamped++;
        return 1024;
    }
    while (r < d)
        r = (u16)(r << 1);
    return r;
}

/* The patch the host GL shader samples (model2_geo_gl.c: sheet R8 from model2_get_texel, palette LUT from
 * model2_palette_build_texel_lut with the polygon luma, cut-out = texel 0xF when the cutout bit is set), decoded
 * with the polygon's luma baked in (the vertex colour is white). The luma axis of the lookup is not a brightness
 * scale (lumaram * luma / 256 indexes colorxlat ramps: colours shift, not dim), so it cannot be a modulation. */
static int texture_for(const model2_mesh_prim_t *p, u8 luma)
{
    const u32 *sheet = model2_tex_sheet_bank(p->sheet & 1u);
    int cutout = (p->tex_flags & MODEL2_GEO_TEX_CUTOUT) != 0;
    u16 pw = p->patch_w ? p->patch_w : 32, ph = p->patch_h ? p->patch_h : 32;
    u16 w, h;
    u32 flags = SR_TEX_REPEAT_U | SR_TEX_REPEAT_V, hsh = 2166136261u;
    u16 *tx;
    unsigned i, x, y;
    int cut_any = 0;
    ex_tex_t *t;

    if (p->tex_flags & MODEL2_GEO_TEX_MIRROR_X)
        flags |= SR_TEX_MIRROR_U;
    if (p->tex_flags & MODEL2_GEO_TEX_MIRROR_Y)
        flags |= SR_TEX_MIRROR_V;
    for (i = 0; i < s_tk_n; i++) {
        const tex_key_t *k = &s_tk[i];

        if (k->frame == s_frame && k->px == p->patch_x && k->py == p->patch_y && k->pw == pw && k->ph == ph
            && k->cb == p->colorbase && k->lb == p->lumabase && k->sheet == p->sheet
            && k->flags == p->tex_flags && k->luma == luma)
            return k->index;
    }
    if (!sheet) {
        s_tex_no_sheet++;
        return -1;
    }
    w = tex_dim(pw);
    h = tex_dim(ph);
    tx = (u16 *)malloc((size_t)w * h * sizeof(u16));
    if (!tx)
        return -1;
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            /* Patch sizes are powers of two (32 << n); a smaller-than-8 one would repeat. */
            u16 tv = model2_get_texel(sheet, p->patch_x, p->patch_y, (int)(x % pw), (int)(y % ph));
            int cut = cutout && tv == 0x0f;
            u8 rgba[4];

            model2_palette_lookup_texel(tv, p->colorbase, p->lumabase, luma, cutout, rgba);
            cut_any |= cut;
            tx[sr_twiddle_index(x, y, w, h)] = to1555(rgba, cut);
        }
    for (i = 0; i < (unsigned)w * h; i++)
        hsh = (hsh ^ tx[i]) * 16777619u;
    hsh = (hsh ^ ((u32)w << 16 | h)) * 16777619u;
    hsh = (hsh ^ flags) * 16777619u;
    for (i = 0; i < s_n_tex; i++) {
        t = &s_tex[i];
        if (t->hash == hsh && t->w == w && t->h == h && t->flags == flags
            && memcmp(t->texels, tx, (size_t)w * h * sizeof(u16)) == 0) {
            free(tx);
            goto found;
        }
    }
    if (s_n_tex >= s_cap_tex) {
        unsigned nc = s_cap_tex ? s_cap_tex * 2u : 64u;
        ex_tex_t *nt = (ex_tex_t *)realloc(s_tex, nc * sizeof(*nt));

        if (!nt) {
            free(tx);
            return -1;
        }
        s_tex = nt;
        s_cap_tex = nc;
    }
    t = &s_tex[s_n_tex];
    t->w = w;
    t->h = h;
    t->flags = flags;
    t->texels = tx;
    t->hash = hsh;
    t->has_cut = cut_any;
    i = s_n_tex++;
found:
    if (s_tk_n < sizeof(s_tk) / sizeof(s_tk[0])) {
        tex_key_t *k = &s_tk[s_tk_n++];

        k->px = p->patch_x;
        k->py = p->patch_y;
        k->pw = pw;
        k->ph = ph;
        k->cb = p->colorbase;
        k->lb = p->lumabase;
        k->sheet = p->sheet;
        k->flags = p->tex_flags;
        k->luma = luma;
        k->frame = s_frame;
        k->index = (int)i;
    } else {
        s_tk_n = 0;
    }
    return (int)i;
}

/* ------------------------------------------------------------ decal layers */

/* 2-D separating-axis overlap of two triangles projected along their dominant axis; touching (shared edges)
 * is not overlap. */
static int tri_overlap_2d(const float a[6], const float b[6])
{
    const float *tri[2] = {a, b};
    int ti, e, k;

    for (ti = 0; ti < 2; ti++)
        for (e = 0; e < 3; e++) {
            const float *t = tri[ti];
            float ex = t[2 * ((e + 1) % 3)] - t[2 * e], ey = t[2 * ((e + 1) % 3) + 1] - t[2 * e + 1];
            float nx = -ey, ny = ex, len = sqrtf(nx * nx + ny * ny);
            float mn[2] = {1e30f, 1e30f}, mx[2] = {-1e30f, -1e30f};
            int q;

            if (len < 1e-9f)
                return 0;
            nx /= len;
            ny /= len;
            for (q = 0; q < 2; q++)
                for (k = 0; k < 3; k++) {
                    float d = tri[q][2 * k] * nx + tri[q][2 * k + 1] * ny;

                    if (d < mn[q]) mn[q] = d;
                    if (d > mx[q]) mx[q] = d;
                }
            if (mx[0] <= mn[1] + 1e-3f || mx[1] <= mn[0] + 1e-3f)
                return 0;
        }
    return 1;
}

static void tri_plane(const ex_tri_t *t, float n[3], float *d)
{
    float ax = t->p[3] - t->p[0], ay = t->p[4] - t->p[1], az = t->p[5] - t->p[2];
    float bx = t->p[6] - t->p[0], by = t->p[7] - t->p[1], bz = t->p[8] - t->p[2];
    float l;

    n[0] = ay * bz - az * by;
    n[1] = az * bx - ax * bz;
    n[2] = ax * by - ay * bx;
    l = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    if (l > 0.f) {
        n[0] /= l;
        n[1] /= l;
        n[2] /= l;
    }
    *d = n[0] * t->p[0] + n[1] * t->p[1] + n[2] * t->p[2];
}

static void project2(const ex_tri_t *t, int axis, float out[6])
{
    int k;

    for (k = 0; k < 3; k++) {
        out[2 * k] = t->p[3 * k + (axis == 0 ? 1 : 0)];
        out[2 * k + 1] = t->p[3 * k + (axis == 2 ? 1 : 2)];
    }
}

/* A polygon coplanar with (normal dot > 0.999, plane distance < 0.01) and overlapping an earlier polygon of the
 * same object goes one layer above it (painter order: road markings, kerbs, painted signs). */
static void assign_layers(ex_mesh_t *em)
{
    unsigned i, j;
    float (*pl)[4] = (float (*)[4])malloc((em->n_tris ? em->n_tris : 1u) * sizeof(*pl));
    u8 *layer = (u8 *)calloc(em->n_tris ? em->n_tris : 1u, 1);

    if (!pl || !layer) {
        free(pl);
        free(layer);
        return;
    }
    for (i = 0; i < em->n_tris; i++)
        tri_plane(&em->tris[i], pl[i], &pl[i][3]);
    for (i = 0; i < em->n_tris; i++) {
        int axis = 0;
        float a2[6];

        if (fabsf(pl[i][1]) >= fabsf(pl[i][0]) && fabsf(pl[i][1]) >= fabsf(pl[i][2]))
            axis = 1;
        else if (fabsf(pl[i][2]) >= fabsf(pl[i][0]))
            axis = 2;
        project2(&em->tris[i], axis, a2);
        for (j = 0; j < i; j++) {
            float b2[6];

            if (em->tris[j].prim == em->tris[i].prim || layer[j] + 1u <= layer[i])
                continue;
            if (pl[i][0] * pl[j][0] + pl[i][1] * pl[j][1] + pl[i][2] * pl[j][2] <= 0.999f)
                continue;
            if (fabsf(pl[i][3] - pl[j][3]) >= 0.01f)
                continue;
            project2(&em->tris[j], axis, b2);
            if (!tri_overlap_2d(a2, b2))
                continue;
            layer[i] = (u8)(layer[j] + 1u > 15u ? 15u : layer[j] + 1u);
        }
    }
    for (i = 0; i < em->n_tris; i++)
        em->tris[i].tflags = (u16)((em->tris[i].tflags & ~SR_TRI_LAYER_MASK) | ((u32)layer[i] << SR_TRI_LAYER_SHIFT));
    free(pl);
    free(layer);
}

static u32 tri_colour(const ex_tri_t *t);

static int mesh_from_collector(ex_mesh_t *em, const model2_mesh_collector_t *c,
                               const model2_geo_state_t *st)
{
    unsigned pi, n = 0;
    float mn[3] = {1e30f, 1e30f, 1e30f}, mx[3] = {-1e30f, -1e30f, -1e30f};
    float r2 = 0.f;

    for (pi = 0; pi < c->n_prims; pi++)
        if (model2_mame_polygon_exportable(c->prims[pi].attr))
            n += (unsigned)(c->prims[pi].n_indices - 2);
    em->tris = (ex_tri_t *)calloc(n ? n : 1u, sizeof(ex_tri_t));
    if (!em->tris)
        return -1;
    for (pi = 0; pi < c->n_prims; pi++) {
        const model2_mesh_prim_t *p = &c->prims[pi];
        int k;

        if (!model2_mame_polygon_exportable(p->attr))
            continue;
        for (k = 1; k + 1 < p->n_indices; k++) {
            ex_tri_t *t = &em->tris[em->n_tris];
            int idx[3] = {p->indices[0], p->indices[k], p->indices[k + 1]};
            int j, ok = 1;

            for (j = 0; j < 3; j++)
                if (idx[j] < 0 || (unsigned)idx[j] >= c->n_verts)
                    ok = 0;
            if (!ok)
                continue;
            for (j = 0; j < 3; j++) {
                const model2_poly_vertex_t *v = &c->verts[idx[j]];

                t->p[3 * j] = v->x;
                t->p[3 * j + 1] = v->y;
                t->p[3 * j + 2] = v->z;
                if (v->x < mn[0]) mn[0] = v->x;
                if (v->y < mn[1]) mn[1] = v->y;
                if (v->z < mn[2]) mn[2] = v->z;
                if (v->x > mx[0]) mx[0] = v->x;
                if (v->y > mx[1]) mx[1] = v->y;
                if (v->z > mx[2]) mx[2] = v->z;
            }
            t->tex_valid = (u8)p->tex_valid;
            t->colorbase = p->tex_valid ? p->colorbase : (u16)((p->attr >> 16) & 0x3ffu);
            t->lumabase = p->lumabase;
            t->patch_x = p->patch_x;
            t->patch_y = p->patch_y;
            t->patch_w = p->patch_w;
            t->patch_h = p->patch_h;
            t->sheet = p->sheet;
            t->tex_flags = p->tex_flags;
            t->luma = param_luma(st, p->attr);
            if (s_cur_fprims && pi < s_cur_fleft && s_cur_fprims[pi].attr == p->attr) {
                t->luma = s_cur_fprims[pi].poly_luma;
                s_luma_from_frame++;
            } else {
                s_luma_fallback++;
            }
            {
                const model2_texture_param_t *tp = &st->texture_parameters[(p->attr >> 18) & 0x1fu];
                float nl = sqrtf(p->nx * p->nx + p->ny * p->ny + p->nz * p->nz);
                int textured = p->tex_valid && (p->tex_flags & MODEL2_GEO_TEX_TEXTURED);
                /* Quantised to 16 steps to bound the texture variants (in-game lumas: 150..255). */
                const int lit = lit_oba(em->oba);
                u8 lq = lit ? 255 : (u8)(t->luma >= 248 ? 255 : ((t->luma + 8) & ~15));
                int ti = textured ? texture_for(p, lq) : -1;
                int cs[3] = {0, k, k + 1};

                t->n[0] = nl > 0.f ? p->nx / nl : 0.f;
                t->n[1] = nl > 0.f ? p->ny / nl : 0.f;
                t->n[2] = nl > 0.f ? p->nz / nl : 0.f;
                t->amb = tp->ambient;
                t->dif = tp->diffuse;
                t->spec_ctrl = (u8)tp->specular_control;
                t->spec_scale = tp->specular_scale;
                t->mode_spec = (u8)(st->mode & 1u);
                t->light_table = lit ? light_table_for(textured, t->colorbase, t->lumabase) : 0;
                t->two_sided = (u8)((p->attr >> 17) & 1u);
                t->prim = pi;
                t->tflags = t->two_sided ? SR_TRI_TWO_SIDED : 0;
                if (ti >= 0) {
                    float pw = (float)(p->patch_w ? p->patch_w : 32), ph = (float)(p->patch_h ? p->patch_h : 32);

                    t->tex = (u16)ti;
                    t->base_rgb = 0xffffffu;
                    if (s_tex[ti].has_cut)
                        t->tflags |= SR_TRI_CUTOUT;
                    for (j = 0; j < 3; j++) {
                        t->uv[2 * j] = p->u[cs[j]] / pw;
                        t->uv[2 * j + 1] = p->v[cs[j]] / ph;
                    }
                } else {
                    u8 rgb[3];

                    t->tex = SR_NO_TEXTURE;
                    model2_palette_lookup_solid(t->colorbase, lit ? 255 : t->luma, rgb);
                    t->base_rgb = ((u32)rgb[0] << 16) | ((u32)rgb[1] << 8) | rgb[2];
                }
            }
            em->n_tris++;
        }
    }
    if (em->n_tris == 0) {
        memset(em->bounds, 0, sizeof(em->bounds));
        return 0;
    }
    em->bounds[0] = 0.5f * (mn[0] + mx[0]);
    em->bounds[1] = 0.5f * (mn[1] + mx[1]);
    em->bounds[2] = 0.5f * (mn[2] + mx[2]);
    for (pi = 0; pi < em->n_tris; pi++) {
        int j;

        for (j = 0; j < 3; j++) {
            float dx = em->tris[pi].p[3 * j] - em->bounds[0];
            float dy = em->tris[pi].p[3 * j + 1] - em->bounds[1];
            float dz = em->tris[pi].p[3 * j + 2] - em->bounds[2];
            float d = dx * dx + dy * dy + dz * dz;

            if (d > r2)
                r2 = d;
        }
    }
    em->bounds[3] = sqrtf(r2);
    assign_layers(em);
    return 0;
}

static int find_mesh(u32 oba, u32 obc, int mode, u32 hash)
{
    unsigned i;

    for (i = 0; i < s_n_meshes; i++) {
        const ex_mesh_t *m = &s_meshes[i];

        if (m->oba == oba && m->obc == obc && m->mode == mode && m->hash == hash)
            return (int)i;
    }
    return -1;
}

static int add_mesh(model2_geo_dl_ctx_t *ctx, const model2_drawn_object_t *o,
                    const model2_geo_state_t *st, int ram)
{
    model2_geo_dl_result_t r;
    u32 hash = 0;
    int idx;
    ex_mesh_t *em;

    model2_geo_dl_result_init(&r);
    if (model2_geo_dl_decode_object(ctx, o->tpa, o->tha, o->oba, o->obc, o->mode, &r) != 0
        || r.mesh.n_verts == 0) {
        model2_geo_dl_result_free(&r);
        return -1;
    }
    if (ram) {
        hash = hash_mesh(&r.mesh);
        idx = find_mesh(o->oba, o->obc, o->mode, hash);
        if (idx >= 0) {
            model2_geo_dl_result_free(&r);
            return idx;
        }
    }
    if (s_n_meshes >= s_cap_meshes) {
        unsigned nc = s_cap_meshes ? s_cap_meshes * 2u : 256u;
        ex_mesh_t *nm = (ex_mesh_t *)realloc(s_meshes, nc * sizeof(*nm));

        if (!nm) {
            model2_geo_dl_result_free(&r);
            return -1;
        }
        s_meshes = nm;
        s_cap_meshes = nc;
    }
    em = &s_meshes[s_n_meshes];
    memset(em, 0, sizeof(*em));
    em->oba = o->oba;
    em->obc = o->obc;
    em->mode = o->mode;
    em->hash = hash;
    em->is_ram = ram;
    em->first_cluster = -1;
    em->out_index = -1;
    if (s_debug && r.mesh.n_prims)
        fprintf(stderr, "export: new mesh oba=%08x obc=%u tpa=%08x tha=%08x prims=%u attr0=%08x tex=%d flags=%#x "
                        "cb=%u lb=%u\n",
                (unsigned)o->oba, (unsigned)o->obc, (unsigned)o->tpa, (unsigned)o->tha, r.mesh.n_prims,
                (unsigned)r.mesh.prims[0].attr, r.mesh.prims[0].tex_valid, r.mesh.prims[0].tex_flags,
                r.mesh.prims[0].colorbase, r.mesh.prims[0].lumabase);
    em->n_prims_all = r.mesh.n_prims;
    if (mesh_from_collector(em, &r.mesh, st) != 0) {
        model2_geo_dl_result_free(&r);
        return -1;
    }
    model2_geo_dl_result_free(&r);
    return (int)s_n_meshes++;
}

/* ROM objects: cache (oba, obc, mode) -> mesh in a small open-addressed table. */
#define ROM_CACHE_CAP 8192u
static u32 s_rc_oba[ROM_CACHE_CAP], s_rc_obc[ROM_CACHE_CAP];
static int s_rc_mode[ROM_CACHE_CAP], s_rc_mesh[ROM_CACHE_CAP];
static u8 s_rc_used[ROM_CACHE_CAP];

static int rom_mesh(model2_geo_dl_ctx_t *ctx, const model2_drawn_object_t *o,
                    const model2_geo_state_t *st)
{
    u32 h = ((o->oba * 2654435761u) ^ (o->obc * 40503u) ^ (u32)o->mode) & (ROM_CACHE_CAP - 1u);

    while (s_rc_used[h]) {
        if (s_rc_oba[h] == o->oba && s_rc_obc[h] == o->obc && s_rc_mode[h] == o->mode)
            return s_rc_mesh[h];
        h = (h + 1u) & (ROM_CACHE_CAP - 1u);
    }
    s_rc_used[h] = 1;
    s_rc_oba[h] = o->oba;
    s_rc_obc[h] = o->obc;
    s_rc_mode[h] = o->mode;
    s_rc_mesh[h] = add_mesh(ctx, o, st, 0);
    return s_rc_mesh[h];
}

/* ---------------------------------------------------------------- clusters */

static unsigned cell_hash(u32 mesh, int x, int y, int z)
{
    u32 h = mesh * 73856093u ^ (u32)x * 19349663u ^ (u32)y * 83492791u ^ (u32)z * 2654435761u;

    return h & (s_cell_cap - 1u);
}

static int *cell_slot(u32 mesh, int x, int y, int z, int create)
{
    unsigned h;

    if (create && (s_cell_used + 1u) * 2u > s_cell_cap) {
        unsigned oc = s_cell_cap, nc = oc ? oc * 2u : 1u << 16, i;
        int *oh = s_cell_head, *ox = s_cell_key_xyz;
        u32 *om = s_cell_key_mesh;

        s_cell_head = (int *)malloc(nc * sizeof(int));
        s_cell_key_mesh = (u32 *)malloc(nc * sizeof(u32));
        s_cell_key_xyz = (int *)malloc(nc * 3u * sizeof(int));
        if (!s_cell_head || !s_cell_key_mesh || !s_cell_key_xyz) {
            fprintf(stderr, "lift: export-course: out of memory\n");
            exit(1);
        }
        for (i = 0; i < nc; i++)
            s_cell_head[i] = -2; /* empty */
        s_cell_cap = nc;
        for (i = 0; i < oc; i++) {
            unsigned nh;

            if (oh[i] == -2)
                continue;
            nh = cell_hash(om[i], ox[3 * i], ox[3 * i + 1], ox[3 * i + 2]);
            while (s_cell_head[nh] != -2)
                nh = (nh + 1u) & (nc - 1u);
            s_cell_head[nh] = oh[i];
            s_cell_key_mesh[nh] = om[i];
            memcpy(&s_cell_key_xyz[3 * nh], &ox[3 * i], 3 * sizeof(int));
        }
        free(oh);
        free(om);
        free(ox);
    }
    if (!s_cell_cap)
        return NULL;
    h = cell_hash(mesh, x, y, z);
    while (s_cell_head[h] != -2) {
        if (s_cell_key_mesh[h] == mesh && s_cell_key_xyz[3 * h] == x
            && s_cell_key_xyz[3 * h + 1] == y && s_cell_key_xyz[3 * h + 2] == z)
            return &s_cell_head[h];
        h = (h + 1u) & (s_cell_cap - 1u);
    }
    if (!create)
        return NULL;
    s_cell_head[h] = -1;
    s_cell_key_mesh[h] = mesh;
    s_cell_key_xyz[3 * h] = x;
    s_cell_key_xyz[3 * h + 1] = y;
    s_cell_key_xyz[3 * h + 2] = z;
    s_cell_used++;
    return &s_cell_head[h];
}

static int cluster_match(const ex_cluster_t *c, const float w[12])
{
    int i;

    for (i = 0; i < 9; i++)
        if (fabsf(c->w[i] - w[i]) > EXPORT_R_TOL)
            return 0;
    for (i = 9; i < 12; i++)
        if (fabsf(c->w[i] - w[i]) > EXPORT_T_TOL)
            return 0;
    return 1;
}

static void add_draw(u32 mesh, const float w[12])
{
    int cx = (int)floorf(w[9] / EXPORT_CELL), cy = (int)floorf(w[10] / EXPORT_CELL),
        cz = (int)floorf(w[11] / EXPORT_CELL);
    int dx, dy, dz, *slot;
    ex_cluster_t *c;

    s_meshes[mesh].draws++;
    for (dx = -1; dx <= 1; dx++)
        for (dy = -1; dy <= 1; dy++)
            for (dz = -1; dz <= 1; dz++) {
                int ci;

                slot = cell_slot(mesh, cx + dx, cy + dy, cz + dz, 0);
                if (!slot)
                    continue;
                for (ci = *slot; ci >= 0; ci = s_clusters[ci].next_in_cell) {
                    c = &s_clusters[ci];
                    if (!cluster_match(c, w))
                        continue;
                    if (c->last_frame != s_frame) {
                        c->frames++;
                        c->last_frame = s_frame;
                    }
                    return;
                }
            }
    if (s_n_clusters >= s_cap_clusters) {
        unsigned nc = s_cap_clusters ? s_cap_clusters * 2u : 4096u;
        ex_cluster_t *ncl = (ex_cluster_t *)realloc(s_clusters, nc * sizeof(*ncl));

        if (!ncl) {
            fprintf(stderr, "lift: export-course: out of memory\n");
            exit(1);
        }
        s_clusters = ncl;
        s_cap_clusters = nc;
    }
    c = &s_clusters[s_n_clusters];
    memcpy(c->w, w, sizeof(c->w));
    c->mesh = mesh;
    c->frames = 1;
    c->last_frame = s_frame;
    c->cx = cx;
    c->cy = cy;
    c->cz = cz;
    c->next_in_mesh = s_meshes[mesh].first_cluster;
    s_meshes[mesh].first_cluster = (int)s_n_clusters;
    slot = cell_slot(mesh, cx, cy, cz, 1);
    c->next_in_cell = *slot;
    *slot = (int)s_n_clusters;
    s_n_clusters++;
}

/* ----------------------------------------------------------------- capture */

static void node_xz(u32 node, float out[2])
{
    out[0] = model2_u2f(i960_ld_u32(I960_ABS, node + 0x14u, 0));
    out[1] = model2_u2f(i960_ld_u32(I960_ABS, node + 0x1cu, 0));
}


static void check_world(const float w[12])
{
    int c;
    double s[3];

    for (c = 0; c < 3; c++)
        s[c] = sqrt((double)w[3 * c] * w[3 * c] + (double)w[3 * c + 1] * w[3 * c + 1]
                    + (double)w[3 * c + 2] * w[3 * c + 2]);
    for (c = 0; c < 3; c++) {
        double d01 = (w[3 * c] * w[3 * ((c + 1) % 3)] + w[3 * c + 1] * w[3 * ((c + 1) % 3) + 1]
                      + w[3 * c + 2] * w[3 * ((c + 1) % 3) + 2])
                     / (s[c] * s[(c + 1) % 3] + 1e-30);

        if (fabs(d01) > s_ortho_err_max)
            s_ortho_err_max = fabs(d01);
        if (s[c] < s_scale_min)
            s_scale_min = s[c];
        if (s[c] > s_scale_max)
            s_scale_max = s[c];
    }
}

static void render_frame(const model2_geo_state_t *st, const model2_mesh_collector_t *mesh, int tick);

static void capture_cb(void *user, model2_geo_dl_ctx_t *ctx, const model2_geo_state_t *st,
                       const model2_drawn_object_t *objs, unsigned n,
                       const model2_mesh_collector_t *frame_mesh)
{
    float vinv[12];
    unsigned i, poff = 0, dyn_n = 0;
    long dyn_count_at = -1;

    (void)user;
    if (s_render_tick >= 0)
        render_frame(st, frame_mesh, s_render_tick);
    if (s_debug && (s_frame % 1000u) == 0u) {
        unsigned ti, pi, used[32] = {0}, lmin[32], lmax[32];

        for (ti = 0; ti < 32u; ti++) {
            lmin[ti] = 255;
            lmax[ti] = 0;
        }
        for (pi = 0; pi < frame_mesh->n_prims; pi++) {
            unsigned set = (frame_mesh->prims[pi].attr >> 18) & 0x1fu, L = frame_mesh->prims[pi].poly_luma;

            used[set]++;
            if (L < lmin[set]) lmin[set] = L;
            if (L > lmax[set]) lmax[set] = L;
        }
        fprintf(stderr, "export: frame %u light view (%.3f,%.3f,%.3f) sets:", (unsigned)s_frame, st->light.x,
                st->light.y, st->light.z);
        for (ti = 0; ti < 32u; ti++)
            if (used[ti])
                fprintf(stderr, " [%u amb %.0f dif %.0f n %u luma %u..%u]", ti, st->texture_parameters[ti].ambient,
                        st->texture_parameters[ti].diffuse, used[ti], lmin[ti], lmax[ti]);
        fprintf(stderr, "\n");
    }
    if (!s_pack)
        return;
    if (mat_inverse(vinv, s_view) != 0)
        return;
    if (s_dyn) {
        u32 hdr[2] = {s_frame, (u32)s_rtick}, zero = 0;
        u8 buf[0x100];
        unsigned k2, b;

        fwrite("F", 1, 1, s_dyn);
        fwrite(hdr, 4, 2, s_dyn);
        for (k2 = 0; k2 < 8u; k2++) {
            for (b = 0; b < 0x58u; b += 4)
                *(u32 *)(buf + b) = i960_ld_u32(I960_ABS, 0x213b40u + k2 * 0x58u + b, 0);
            fwrite(buf, 1, 0x58, s_dyn);
        }
        for (k2 = 0; k2 < 4u; k2++) {
            for (b = 0; b < 0x100u; b += 4)
                *(u32 *)(buf + b) = i960_ld_u32(I960_ABS, 0x500000u + k2 * 0x100u + b, 0);
            fwrite(buf, 1, 0x100, s_dyn);
        }
        dyn_count_at = ftell(s_dyn);
        fwrite(&zero, 4, 1, s_dyn);
    }
    for (i = 0; i < n; i++) {
        const model2_drawn_object_t *o = &objs[i];
        int ram = !(o->oba & 0x00800000u) || (o->oba & 0x01000000u);
        int mi;
        float w[12];
        int k, finite = 1, dyn_draw = 0;

        s_cur_fprims = poff < frame_mesh->n_prims ? frame_mesh->prims + poff : NULL;
        s_cur_fleft = poff < frame_mesh->n_prims ? frame_mesh->n_prims - poff : 0u;
        mi = ram ? add_mesh(ctx, o, st, 1) : rom_mesh(ctx, o, st);
        s_cur_fprims = NULL;
        if (mi >= 0)
            poff += s_meshes[mi].n_prims_all;

        if (mi < 0 || s_meshes[mi].n_tris == 0)
            continue;
        if (s_dyn)
            dyn_draw = 1;
        mat_mul(w, vinv, o->matrix);
        for (k = 0; k < 12; k++)
            if (!isfinite(w[k]))
                finite = 0;
        if (!finite)
            continue;
        check_world(w);
        if (s_dyn && dyn_draw) {
            u32 rec[3] = {o->oba, o->obc, s_meshes[mi].n_tris};

            fwrite(rec, 4, 3, s_dyn);
            fwrite(w, 4, 12, s_dyn);
            dyn_n++;
        }
        add_draw((u32)mi, w);
        s_draws_total++;
        if (s_debug && (s_frame % 200u) == 0u && (o->oba == 0x00815599u || o->oba == 0x00814e11u)) {
            float a[2], b[2];

            node_xz(0x500000u, a);
            node_xz(0x500100u, b);
            fprintf(stderr,
                    "export: frame %u body oba=%#x world T=(%.3f,%.3f,%.3f) player node x,z=(%.3f,%.3f) "
                    "road car node x,z=(%.3f,%.3f)\n",
                    (unsigned)s_frame, (unsigned)o->oba, w[9], w[10], w[11], a[0], a[1], b[0], b[1]);
        }
    }
    if (s_dyn && dyn_count_at >= 0) {
        long end = ftell(s_dyn);

        fseek(s_dyn, dyn_count_at, SEEK_SET);
        fwrite(&dyn_n, 4, 1, s_dyn);
        fseek(s_dyn, end, SEEK_SET);
    }
}

/* --------------------------------------------------------------- autopilot */

#define AP_PATH_MAX 65536u
static float s_ap_path[AP_PATH_MAX][2];
static unsigned s_ap_n;
static unsigned s_ap_near;
static float s_ap_prev[2];
static int s_ap_have_prev;
static unsigned s_ap_gear = 1;
static unsigned s_ap_shift_hold;
static int s_countdown; /* I960_HOST_EXPORT_COUNTDOWN=1: capture the countdown before GO as well */
static FILE *s_ap_out;      /* I960_HOST_EXPORT_AUTOPILOT_INPUTS */
static int s_ap_last[4] = {-1, -1, -1, -1};

/* The controls set at the end of race tick s_rtick are what the next tick's poll samples. */
static void autopilot_dump(int steer, unsigned accel, unsigned brake, unsigned gear)
{
    if (!s_ap_out || (s_ap_last[0] == steer && s_ap_last[1] == (int)accel && s_ap_last[2] == (int)brake
                      && s_ap_last[3] == (int)gear))
        return;
    s_ap_last[0] = steer;
    s_ap_last[1] = (int)accel;
    s_ap_last[2] = (int)brake;
    s_ap_last[3] = (int)gear;
    fprintf(s_ap_out, "%d 0x%02x 0x%02x 0x%02x %u\n", s_rtick + 1, (unsigned)steer, accel, brake, gear);
    fflush(s_ap_out);
}

static void autopilot_tick(void)
{
    float ai[2], me[2], hd[2], tgt[2];
    float speed = 0.f, look, cross, dot, ang;
    unsigned i, best, end;
    float bestd = 1e30f;
    int steer;
    u8 accel = 0xff, brake = 0;

    node_xz(0x500100u, ai);
    node_xz(0x500000u, me);
    if (s_ap_n == 0 || (fabsf(ai[0] - s_ap_path[s_ap_n - 1][0])
                        + fabsf(ai[1] - s_ap_path[s_ap_n - 1][1])) > 0.5f) {
        if (s_ap_n < AP_PATH_MAX) {
            s_ap_path[s_ap_n][0] = ai[0];
            s_ap_path[s_ap_n][1] = ai[1];
            s_ap_n++;
        }
    }
    if (s_ap_have_prev) {
        hd[0] = me[0] - s_ap_prev[0];
        hd[1] = me[1] - s_ap_prev[1];
        speed = sqrtf(hd[0] * hd[0] + hd[1] * hd[1]);
    } else {
        hd[0] = hd[1] = 0.f;
    }
    s_ap_prev[0] = me[0];
    s_ap_prev[1] = me[1];
    s_ap_have_prev = 1;

    /* Nearest path point, searched forward from the last one. */
    end = s_ap_near + 400u < s_ap_n ? s_ap_near + 400u : s_ap_n;
    best = s_ap_near;
    for (i = s_ap_near; i < end; i++) {
        float dx = s_ap_path[i][0] - me[0], dz = s_ap_path[i][1] - me[1];
        float d = dx * dx + dz * dz;

        if (d < bestd) {
            bestd = d;
            best = i;
        }
    }
    if (bestd > 8.f * 8.f) {
        /* Lost the path (a missed hairpin): find it again near where the car is, anywhere from a little behind to
         * far ahead, so the forward search does not stay stuck on a point the car has left. */
        unsigned j, from = s_ap_near > 600u ? s_ap_near - 600u : 0u, to = s_ap_near + 3000u < s_ap_n ? s_ap_near + 3000u : s_ap_n;

        for (j = from; j < to; j++) {
            float dx = s_ap_path[j][0] - me[0], dz = s_ap_path[j][1] - me[1];
            float d = dx * dx + dz * dz;

            if (d < bestd) {
                bestd = d;
                best = j;
            }
        }
    }
    s_ap_near = best;
    /* Look-ahead along the path (in samples of ~0.5 units). */
    look = 12.f + speed * 18.f;
    i = best + (unsigned)(look / 0.5f);
    if (i >= s_ap_n && s_ap_n > 400u) {
        /* The road car has stopped (its race is over): the path is its laps, so go on from the nearest point of an
         * earlier lap instead of easing off at its end. */
        unsigned j, wrap = s_ap_near;
        float wd = 1e30f;

        for (j = 0; j + 400u < s_ap_n; j++) {
            float dx = s_ap_path[j][0] - me[0], dz = s_ap_path[j][1] - me[1];
            float d = dx * dx + dz * dz;

            if (d < wd) {
                wd = d;
                wrap = j;
            }
        }
        if (wd < 36.f) {
            s_ap_near = best = wrap;
            i = best + (unsigned)(look / 0.5f);
        }
    }
    if (i >= s_ap_n) {
        i = s_ap_n ? s_ap_n - 1u : 0u;
        accel = 0x40; /* caught up with the path's end: ease off */
    }
    tgt[0] = s_ap_path[i][0] - me[0];
    tgt[1] = s_ap_path[i][1] - me[1];
    steer = 0x80;
    if (speed > 0.02f) {
        float tl = sqrtf(tgt[0] * tgt[0] + tgt[1] * tgt[1]) + 1e-6f;

        cross = (hd[0] * tgt[1] - hd[1] * tgt[0]) / (speed * tl);
        dot = (hd[0] * tgt[0] + hd[1] * tgt[1]) / (speed * tl);
        ang = atan2f(cross, dot);
        steer = 0x80 - (int)(ang * 260.f);
        if (steer < 0)
            steer = 0;
        if (steer > 255)
            steer = 255;
        if (fabsf(ang) > 0.6f && speed > 0.7f) {
            accel = 0;
            brake = 0xc0;
        }
    }
    /* Off the path (> 6 units): slow down so the car can turn back. */
    if (bestd > 36.f && speed > 0.5f) {
        accel = 0x30;
    }
    if (s_ap_shift_hold)
        s_ap_shift_hold--;
    else {
        /* Up at ~85% of each gear's top speed (gear 1 tops out near 0.34/tick). */
        static const float up[4] = {0.28f, 0.45f, 0.65f, 1e9f};
        static const float down[4] = {-1.f, 0.20f, 0.33f, 0.48f};

        if (speed > up[s_ap_gear - 1u] && s_ap_gear < 4u) {
            s_ap_gear++;
            s_ap_shift_hold = 15;
        } else if (speed < down[s_ap_gear - 1u] && s_ap_gear > 1u) {
            s_ap_gear--;
            s_ap_shift_hold = 15;
        }
    }
    model2_io_analog_set(MODEL2_IO_AN_STEER, (u8)steer);
    model2_io_analog_set(MODEL2_IO_AN_ACCEL, accel);
    model2_io_analog_set(MODEL2_IO_AN_BRAKE, brake);
    model2_io_shifter_set((u8)s_ap_gear);
    autopilot_dump(steer, accel, brake, s_ap_gear);
    if (s_debug && (s_frame % 120u) == 0u)
        fprintf(stderr, "export: ap frame %u me=(%.1f,%.1f) ai=(%.1f,%.1f) path=%u near=%u "
                        "d=%.1f speed=%.2f steer=%d gear=%u\n",
                (unsigned)s_frame, me[0], me[1], ai[0], ai[1], s_ap_n, s_ap_near,
                sqrtf(bestd), speed, steer, s_ap_gear);
}

/* ------------------------------------------------------------------ output */


static u32 tri_colour(const ex_tri_t *t)
{
    unsigned i;
    u8 rgb[3];
    u32 argb;
    int textured = t->tex_valid && (t->tex_flags & MODEL2_GEO_TEX_TEXTURED);

    for (i = 0; i < s_cc_n; i++) {
        const colour_cache_t *c = &s_cc[i];

        if (c->colorbase == t->colorbase && c->luma == t->luma && c->tex == (u8)textured
            && (!textured
                || (c->lumabase == t->lumabase && c->patch_x == t->patch_x
                    && c->patch_y == t->patch_y && c->patch_w == t->patch_w
                    && c->patch_h == t->patch_h && c->sheet == t->sheet
                    && c->flags == t->tex_flags)))
            return c->argb;
    }
    if (textured) {
        const u32 *sheet = model2_tex_sheet_bank(t->sheet);
        unsigned sx = t->patch_w > 64u ? t->patch_w / 64u : 1u;
        unsigned sy = t->patch_h > 64u ? t->patch_h / 64u : 1u;
        unsigned x, y, n = 0;
        unsigned long acc[3] = {0, 0, 0};

        if (sheet) {
            for (y = 0; y < t->patch_h; y += sy)
                for (x = 0; x < t->patch_w; x += sx) {
                    u16 tx = model2_get_texel(sheet, t->patch_x, t->patch_y, (int)x, (int)y);
                    u8 rgba[4];

                    if ((t->tex_flags & MODEL2_GEO_TEX_CUTOUT) && tx == 0x0f)
                        continue;
                    model2_palette_lookup_texel(tx, t->colorbase, t->lumabase, t->luma, 0, rgba);
                    acc[0] += rgba[0];
                    acc[1] += rgba[1];
                    acc[2] += rgba[2];
                    n++;
                }
        }
        if (n) {
            rgb[0] = (u8)(acc[0] / n);
            rgb[1] = (u8)(acc[1] / n);
            rgb[2] = (u8)(acc[2] / n);
        } else {
            model2_palette_lookup_solid(t->colorbase, t->luma, rgb);
        }
    } else {
        model2_palette_lookup_solid(t->colorbase, t->luma, rgb);
    }
    argb = 0xff000000u | ((u32)rgb[0] << 16) | ((u32)rgb[1] << 8) | rgb[2];
    if (s_cc_n >= s_cc_cap) {
        unsigned nc = s_cc_cap ? s_cc_cap * 2u : 256u;
        colour_cache_t *ncc = (colour_cache_t *)realloc(s_cc, nc * sizeof(*ncc));

        if (!ncc)
            return argb;
        s_cc = ncc;
        s_cc_cap = nc;
    }
    s_cc[s_cc_n].colorbase = t->colorbase;
    s_cc[s_cc_n].lumabase = t->lumabase;
    s_cc[s_cc_n].patch_x = t->patch_x;
    s_cc[s_cc_n].patch_y = t->patch_y;
    s_cc[s_cc_n].patch_w = t->patch_w;
    s_cc[s_cc_n].patch_h = t->patch_h;
    s_cc[s_cc_n].sheet = t->sheet;
    s_cc[s_cc_n].flags = t->tex_flags;
    s_cc[s_cc_n].luma = t->luma;
    s_cc[s_cc_n].tex = (u8)textured;
    s_cc[s_cc_n].argb = argb;
    s_cc_n++;
    return argb;
}

static int put32(FILE *f, u32 v);
static int putf(FILE *f, float v);

/* PACK.light (the pack's name, .course -> .light): what the console needs to light the cars per frame as the GEO
 * does (MAME geo_parse luma = |N.L| x diffuse + ambient (+ specular), 0 when the light and the eye are on either
 * side): "SRLIT01\0", u32 n_tables, n_tables x 17 floats k(L) (L = 0, 16, .. 256: the brightness of a colour baked
 * at luma 255 shown at luma L), u32 n_meshes, per mesh u32 oba, u32 n_tris, per triangle (in the pack's order)
 * float ambient, diffuse, specular_scale, u8 specular_control, u8 the GEO's specular mode bit, u16 table. The
 * normals come from the triangles themselves (their sign does not matter to the formula). */
static void write_light_file(unsigned n_out_meshes)
{
    char path[1100];
    FILE *f;
    unsigned oi, i, n_car = 0;
    int ok;
    size_t len = strlen(s_path);

    if (len > 7 && strcmp(s_path + len - 7, ".course") == 0)
        snprintf(path, sizeof(path), "%.*s.light", (int)(len - 7), s_path);
    else
        snprintf(path, sizeof(path), "%s.light", s_path);
    for (oi = 0; oi < n_out_meshes; oi++)
        for (i = 0; i < s_n_meshes; i++)
            if (s_meshes[i].out_index == (int)oi && lit_oba(s_meshes[i].oba))
                n_car++;
    f = fopen(path, "wb");
    if (!f)
        return;
    ok = fwrite("SRLIT01\0", 1, 8, f) == 8 && put32(f, s_n_light_tables);
    for (i = 0; i < s_n_light_tables && ok; i++) {
        unsigned k;
        for (k = 0; k < 17 && ok; k++)
            ok = putf(f, s_light_tables[i].k[k]);
    }
    ok = ok && put32(f, n_car);
    for (oi = 0; oi < n_out_meshes && ok; oi++)
        for (i = 0; i < s_n_meshes && ok; i++) {
            const ex_mesh_t *m = &s_meshes[i];
            unsigned t;

            if (m->out_index != (int)oi || !lit_oba(m->oba))
                continue;
            ok = put32(f, m->oba) && put32(f, m->n_tris);
            for (t = 0; t < m->n_tris && ok; t++) {
                const ex_tri_t *tr = &m->tris[t];
                const u8 b[2] = {tr->spec_ctrl, tr->mode_spec};

                ok = putf(f, tr->amb) && putf(f, tr->dif) && putf(f, tr->spec_scale) && fwrite(b, 1, 2, f) == 2
                     && fwrite(&tr->light_table, 2, 1, f) == 1;
            }
        }
    fclose(f);
    fprintf(stderr, "lift: export-course light %s: %u car meshes, %u brightness tables%s\n", path, n_car,
            s_n_light_tables, ok ? "" : " (write failed)");
}

static int put32(FILE *f, u32 v)
{
    return fwrite(&v, 4, 1, f) == 1;
}

static int putf(FILE *f, float v)
{
    return fwrite(&v, 4, 1, f) == 1;
}

/* Static clusters of one mesh within EXPORT_ISOLATE of each other form a site. Scenery copies of a mesh never
 * overlap, so a site is either an animated object standing in one place (a few poses: keep the pose drawn in the
 * most frames) or a chain left by something crawling to a stop (a car's parts at the finish: more than
 * EXPORT_SITE_MAX poses, drop it). */
#define EXPORT_ISOLATE  1.0f
#define EXPORT_SITE_MAX 4
static int cluster_keep(unsigned ci)
{
    const ex_cluster_t *c = &s_clusters[ci];
    int r = (int)ceilf(EXPORT_ISOLATE / EXPORT_CELL) + 1, dx, dy, dz;
    int site = 1, dominant = 1;

    for (dx = -r; dx <= r; dx++)
        for (dy = -r; dy <= r; dy++)
            for (dz = -r; dz <= r; dz++) {
                int *slot = cell_slot(c->mesh, c->cx + dx, c->cy + dy, c->cz + dz, 0);
                int oi;

                if (!slot)
                    continue;
                for (oi = *slot; oi >= 0; oi = s_clusters[oi].next_in_cell) {
                    const ex_cluster_t *o = &s_clusters[oi];
                    float ddx, ddy, ddz;

                    if ((unsigned)oi == ci || o->frames < s_static_frames)
                        continue;
                    ddx = o->w[9] - c->w[9];
                    ddy = o->w[10] - c->w[10];
                    ddz = o->w[11] - c->w[11];
                    if (ddx * ddx + ddy * ddy + ddz * ddz >= EXPORT_ISOLATE * EXPORT_ISOLATE)
                        continue;
                    site++;
                    if (o->frames > c->frames || (o->frames == c->frames && (unsigned)oi < ci))
                        dominant = 0;
                }
            }
    return dominant && site <= EXPORT_SITE_MAX;
}

static void export_write(void)
{
    FILE *f;
    unsigned i, n_out_meshes = 0, n_static = 0, n_dyn = 0, tris = 0;
    unsigned ci;
    int ok = 1;
    float wmn[3] = {1e30f, 1e30f, 1e30f}, wmx[3] = {-1e30f, -1e30f, -1e30f};
    typedef struct {
        u32 mesh, flags;
        const float *w;
    } out_inst_t;
    out_inst_t *inst;
    unsigned n_inst = 0;
    int *tex_remap = NULL;
    unsigned *tex_order = (unsigned *)malloc((s_n_tex + 1u) * sizeof(unsigned));
    unsigned n_out_tex = 0, n_flat = 0, n_cut = 0, n_texd = 0, layer_hist[16] = {0};
    unsigned long tex_bytes = 0;

    if (!s_pack || !s_path[0])
        return;
    inst = (out_inst_t *)malloc((s_n_clusters + s_n_meshes + 1u) * sizeof(*inst));
    if (!inst)
        return;
    /* One-frame draws per mesh: tells moving objects from scenery. */
    for (ci = 0; ci < s_n_clusters; ci++)
        if (s_clusters[ci].frames == 1)
            s_meshes[s_clusters[ci].mesh].single++;
    for (i = 0; i < s_n_meshes; i++) {
        ex_mesh_t *m = &s_meshes[i];
        int moving = m->draws && (double)m->single / (double)m->draws > 0.3;
        int have_static = 0;
        int best = -1;

        for (ci = (unsigned)m->first_cluster; (int)ci >= 0;
             ci = (unsigned)s_clusters[ci].next_in_mesh) {
            const ex_cluster_t *c = &s_clusters[ci];

            if (best < 0 || c->frames > s_clusters[best].frames)
                best = (int)ci;
            if (moving || c->frames < s_static_frames)
                continue;
            if (!cluster_keep(ci)) {
                if (s_debug)
                    fprintf(stderr, "export: chain oba=%08x frames=%u T=(%.2f,%.2f,%.2f) R0=(%.3f,%.3f,%.3f)\n",
                            (unsigned)m->oba, (unsigned)c->frames, c->w[9], c->w[10], c->w[11], c->w[0],
                            c->w[1], c->w[2]);
                continue;
            }
            if (m->out_index < 0)
                m->out_index = (int)n_out_meshes++;
            inst[n_inst].mesh = (u32)m->out_index;
            inst[n_inst].flags = SR_INST_STATIC;
            inst[n_inst].w = c->w;
            if (!m->rep)
                m->rep = c->w;
            n_inst++;
            n_static++;
            have_static = 1;
        }
        if (!have_static && best >= 0) {
            if (m->out_index < 0)
                m->out_index = (int)n_out_meshes++;
            inst[n_inst].mesh = (u32)m->out_index;
            inst[n_inst].flags = 0;
            inst[n_inst].w = s_clusters[best].w;
            m->rep = s_clusters[best].w;
            n_inst++;
            n_dyn++;
        }
    }
    if (s_debug) {
        for (i = 0; i < s_n_meshes; i++) {
            const ex_mesh_t *m = &s_meshes[i];
            unsigned ncl = 0, maxf = 0;

            for (ci = (unsigned)m->first_cluster; (int)ci >= 0;
                 ci = (unsigned)s_clusters[ci].next_in_mesh) {
                ncl++;
                if (s_clusters[ci].frames > maxf)
                    maxf = s_clusters[ci].frames;
            }
            fprintf(stderr,
                    "export: mesh %3u oba=%08x obc=%5u mode=%d %s tris=%5u draws=%6u single=%6u "
                    "clusters=%5u maxframes=%5u out=%d r=%.1f\n",
                    i, (unsigned)m->oba, (unsigned)m->obc, m->mode, m->is_ram ? "RAM" : "ROM",
                    m->n_tris, m->draws, m->single, ncl, maxf, m->out_index, m->bounds[3]);
        }
    }
    f = fopen(s_path, "wb");
    if (!f) {
        fprintf(stderr, "lift: export-course %s: cannot open\n", s_path);
        free(inst);
        free(tex_order);
        return;
    }
    /* Textures used by the exported meshes, in first-use order. */
    {
        unsigned oi, t;

        tex_remap = (int *)malloc((s_n_tex + 1u) * sizeof(int));
        for (t = 0; t < s_n_tex; t++)
            tex_remap[t] = -1;
        for (oi = 0; oi < n_out_meshes; oi++)
            for (i = 0; i < s_n_meshes; i++)
                if (s_meshes[i].out_index == (int)oi) {
                    for (t = 0; t < s_meshes[i].n_tris; t++) {
                        u16 tx = s_meshes[i].tris[t].tex;

                        if (tx != SR_NO_TEXTURE && tex_remap[tx] < 0) {
                            tex_remap[tx] = (int)n_out_tex;
                            tex_order[n_out_tex++] = tx;
                        }
                    }
                    break;
                }
    }
    ok = fwrite("SRCRS02\0", 1, 8, f) == 8 && put32(f, n_out_tex) && put32(f, n_out_meshes) && put32(f, n_inst);
    for (i = 0; i < n_out_tex && ok; i++) {
        const ex_tex_t *t = &s_tex[tex_order[i]];
        u16 wh[2] = {t->w, t->h};

        ok = fwrite(wh, 2, 2, f) == 2 && put32(f, t->flags)
             && fwrite(t->texels, 2, (size_t)t->w * t->h, f) == (size_t)t->w * t->h;
        tex_bytes += (unsigned long)t->w * t->h * 2u;
    }
    {
        unsigned oi;

        for (oi = 0; oi < n_out_meshes && ok; oi++) {
            const ex_mesh_t *m = NULL;
            unsigned t;

            for (i = 0; i < s_n_meshes; i++)
                if (s_meshes[i].out_index == (int)oi) {
                    m = &s_meshes[i];
                    break;
                }
            ok = ok && put32(f, m->oba) && put32(f, m->obc) && put32(f, m->n_tris);
            for (t = 0; t < 4 && ok; t++)
                ok = putf(f, m->bounds[t]);
            for (t = 0; t < m->n_tris && ok; t++) {
                const ex_tri_t *tr = &m->tris[t];
                u32 argb;
                u16 hdr[2];
                int j;

                argb = 0xff000000u | tr->base_rgb; /* textured: white (luma baked); flat: palette colour at luma */
                hdr[0] = tr->tex == SR_NO_TEXTURE ? (u16)SR_NO_TEXTURE : (u16)tex_remap[tr->tex];
                hdr[1] = tr->tflags;
                ok = ok && fwrite(hdr, 2, 2, f) == 2;
                for (j = 0; j < 3 && ok; j++)
                    ok = putf(f, tr->p[3 * j]) && putf(f, tr->p[3 * j + 1]) && putf(f, tr->p[3 * j + 2])
                         && putf(f, tr->uv[2 * j]) && putf(f, tr->uv[2 * j + 1]) && put32(f, argb);
                if (tr->tex == SR_NO_TEXTURE)
                    n_flat++;
                else if (tr->tflags & SR_TRI_CUTOUT)
                    n_cut++;
                else
                    n_texd++;
                layer_hist[(tr->tflags & SR_TRI_LAYER_MASK) >> SR_TRI_LAYER_SHIFT]++;
            }
            tris += m->n_tris;
        }
    }
    for (i = 0; i < n_inst && ok; i++) {
        int k;

        ok = put32(f, inst[i].mesh) && put32(f, inst[i].flags);
        for (k = 0; k < 12 && ok; k++)
            ok = putf(f, inst[i].w[k]);
        if (inst[i].flags & SR_INST_STATIC)
            for (k = 0; k < 3; k++) {
                if (inst[i].w[9 + k] < wmn[k])
                    wmn[k] = inst[i].w[9 + k];
                if (inst[i].w[9 + k] > wmx[k])
                    wmx[k] = inst[i].w[9 + k];
            }
    }
    if (fclose(f) != 0)
        ok = 0;
    fprintf(stderr,
            "lift: export-course %s%s: %u frames, %u draws, %u meshes seen; wrote %u meshes "
            "(%u tris), %u static + %u dynamic instances; static T x=[%.1f,%.1f] y=[%.1f,%.1f] "
            "z=[%.1f,%.1f]; world R column scale [%.4f,%.4f] max |cos| %.2e\n",
            s_path, ok ? "" : " (WRITE FAILED)", (unsigned)s_frame, s_draws_total, s_n_meshes,
            n_out_meshes, tris, n_static, n_dyn, wmn[0], wmx[0], wmn[1], wmx[1], wmn[2], wmx[2],
            s_scale_min, s_scale_max, s_ortho_err_max);
    fprintf(stderr,
            "lift: export-course v2: %u textures (%lu bytes; %u decoded, %u patches clamped to 1024, %u without "
            "sheet); tris textured %u, cut-out %u, flat %u; layers",
            n_out_tex, tex_bytes, s_n_tex, s_tex_clamped, s_tex_no_sheet, n_texd, n_cut, n_flat);
    for (i = 0; i < 16; i++)
        if (layer_hist[i])
            fprintf(stderr, " %u:%u", i, layer_hist[i]);
    fprintf(stderr, "; polygon luma from the frame %u, fallback %u\n", s_luma_from_frame, s_luma_fallback);
    write_light_file(n_out_meshes);
    free(tex_remap);
    free(tex_order);
    free(inst);
}

/* ------------------------------------------------------------ frame render */

#define FR_W 640
#define FR_H 480
#define FR_ZNEAR 0.01f

static int signed12(u32 v)
{
    v &= 0xfffu;
    return (v & 0x800u) ? (int)v - 0x1000 : (int)v;
}

typedef struct {
    float x, y, z;
} fr_v3_t;

static void fr_fill(u8 *rgb, float *depth, const float sx[3], const float sy[3], const float iz[3], u32 argb)
{
    float minx = sx[0], maxx = sx[0], miny = sy[0], maxy = sy[0], area;
    int x0, x1, y0, y1, x, y, k;

    for (k = 1; k < 3; k++) {
        if (sx[k] < minx) minx = sx[k];
        if (sx[k] > maxx) maxx = sx[k];
        if (sy[k] < miny) miny = sy[k];
        if (sy[k] > maxy) maxy = sy[k];
    }
    area = (sx[1] - sx[0]) * (sy[2] - sy[0]) - (sx[2] - sx[0]) * (sy[1] - sy[0]);
    if (fabsf(area) < 1e-9f)
        return;
    x0 = minx < 0.f ? 0 : (int)floorf(minx);
    y0 = miny < 0.f ? 0 : (int)floorf(miny);
    x1 = maxx > FR_W - 1 ? FR_W - 1 : (int)ceilf(maxx);
    y1 = maxy > FR_H - 1 ? FR_H - 1 : (int)ceilf(maxy);
    for (y = y0; y <= y1; y++)
        for (x = x0; x <= x1; x++) {
            float px = (float)x + 0.5f, py = (float)y + 0.5f;
            float w0 = ((sx[1] - px) * (sy[2] - py) - (sx[2] - px) * (sy[1] - py)) / area;
            float w1 = ((sx[2] - px) * (sy[0] - py) - (sx[0] - px) * (sy[2] - py)) / area;
            float w2 = 1.f - w0 - w1, d;

            if (w0 < 0.f || w1 < 0.f || w2 < 0.f)
                continue;
            d = w0 * iz[0] + w1 * iz[1] + w2 * iz[2]; /* 1/z: larger is nearer */
            if (d <= depth[y * FR_W + x])
                continue;
            depth[y * FR_W + x] = d;
            rgb[3 * (y * FR_W + x)] = (u8)(argb >> 16);
            rgb[3 * (y * FR_W + x) + 1] = (u8)(argb >> 8);
            rgb[3 * (y * FR_W + x) + 2] = (u8)argb;
        }
}

static void render_frame(const model2_geo_state_t *st, const model2_mesh_collector_t *mesh, int tick)
{
    static u8 rgb[FR_W * FR_H * 3];
    static float depth[FR_W * FR_H];
    int v[4], c[2];
    float scx, scy, ox, oy;
    unsigned pi, ntri = 0, i;
    char path[1200];
    FILE *f;
    float view[12];
    int have_view;

    if (!st->window_valid || !(st->focus.x > 0.f) || !(st->focus.y > 0.f)) {
        fprintf(stderr, "lift: export frame %d: no GEO window/focus yet\n", tick);
        return;
    }
    v[0] = signed12(st->window[0] >> 16);
    v[1] = signed12(st->window[0]);
    v[2] = signed12(st->window[1] >> 16);
    v[3] = signed12(st->window[1]);
    c[0] = signed12(st->window[2] >> 16);
    c[1] = signed12(st->window[2]);
    if (v[2] <= v[0] || v[3] <= v[1])
        return;
    scx = (float)FR_W / (float)(v[2] - v[0]);
    scy = (float)FR_H / (float)(v[3] - v[1]);
    ox = (float)(c[0] - v[0]); /* column of the center, window pixels */
    oy = (float)(v[3] - c[1]); /* row from top of the center */
    for (i = 0; i < FR_W * FR_H; i++) {
        rgb[3 * i] = 90;
        rgb[3 * i + 1] = 140;
        rgb[3 * i + 2] = 200;
        depth[i] = 0.f;
    }
    for (pi = 0; pi < mesh->n_prims; pi++) {
        const model2_mesh_prim_t *p = &mesh->prims[pi];
        ex_tri_t t;
        fr_v3_t in[4], poly[8];
        int n = 0, k, ok = 1;

        if (!model2_mame_polygon_exportable(p->attr))
            continue;
        for (k = 0; k < p->n_indices; k++) {
            int ix = p->indices[k];

            if (ix < 0 || (unsigned)ix >= mesh->n_verts) {
                ok = 0;
                break;
            }
            in[k].x = mesh->verts[ix].x;
            in[k].y = mesh->verts[ix].y;
            in[k].z = mesh->verts[ix].z;
        }
        if (!ok)
            continue;
        /* Clip the polygon against z >= znear (GL: clip z = z - 2 znear, w = z). */
        for (k = 0; k < p->n_indices; k++) {
            const fr_v3_t *a = &in[k], *b = &in[(k + 1) % p->n_indices];
            int ain = a->z >= FR_ZNEAR, bin = b->z >= FR_ZNEAR;

            if (ain)
                poly[n++] = *a;
            if (ain != bin) {
                float u = (FR_ZNEAR - a->z) / (b->z - a->z);

                poly[n].x = a->x + u * (b->x - a->x);
                poly[n].y = a->y + u * (b->y - a->y);
                poly[n].z = FR_ZNEAR;
                n++;
            }
        }
        if (n < 3)
            continue;
        memset(&t, 0, sizeof(t));
        t.tex_valid = (u8)p->tex_valid;
        t.colorbase = p->tex_valid ? p->colorbase : (u16)((p->attr >> 16) & 0x3ffu);
        t.lumabase = p->lumabase;
        t.patch_x = p->patch_x;
        t.patch_y = p->patch_y;
        t.patch_w = p->patch_w;
        t.patch_h = p->patch_h;
        t.sheet = p->sheet;
        t.tex_flags = p->tex_flags;
        t.luma = param_luma(st, p->attr);
        t.argb = tri_colour(&t);
        for (k = 1; k + 1 < n; k++) {
            const fr_v3_t *q[3] = {&poly[0], &poly[k], &poly[k + 1]};
            float sx[3], sy[3], iz[3];
            int j;

            for (j = 0; j < 3; j++) {
                iz[j] = 1.f / q[j]->z;
                sx[j] = (ox + q[j]->x * iz[j]) * scx;
                sy[j] = (oy - q[j]->y * iz[j]) * scy;
            }
            fr_fill(rgb, depth, sx, sy, iz, t.argb);
            ntri++;
        }
    }
    snprintf(path, sizeof(path), "%s/frame@%d.ppm", s_frames_dir, tick);
    f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "lift: export frame %s: cannot open\n", path);
        return;
    }
    fprintf(f, "P6\n%d %d\n255\n", FR_W, FR_H);
    fwrite(rgb, 1, sizeof(rgb), f);
    fclose(f);
    have_view = model2_hw_view_matrix(view);
    snprintf(path, sizeof(path), "%s/frame@%d.txt", s_frames_dir, tick);
    f = fopen(path, "w");
    for (i = 0; i < 2; i++) {
        FILE *o = i ? stderr : f;

        if (!o)
            continue;
        fprintf(o,
                "frame@%d: race_tick %d, %u triangles; focus (%.6g, %.6g); window v0..v3 (%d, %d, %d, %d) = %dx%d; "
                "center (%d, %d); geo mode %d\n"
                "  screen (640x480): col = %.6g * ((%d) + x/z), row_from_top = %.6g * ((%d) - y/z)   "
                "[x, y view space after focus; view +y is screen up]\n"
                "  i.e. raw view space: col = %.6g + %.6g * x/z, row = %.6g - %.6g * y/z\n",
                tick, tick, ntri, st->focus.x, st->focus.y, v[0], v[1], v[2], v[3], v[2] - v[0], v[3] - v[1],
                c[0], c[1], st->mode, scx, c[0] - v[0], scy, v[3] - c[1], ox * scx, st->focus.x * scx,
                oy * scy, st->focus.y * scy);
        if (have_view)
            fprintf(o,
                    "  latched view (model2_hw_view_matrix, columns; T at 9..11):\n"
                    "    R col0 %.6f %.6f %.6f\n    R col1 %.6f %.6f %.6f\n    R col2 %.6f %.6f %.6f\n"
                    "    T      %.6f %.6f %.6f\n",
                    view[0], view[1], view[2], view[3], view[4], view[5], view[6], view[7], view[8], view[9],
                    view[10], view[11]);
    }
    if (f)
        fclose(f);
    fprintf(stderr, "lift: export frame -> %s/frame@%d.ppm\n", s_frames_dir, tick);
}

/* -------------------------------------------------------------------- tick */

static void export_init(void)
{
    const char *s = getenv("I960_HOST_EXPORT_FRAMES");
    const char *pack = getenv("I960_HOST_EXPORT_COURSE");

    s_enabled = 0;
    if (s && *s) {
        char *end;

        while (*s && s_n_frame_ticks < FRAMES_MAX) {
            long t = strtol(s, &end, 0);

            if (end == s) {
                s++;
                continue;
            }
            s_frame_ticks[s_n_frame_ticks++] = (int)t;
            s = end;
        }
        s = getenv("I960_HOST_EXPORT_FRAMES_DIR");
        if (s && *s)
            snprintf(s_frames_dir, sizeof(s_frames_dir), "%s", s);
        mkdir(s_frames_dir, 0755);
        s_enabled = s_n_frame_ticks > 0;
    }
    if (!pack || !*pack)
        return;
    snprintf(s_path, sizeof(s_path), "%s", pack);
    s_enabled = 1;
    s_pack = 1;
    s = getenv("I960_HOST_EXPORT_DEBUG");
    s_debug = s && *s && *s != '0';
    s = getenv("I960_HOST_EXPORT_AUTOPILOT");
    s_autopilot = s && *s && *s != '0' && !i960_host_race_scripted();
    s = getenv("I960_HOST_EXPORT_COUNTDOWN");
    s_countdown = s && *s && *s != '0';
    s = getenv("I960_HOST_EXPORT_AUTOPILOT_INPUTS");
    if (s_autopilot && s && *s) {
        s_ap_out = fopen(s, "w");
        if (!s_ap_out) {
            fprintf(stderr, "lift: autopilot inputs %s: cannot open\n", s);
            exit(2);
        }
        /* The GO tick's poll still sees the wheel idle (0x80, 0, 0) in neutral. */
        fprintf(s_ap_out, "# autopilot controls: race_tick steer accel brake gear\n0 0x80 0x00 0x00 0\n");
        fflush(s_ap_out);
    }
    s = getenv("I960_HOST_EXPORT_DYNDUMP");
    if (s && *s) {
        s_dyn = fopen(s, "wb");
        if (s_dyn)
            fwrite("SRDYN01\0", 1, 8, s_dyn);
    }
    s = getenv("I960_HOST_EXPORT_STATIC_FRAMES");
    if (s && *s)
        s_static_frames = (unsigned)strtoul(s, NULL, 0);
    atexit(export_write);
    fprintf(stderr, "lift: export-course -> %s%s\n", s_path, s_autopilot ? " (autopilot)" : "");
}

/* End of one tick (i960_host_frame_present), after the race trace row. */
void i960_host_export_course_tick(void)
{
    int rc;

    unsigned fi;

    if (s_enabled < 0)
        export_init();
    if (!s_enabled)
        return;
    if ((i32)i960_ld_u32(I960_WORKRAM, 0x214120, 0) > 0)
        s_rtick++;
    s_render_tick = -1;
    for (fi = 0; fi < s_n_frame_ticks; fi++)
        if (s_frame_ticks[fi] == s_rtick)
            s_render_tick = s_rtick;
    if (s_race_done)
        return;
    /* The race scene (mode 3, scene 5) with the clock running. The clock stays positive after the race ends
     * (scene 12, then attract and menus), so stop for good once the scene is left. */
    if (i960_ld_u32(I960_WORKRAM, 0x202098, 0) != 3u
        || (i960_ld_u32(I960_WORKRAM, 0x2020ac, 0) & 15u) != 5u
        || ((i32)i960_ld_u32(I960_WORKRAM, 0x214120, 0) <= 0 && !s_countdown)) {
        if (s_frame)
            s_race_done = 1;
        return;
    }
    if (s_autopilot)
        autopilot_tick();
    if (!s_pack && s_render_tick < 0)
        return;
    if (!model2_hw_view_matrix(s_view))
        return;
    s_cc_n = 0; /* palette / texture RAM may have changed since the last frame */
    rc = model2_geo_render_capture_display(capture_cb, NULL);
    if (rc == 1)
        s_frame++;
}
