/* Direct host OpenGL textured geo — no MAME rasterizer port.
 *
 * Single pass, coarse Model 2 draw-priority sort: triangles are ranked by
 * z_sort/z_adjust (see float_to_zval), HUD polys first, last-submitted wins
 * on ties, then encoded as a per-vertex depth key (aDepthKey) written to
 * gl_Position.z so GL depth test (LESS) reproduces front-to-back ordering.
 * Each triangle is emitted exactly once — no separate solid/opaque/cutout
 * passes. Polygons are single-sided by default; a per-material cull_face
 * attribute flags the ones that must render double-sided (GL_CULL_FACE is
 * toggled per batch accordingly). A secondary sort by material
 * (sheet/flags/cull/patch) groups the draw into GL state-coherent batches
 * without changing this priority order.
 */

#include "model2_geo_gl.h"
#include "lift_log.h"

#ifdef I960_HOST_HAVE_GL

#include "model2_geo_tex.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "model2_gl.h"

enum {
    LUT_CACHE_CAP = 512,
    DRAW_SCRATCH_INIT_TRIS = 8192
};

typedef struct {
    u16 colorbase;
    u16 lumabase;
    u8 sheet;
    u8 flags;
    u8 luma;
    GLuint tex;
} lut_cache_ent_t;

static GLuint g_prog;
static GLuint g_solid_prog;
static GLuint g_sheet_tex[2];
static int g_ready;
static int g_sheets_uploaded;
static u32 g_sheets_gen_uploaded;
static u32 g_lut_palette_sig;
static int g_logged_shader;
static int g_flat_mode = -1; /* -1 = read env once */

int model2_geo_gl_want_flat(void)
{
    if (g_flat_mode < 0) {
        const char *e = getenv("I960_GEO_FLAT");

        /* Opt-in only: default is full textured/palette colour (attract + race). */
        g_flat_mode = (e && e[0] == '1') ? 1 : 0;
    }
    return g_flat_mode;
}

void model2_geo_gl_set_flat(int on)
{
    g_flat_mode = on ? 1 : 0;
}

static GLint g_loc_sheet = -1;
static GLint g_loc_lut = -1;
static GLint g_loc_cutout = -1;
static GLint g_loc_patch = -1;   /* xy = origin, zw = size (pixels) */
static GLint g_loc_mirror = -1;  /* x = mirror_x, y = mirror_y */
static GLint g_loc_sort_depth = -1;
static GLint g_loc_solid_depth = -1;
static GLint g_loc_stipple = -1;
static GLint g_loc_solid_stipple = -1;

static lut_cache_ent_t g_lut_cache[LUT_CACHE_CAP];
static unsigned g_lut_cache_n;
static unsigned g_lut_cache_clock;

static unsigned *g_order; 
static unsigned *g_elem; /* 3 indices per tri, remapped for DrawElements */
static unsigned char *g_solid_rgb; /* 9 bytes per tri (only used for solids) */
static unsigned g_scratch_tris;
static unsigned *g_prio_idx; /* Scratch buffer for priority sorting (HUD/zval/arrival) */
static unsigned *g_rank;     /* g_rank[original_triangle] = unique priority rank (0..ntris-1) */
static float *g_depth_keys = NULL; // Scratch buffer for keys

static const model2_geo_tri_mat_t *g_prio_mats;

static GLuint compile_shader(GLenum type, const char *src)
{
    GLuint s = glCreateShader(type);
    GLint ok = 0;
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, (GLsizei)sizeof(log), NULL, log);
        fprintf(stderr, "lift: geo shader compile: %s\n", log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

static int build_program(void)
{
    /* macOS legacy GL defaults to GLSL 1.10 (no int bitops) — stay float-only. */
    // Customize gl_Position to control draw order in code to simulate Model 2    
    static const char *vs =
        "attribute float aDepthKey;\n"
        "void main() {\n"
        "  gl_TexCoord[0] = gl_MultiTexCoord0;\n"
        "  vec4 pos = ftransform();\n"
        "  pos.z = (aDepthKey * 2.0 - 1.0) * pos.w;\n"
        "  gl_Position = pos;\n"
        "}\n";
    /*
     * Texcoords are patch-local pixels (pu/8). Mirror + wrap match MAME
     * fetch_bilinear_texel (nearest), then sample absolute sheet UV.
     * Sheet R8 is baked with get_texel(0,0,sx,sy) including the x>=1024 fold.
     * Real Model 2 has no alpha blending: this 1-in-2 pixel dither fakes 50% translucency exactly like the arcade hardware.
     */
    static const char *fs =
        "uniform sampler2D uSheet;\n"
        "uniform sampler2D uLut;\n"
        "uniform float uCutout;\n"
        "uniform vec4 uPatch;\n"
        "uniform vec2 uMirror;\n"
        "uniform float uStipple;\n"
        "float wrap_coord(float pix, float size, float mirror) {\n"
        "  float s = max(size, 1.0);\n"
        "  float cell = floor(pix / s);\n"
        "  float local = pix - cell * s;\n"
        "  if (mirror > 0.5 && mod(cell, 2.0) >= 1.0)\n"
        "    local = s - 1.0 - local;\n"
        "  local = local - floor(local / s) * s;\n"
        "  if (local < 0.0) local += s;\n"
        "  return floor(local);\n"
        "}\n"
        "void main() {\n"
        "  if (uStipple > 0.5) {\n"
        "    float chk = mod(floor(gl_FragCoord.x) + floor(gl_FragCoord.y), 2.0);\n"
        "    if (chk < 1.0) discard;\n"
        "  }\n"
        "  float u0 = wrap_coord(gl_TexCoord[0].x, uPatch.z, uMirror.x);\n"
        "  float v0 = wrap_coord(gl_TexCoord[0].y, uPatch.w, uMirror.y);\n"
        "  vec2 st = vec2((uPatch.x + u0 + 0.5) / 2048.0,\n"
        "                 (uPatch.y + v0 + 0.5) / 1024.0);\n"
        "  float t = floor(texture2D(uSheet, st).r * 255.0 + 0.5);\n"
        "  vec4 c = texture2D(uLut, vec2((t + 0.5) / 16.0, 0.5));\n"
        "  if (uCutout > 0.5 && c.a < 0.5) discard;\n"
        "  gl_FragColor = vec4(c.rgb, 1.0);\n"
        "}\n";


    /* SHADER UNICOLOR */
    static const char *vs_solid =
        "attribute float aDepthKey;\n"
        "void main() {\n"
        "  gl_FrontColor = gl_Color;\n"
        "  vec4 pos = ftransform();\n"
        "  pos.z = (aDepthKey * 2.0 - 1.0) * pos.w;\n"
        "  gl_Position = pos;\n"
        "}\n";

    static const char *fs_solid =
        "uniform float uStipple;\n"
        "void main() {\n"
        "  if (uStipple > 0.5) {\n"
        "    float chk = mod(floor(gl_FragCoord.x) + floor(gl_FragCoord.y), 2.0);\n"
        "    if (chk < 1.0) discard;\n"
        "  }\n"
        "  gl_FragColor = gl_Color;\n"
        "}\n";

    GLuint v, f, vs_s, fs_s;
    GLint ok = 0;

    v = compile_shader(GL_VERTEX_SHADER, vs);
    f = compile_shader(GL_FRAGMENT_SHADER, fs);
    if (!v || !f) {
        if (v)
            glDeleteShader(v);
        if (f)
            glDeleteShader(f);
        return -1;
    }
    g_prog = glCreateProgram();
    glAttachShader(g_prog, v);
    glAttachShader(g_prog, f);
    glBindAttribLocation(g_prog, 1, "aDepthKey");
    glLinkProgram(g_prog);
    glDeleteShader(v);
    glDeleteShader(f);
    g_loc_sheet = glGetUniformLocation(g_prog, "uSheet");
    g_loc_lut = glGetUniformLocation(g_prog, "uLut");
    g_loc_cutout = glGetUniformLocation(g_prog, "uCutout");
    g_loc_patch = glGetUniformLocation(g_prog, "uPatch");
    g_loc_mirror = glGetUniformLocation(g_prog, "uMirror");
    g_loc_sort_depth = glGetAttribLocation(g_prog, "aDepthKey");
    g_loc_stipple = glGetUniformLocation(g_prog, "uStipple");

    vs_s = compile_shader(GL_VERTEX_SHADER, vs_solid);
    fs_s = compile_shader(GL_FRAGMENT_SHADER, fs_solid);
    if (vs_s && fs_s) {
        g_solid_prog = glCreateProgram();
        glAttachShader(g_solid_prog, vs_s);
        glAttachShader(g_solid_prog, fs_s);
        glBindAttribLocation(g_solid_prog, 1, "aDepthKey");
        glLinkProgram(g_solid_prog);
        glDeleteShader(vs_s);
        glDeleteShader(fs_s);
        g_loc_solid_depth = glGetAttribLocation(g_solid_prog, "aDepthKey");
        g_loc_solid_stipple = glGetUniformLocation(g_solid_prog, "uStipple");
    }

    return 0;
}

static void lut_cache_clear(void)
{
    unsigned i;

    for (i = 0; i < g_lut_cache_n; i++)
        glDeleteTextures(1, &g_lut_cache[i].tex);
    g_lut_cache_n = 0;
    g_lut_cache_clock = 0;
}

static void upload_sheets(void)
{
    u8 *r8;
    unsigned s;
    u32 gen = model2_tex_sheets_dirty_gen();

    if (g_sheets_uploaded && gen == g_sheets_gen_uploaded)
        return;
    r8 = (u8 *)malloc((size_t)MODEL2_GEO_SHEET_W * (size_t)MODEL2_GEO_SHEET_H);
    if (!r8)
        return;
    for (s = 0; s < 2u; s++) {
        const u32 *bank = model2_tex_sheet_bank(s);
        if (!bank)
            continue;
        model2_decode_sheet_indices(bank, r8);
        glBindTexture(GL_TEXTURE_2D, g_sheet_tex[s]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, MODEL2_GEO_SHEET_W,
                     MODEL2_GEO_SHEET_H, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, r8);
    }
    free(r8);
    g_sheets_uploaded = 1;
    g_sheets_gen_uploaded = gen;
    lift_log( "lift: geo index sheets uploaded (2048x1024 R8) gen=%u\n",
            (unsigned)gen);
}

static int mat_same(const model2_geo_tri_mat_t *a, const model2_geo_tri_mat_t *b)
{
    if (a->colorbase != b->colorbase || a->lumabase != b->lumabase
        || a->sheet != b->sheet || a->flags != b->flags || a->luma != b->luma
        )
        return 0;
    if (a->cull_face != b->cull_face) // Polygons need to show 2 faces
        return 0;
    if ((a->flags & MODEL2_GEO_TEX_TEXTURED) == 0)
        return 1;
    return a->patch_x == b->patch_x && a->patch_y == b->patch_y
        && a->patch_w == b->patch_w && a->patch_h == b->patch_h;
}

static int ensure_scratch(unsigned ntris)
{
    if (ntris <= g_scratch_tris)
        return 0;
    {
        unsigned cap = g_scratch_tris ? g_scratch_tris : DRAW_SCRATCH_INIT_TRIS;
        while (cap < ntris)
            cap *= 2u;
        g_order = (unsigned *)realloc(g_order, cap * sizeof(unsigned));
        g_prio_idx = (unsigned *)realloc(g_prio_idx, cap * sizeof(unsigned));
        g_rank = (unsigned *)realloc(g_rank, cap * sizeof(unsigned));
        g_elem = (unsigned *)realloc(g_elem, cap * 3u * sizeof(unsigned));
        g_solid_rgb = (unsigned char *)realloc(g_solid_rgb, cap * 9u);
        if (!g_order || !g_elem || !g_solid_rgb || !g_prio_idx || !g_rank )
            return -1;
        g_scratch_tris = cap;
    }
    return 0;
}

static const model2_geo_tri_mat_t *g_sort_mats;

/*
 * MAME model2_state::float_to_zval (model2_v.cpp) — z_adjust from GEO 0x08;
 * z_adjust increases floatval precision (real Model 2 stores z_sort on 16 bits).
 */
static unsigned float_to_zval(float floatval, u32 z_adjust)
{
    union {
        float f;
        u32 u;
    } conv;
    int32_t fpint;
    int32_t exponent;
    u32 mantissa;

    conv.f = floatval;
    fpint = (int32_t)conv.u;
    exponent = ((fpint >> 23) & 0xff) - (int32_t)((z_adjust >> 23) & 0xffu);
    mantissa = (u32)fpint & 0x7fffffu;
    mantissa += 0x400u;
    if (mantissa > 0x7fffffu) {
        exponent++;
        mantissa = (mantissa & 0x7fffffu) >> 1;
    }
    mantissa >>= 11;
    if (fpint < 0)
        return 0;
    if (exponent < -12)
        return 0;
    if (exponent < 0)
        return (mantissa | 0x1000u) >> (unsigned)(-exponent);
    if (exponent < 15)
        return ((unsigned)(exponent + 1) << 12) | mantissa;
    return 0xffffu;
}

static int prio_order_cmp(const void *a, const void *b)
{
    unsigned ia = *(const unsigned *)a;
    unsigned ib = *(const unsigned *)b;
    const model2_geo_tri_mat_t *ma = &g_prio_mats[ia];
    const model2_geo_tri_mat_t *mb = &g_prio_mats[ib];

    /* HUD first */
    if ((ma->pad & 1u) != (mb->pad & 1u))
        return ((ma->pad & 1u) < (mb->pad & 1u)) ? 1 : -1;

    /* Custom z_sort -> Model 2 has lower Z precision than modern architectures */
    unsigned za = float_to_zval(ma->z_sort, ma->z_adjust);
    unsigned zb = float_to_zval(mb->z_sort, mb->z_adjust);
    if (za != zb) return (za < zb) ? -1 : 1;

    /* prioritize last generate */
    if (ia != ib) return (ia > ib) ? -1 : 1;
    return 0;
}

/*
 * Material state sorting for OpenGL pipeline optimization only.
 * Independent of polygon draw order.
 */
static int mat_order_cmp(const void *a, const void *b)
{
    unsigned ia = *(const unsigned *)a;
    unsigned ib = *(const unsigned *)b;
    const model2_geo_tri_mat_t *ma = &g_sort_mats[ia];
    const model2_geo_tri_mat_t *mb = &g_sort_mats[ib];
   
    if (ma->sheet != mb->sheet)
        return (ma->sheet < mb->sheet) ? -1 : 1;

    if (ma->flags != mb->flags)
        return (ma->flags < mb->flags) ? -1 : 1;

    if (ma->cull_face != mb->cull_face)
        return (ma->cull_face < mb->cull_face) ? -1 : 1;

    if (ma->colorbase != mb->colorbase)
        return (ma->colorbase < mb->colorbase) ? -1 : 1;
    if (ma->lumabase != mb->lumabase)
        return (ma->lumabase < mb->lumabase) ? -1 : 1;
    if (ma->luma != mb->luma)
        return (ma->luma < mb->luma) ? -1 : 1;

    if (ma->patch_x != mb->patch_x) return (ma->patch_x < mb->patch_x) ? -1 : 1;
    if (ma->patch_y != mb->patch_y) return (ma->patch_y < mb->patch_y) ? -1 : 1;
    if (ma->patch_w != mb->patch_w) return (ma->patch_w < mb->patch_w) ? -1 : 1;
    if (ma->patch_h != mb->patch_h) return (ma->patch_h < mb->patch_h) ? -1 : 1;

    /* Last-submitted first (MAME list prepend). */
    if (ia != ib)
        return (ia > ib) ? -1 : 1;
    return 0;
}

static int lut_cache_find(const model2_geo_tri_mat_t *m)
{
    unsigned i;
    for (i = 0; i < g_lut_cache_n; i++) {
        if (g_lut_cache[i].colorbase == m->colorbase
            && g_lut_cache[i].lumabase == m->lumabase
            && g_lut_cache[i].sheet == m->sheet
            && g_lut_cache[i].flags == m->flags
            && g_lut_cache[i].luma == m->luma)
            return (int)i;
    }
    return -1;
}

static GLuint lut_cache_get(const model2_geo_tri_mat_t *m)
{
    int found = lut_cache_find(m);
    u8 lut[16 * 4];
    GLuint tex;
    lut_cache_ent_t *ent;

    if (found >= 0)
        return g_lut_cache[found].tex;

    model2_palette_build_texel_lut(m->colorbase, m->lumabase, m->luma,
                                   (m->flags & MODEL2_GEO_TEX_CUTOUT) ? 1 : 0, lut);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, lut);

    if (g_lut_cache_n < LUT_CACHE_CAP) {
        ent = &g_lut_cache[g_lut_cache_n++];
    } else {
        unsigned slot = g_lut_cache_clock++ % LUT_CACHE_CAP;
        glDeleteTextures(1, &g_lut_cache[slot].tex);
        ent = &g_lut_cache[slot];
    }
    ent->colorbase = m->colorbase;
    ent->lumabase = m->lumabase;
    ent->sheet = m->sheet;
    ent->flags = m->flags;
    ent->luma = m->luma;
    ent->tex = tex;
    return tex;
}

void model2_geo_gl_tex_init(void)
{
    if (g_ready)
        return;
    (void)model2_gl_load();
    if (build_program() != 0) {
        if (!g_logged_shader) {
            lift_log( "lift: geo textured shader unavailable — flat fallback\n");
            g_logged_shader = 1;
        }
        return;
    }
    glGenTextures(2, g_sheet_tex);
    g_ready = 1;
    model2_palette_selftest();
}

void model2_geo_gl_tex_shutdown(void)
{
    if (g_prog)
        glDeleteProgram(g_prog);
    if (g_solid_prog)
        glDeleteProgram(g_solid_prog);
    if (g_sheet_tex[0])
        glDeleteTextures(2, g_sheet_tex);
    lut_cache_clear();
    free(g_order);
    free(g_elem);
    free(g_solid_rgb);
    free(g_prio_idx);
    free(g_rank);
    free(g_depth_keys);
    g_prog = 0;
    g_solid_prog = 0;
    g_sheet_tex[0] = g_sheet_tex[1] = 0;
    g_order = NULL;
    g_prio_idx = NULL;
    g_rank = NULL;
    g_elem = NULL;
    g_solid_rgb = NULL;
    g_scratch_tris = 0;
    g_ready = 0;
    g_sheets_uploaded = 0;
    g_sheets_gen_uploaded = 0;
    g_lut_palette_sig = 0;
}

static void draw_tex_elements(const float *xyzuv, const unsigned *elem, unsigned nidx,
                              const model2_geo_tri_mat_t *m)
{
    GLuint lut = lut_cache_get(m);

    glUseProgram(g_prog);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_sheet_tex[m->sheet & 1u]);
    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, lut);
    if (g_loc_sheet >= 0)
        glUniform1i(g_loc_sheet, 0);
    if (g_loc_lut >= 0)
        glUniform1i(g_loc_lut, 1);
    if (g_loc_cutout >= 0)
        glUniform1f(g_loc_cutout, (m->flags & MODEL2_GEO_TEX_CUTOUT) ? 1.f : 0.f);
    if (g_loc_patch >= 0)
        glUniform4f(g_loc_patch, (float)m->patch_x, (float)m->patch_y,
                    (float)(m->patch_w ? m->patch_w : 32),
                    (float)(m->patch_h ? m->patch_h : 32));
    if (g_loc_mirror >= 0)
        glUniform2f(g_loc_mirror, (m->flags & MODEL2_GEO_TEX_MIRROR_X) ? 1.f : 0.f,
                    (m->flags & MODEL2_GEO_TEX_MIRROR_Y) ? 1.f : 0.f);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glVertexPointer(3, GL_FLOAT, 5 * (int)sizeof(float), xyzuv);
    glTexCoordPointer(2, GL_FLOAT, 5 * (int)sizeof(float), xyzuv + 3);
    glDrawElements(GL_TRIANGLES, (GLsizei)nidx, GL_UNSIGNED_INT, elem);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
}

static void draw_solid_elements(const float *xyzuv, const unsigned char *rgb_by_vert,
                                const unsigned *elem, unsigned nidx,
                                const model2_geo_tri_mat_t *m)
{
    (void)m;
    glUseProgram(g_solid_prog);
    /* Textured path leaves LUT on TEXTURE1 — must clear both units or fixed-
     * function solids sample the LUT and go black (logos still look fine). */
    glActiveTexture(GL_TEXTURE1);
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_FLOAT, 5 * (int)sizeof(float), xyzuv);
    glColorPointer(3, GL_UNSIGNED_BYTE, 0, rgb_by_vert);
    glDrawElements(GL_TRIANGLES, (GLsizei)nidx, GL_UNSIGNED_INT, elem);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
}

/* Main polygon loop (Model 2 logic: front-to-back, no overdraw). */
void model2_geo_gl_draw_textured(const float *xyzuv, unsigned nverts,
                                 const model2_geo_tri_mat_t *mats, unsigned ntris)
{
    unsigned t, run_start;
    static int logged_batches;
    static int logged_flat;
    unsigned batches = 0;
    unsigned char *rgb_by_vert = NULL;

    if (!xyzuv || !mats || ntris == 0 || nverts < 3u)
        return;
    if (!g_ready)
        model2_geo_gl_tex_init();
    if (ensure_scratch(ntris) != 0)
        return;

    /*
     * Same latch/draw entry as attract. Flat mode: one gray lit shade per tri
     * (no texture / palette) so race/attract transforms are comparable.
     */
    if (model2_geo_gl_want_flat()) {
        if (!logged_flat) {
            lift_log(
                    "lift: geo flat draw (I960_GEO_FLAT) tris=%u — same latch "
                    "as attract textured path\n",
                    ntris);
            logged_flat = 1;
        }
        glUseProgram(0);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glBegin(GL_TRIANGLES);
        for (t = 0; t < ntris; t++) {
            unsigned b = t * 3u;
            const float *v0 = xyzuv + b * 5u;
            const float *v1 = xyzuv + (b + 1u) * 5u;
            const float *v2 = xyzuv + (b + 2u) * 5u;
            float ax = v1[0] - v0[0], ay = v1[1] - v0[1], az = v1[2] - v0[2];
            float bx = v2[0] - v0[0], by = v2[1] - v0[1], bz = v2[2] - v0[2];
            float nx = ay * bz - az * by;
            float ny = az * bx - ax * bz;
            float nz = ax * by - ay * bx;
            float nl = sqrtf(nx * nx + ny * ny + nz * nz);
            float shade;

            if (nl > 1e-6f) {
                nx /= nl;
                ny /= nl;
                nz /= nl;
            }
            shade = 0.30f + 0.70f * fabsf(nx * 0.35f + ny * 0.85f + nz * 0.35f);
            if (shade > 1.f)
                shade = 1.f;
            glColor3f(shade, shade, shade);
            glVertex3f(v0[0], v0[1], v0[2]);
            glVertex3f(v1[0], v1[1], v1[2]);
            glVertex3f(v2[0], v2[1], v2[2]);
        }
        glEnd();
        return;
    }
    
    /* Palram/colorxlat/lumaram change after CGM — drop stale LUTs. */
    {
        u32 psig = model2_palette_state_sig();
        if (psig != g_lut_palette_sig) {
            lut_cache_clear();
            g_lut_palette_sig = psig;
        }
    }

    /* Custom per-triangle depth key encoding the Model 2 draw-priority algorithm:
    * coarse global z-sort first, last-submitted-wins on ties (rank -> NDC z). */
    for (t = 0; t < ntris; t++) { g_prio_idx[t] = t; }
    g_prio_mats = mats;
    qsort(g_prio_idx, ntris, sizeof(unsigned), prio_order_cmp);

    g_depth_keys = (float *)realloc(g_depth_keys, nverts * sizeof(float));
    {
        float inv_ntris = 1.0f / (float)ntris;
        for (t = 0; t < ntris; t++) {
            unsigned src = g_prio_idx[t];
            unsigned base = src * 3u;
            float dk = (float)t * inv_ntris;

            g_rank[src] = t;
            g_depth_keys[base + 0u] = dk;
            g_depth_keys[base + 1u] = dk;
            g_depth_keys[base + 2u] = dk;
        }
    }

    for (t = 0; t < ntris; t++)
        g_order[t] = t;
    g_sort_mats = mats;
    qsort(g_order, ntris, sizeof(unsigned), mat_order_cmp);

    /* Element indices into the original expanded xyzuv (3 verts/tri). */
    for (t = 0; t < ntris; t++) {
        unsigned src = g_order[t];
        unsigned base = src * 3u;
        g_elem[t * 3u + 0u] = base + 0u;
        g_elem[t * 3u + 1u] = base + 1u;
        g_elem[t * 3u + 2u] = base + 2u;
    }

    /* Solid colors: one RGB per expanded vert (same for all 3 corners). */
    rgb_by_vert = (unsigned char *)realloc(g_solid_rgb, nverts * 3u);
    if (!rgb_by_vert)
        return;
    g_solid_rgb = rgb_by_vert;
    memset(rgb_by_vert, 0, nverts * 3u);
    {
        u8 rgb[3];
        u16 last_cb = 0xffffu, last_lb = 0xffffu;
        u8 last_luma = 0xffu;
        int have_rgb = 0;
        for (t = 0; t < ntris; t++) {
            unsigned src = g_order[t];
            unsigned base, k;
            if (mats[src].flags & MODEL2_GEO_TEX_TEXTURED)
                continue;
            if (!have_rgb || mats[src].colorbase != last_cb
                || mats[src].lumabase != last_lb || mats[src].luma != last_luma) {
                model2_palette_lookup_solid(mats[src].colorbase, mats[src].luma, rgb);
                last_cb = mats[src].colorbase;
                last_lb = mats[src].lumabase;
                last_luma = mats[src].luma;
                have_rgb = 1;
            }
            base = src * 3u;
            for (k = 0; k < 3u; k++) {
                rgb_by_vert[(base + k) * 3u + 0u] = rgb[0];
                rgb_by_vert[(base + k) * 3u + 1u] = rgb[1];
                rgb_by_vert[(base + k) * 3u + 2u] = rgb[2];
            }
        }
    }

    if (!g_ready) {
        draw_solid_elements(xyzuv, rgb_by_vert, g_elem, ntris * 3u, &mats[0]);
        return;
    }    
    upload_sheets();
    glDisable(GL_BLEND);

    glDisable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glClear(GL_DEPTH_BUFFER_BIT);

    glCullFace(GL_FRONT);

    /* Feed the custom per-vertex depth key used to fake Model 2 draw order. */
    if (g_loc_sort_depth >= 0) {
        glEnableVertexAttribArray(g_loc_sort_depth);
        glVertexAttribPointer(g_loc_sort_depth, 1, GL_FLOAT, GL_FALSE, 0, g_depth_keys);
    }


    /* Only one pass for rendering */
    run_start = 0;
    for (t = 1; t <= ntris; t++) {
        int same = (t < ntris) && mat_same(&mats[g_order[run_start]], &mats[g_order[t]]);
        if (same)
            continue;
        {
            const model2_geo_tri_mat_t *batch = &mats[g_order[run_start]];
            unsigned nidx = (t - run_start) * 3u;
            const unsigned *elem = g_elem + run_start * 3u;

            int stip = (batch->flags & MODEL2_GEO_TEX_CHECKER) ? 1 : 0;
            if (g_loc_stipple >= 0) glUniform1f(g_loc_stipple, stip ? 1.f : 0.f);

            if (batch->cull_face) { glEnable(GL_CULL_FACE); }
            else { glDisable(GL_CULL_FACE); }

            if (batch->flags & MODEL2_GEO_TEX_TEXTURED)
                draw_tex_elements(xyzuv, elem, nidx, batch);
            else
                draw_solid_elements(xyzuv, rgb_by_vert, elem, nidx, batch);
            batches++;
            run_start = t;
        }
    }

    if (!logged_batches) {
        lift_log( "lift: geo draw batches=%u tris=%u lut_cache=%u\n", batches,
                ntris, g_lut_cache_n);
        logged_batches = 1;
    }


    if (g_loc_sort_depth >= 0)
        { glDisableVertexAttribArray(g_loc_sort_depth); }

    glDisable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glUseProgram(0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

#endif /* I960_HOST_HAVE_GL */
