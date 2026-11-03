/* Host preview of the course renderer: rasterise the core's triangles into an image with a 1/z buffer and write
 * it as PPM, to check the renderer's output without the console. Textured like the PVR setup in
 * code/render/pvr_backend.c: perspective-correct, nearest texel, ARGB1555 or VQ, repeat/mirror/clamp per texture,
 * cut-out texels discarded, texture times vertex colour. */
#include "preview.h"
#include "render/twiddle.h"
#include "render/hud2d.h"
#include "render/countdown.h"
#include "race/palette_irq.h"
#include "race/hud.h"
#include "sr_mem.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define W 640
#define H 480

typedef struct {
    uint8_t rgb[W * H * 3];
    float depth[W * H];
    const sr_course_t *course;
    const sr_course_texture_t *tex;
    int blend; /* translucent pass: darken by the vertex alpha, keep the depth */
} image_t;

static float edge(float ax, float ay, float bx, float by, float px, float py) {
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}

static void material(void *user, uint16_t texture, uint16_t flags) {
    image_t *im = user;
    (void)flags;
    im->tex = texture < im->course->n_textures ? &im->course->textures[texture] : NULL;
}

static uint32_t wrap(float t, uint32_t size, int repeat, int mirror) {
    int32_t i = (int32_t)floorf(t * (float)size);
    if(!repeat) return i < 0 ? 0 : i >= (int32_t)size ? size - 1 : (uint32_t)i;
    if(mirror) {
        const int32_t period = (int32_t)size * 2;
        i = ((i % period) + period) % period;
        return i < (int32_t)size ? (uint32_t)i : (uint32_t)(period - 1 - i);
    }
    return (uint32_t)(((i % (int32_t)size) + (int32_t)size) % (int32_t)size);
}

static uint32_t s_offscreen, s_backfacing, s_tris;

static void tri(void *user, const sr_screen_vertex_t v[3]) {
    image_t *im = user;
    s_tris++;
    if((v[0].x < 0 && v[1].x < 0 && v[2].x < 0) || (v[0].x > W && v[1].x > W && v[2].x > W) ||
       (v[0].y < 0 && v[1].y < 0 && v[2].y < 0) || (v[0].y > H && v[1].y > H && v[2].y > H))
        s_offscreen++;
    else if(edge(v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y) > 0)
        s_backfacing++;
    const sr_course_texture_t *tx = im->tex;
    float area = edge(v[0].x, v[0].y, v[1].x, v[1].y, v[2].x, v[2].y);
    int x0, x1, y0, y1, x, y, k;
    if(area == 0.f) return;
    x0 = (int)v[0].x, x1 = x0, y0 = (int)v[0].y, y1 = y0;
    for(k = 1; k < 3; k++) {
        if((int)v[k].x < x0) x0 = (int)v[k].x;
        if((int)v[k].x > x1) x1 = (int)v[k].x;
        if((int)v[k].y < y0) y0 = (int)v[k].y;
        if((int)v[k].y > y1) y1 = (int)v[k].y;
    }
    if(x0 < 0) x0 = 0;
    if(y0 < 0) y0 = 0;
    if(x1 >= W) x1 = W - 1;
    if(y1 >= H) y1 = H - 1;
    for(y = y0; y <= y1; y++)
        for(x = x0; x <= x1; x++) {
            const float px = x + 0.5f, py = y + 0.5f;
            const float w0 = edge(v[1].x, v[1].y, v[2].x, v[2].y, px, py) / area;
            const float w1 = edge(v[2].x, v[2].y, v[0].x, v[0].y, px, py) / area;
            const float w2 = 1.f - w0 - w1;
            float z, col[3];
            if(w0 < 0 || w1 < 0 || w2 < 0) continue;
            z = w0 * v[0].invz + w1 * v[1].invz + w2 * v[2].invz;
            if(z <= im->depth[y * W + x]) continue;

            if(im->blend) {
                const float a = (float)(v[0].argb >> 24) / 255.f;
                for(k = 0; k < 3; k++) im->rgb[(y * W + x) * 3 + k] = (uint8_t)(im->rgb[(y * W + x) * 3 + k] * (1.f - a));
                continue;
            }
            for(k = 0; k < 3; k++) {
                const int sh = 16 - 8 * k;
                col[k] = (w0 * ((v[0].argb >> sh) & 255) + w1 * ((v[1].argb >> sh) & 255) +
                          w2 * ((v[2].argb >> sh) & 255)) / 255.f;
            }
            if(tx) {
                /* perspective-correct: interpolate u/z, v/z and 1/z linearly on screen */
                const float iz = w0 * v[0].invz + w1 * v[1].invz + w2 * v[2].invz;
                const float u = (w0 * v[0].u * v[0].invz + w1 * v[1].u * v[1].invz + w2 * v[2].u * v[2].invz) / iz;
                const float vv = (w0 * v[0].v * v[0].invz + w1 * v[1].v * v[1].invz + w2 * v[2].v * v[2].invz) / iz;
                const uint32_t tu = wrap(u, tx->w, tx->flags & SR_TEX_REPEAT_U, tx->flags & SR_TEX_MIRROR_U);
                const uint32_t tv = wrap(vv, tx->h, tx->flags & SR_TEX_REPEAT_V, tx->flags & SR_TEX_MIRROR_V);
                const uint32_t ti = sr_twiddle_index(tu, tv, tx->w, tx->h);
                /* VQ: the 2x2 block's codebook entry, then the texel within it (twiddled: x bit 1, y bit 0) */
                const uint16_t t = (tx->flags & SR_TEX_VQ)
                                       ? tx->texels[((const uint8_t *)tx->texels)[2048 + (ti >> 2)] * 4 + (ti & 3)]
                                       : tx->texels[ti];
                if(!(t & 0x8000u)) continue; /* cut-out texel */
                col[0] *= (float)((t >> 10) & 31) / 31.f;
                col[1] *= (float)((t >> 5) & 31) / 31.f;
                col[2] *= (float)(t & 31) / 31.f;
            }
            im->depth[y * W + x] = z;
            for(k = 0; k < 3; k++) im->rgb[(y * W + x) * 3 + k] = (uint8_t)(col[k] > 1.f ? 255.f : col[k] * 255.f);
        }
}

const uint8_t *sr_preview_chars, *sr_preview_palette;

/* The HUD cells over the image: each 2D layer pixel scaled from 496 x 384 to the image. */
static void draw_hud(image_t *im) {
    static uint8_t visible[SR_HUD_CHARS / 8];
    static int scanned;
    static sr_hud_cell_t cells[64 * 64 * 4];
    uint32_t n, i;
    int x, y;
    if(!sr_tile_map || !sr_preview_chars || !sr_preview_palette) return;
    if(!scanned) {
        sr_hud2d_scan_chars(sr_preview_chars, visible);
        scanned = 1;
    }
    n = sr_hud2d_cells(sr_tile_map, visible, cells, sizeof(cells) / sizeof(cells[0]));
    for(i = 0; i < n; i++)
        for(y = cells[i].y * H / SR_HUD_H; y < (cells[i].y + 8) * H / SR_HUD_H; y++)
            for(x = cells[i].x * W / SR_HUD_W; x < (cells[i].x + 8) * W / SR_HUD_W; x++) {
                const uint32_t px = (uint32_t)(x * SR_HUD_W / W - cells[i].x), py = (uint32_t)(y * SR_HUD_H / H - cells[i].y);
                const uint32_t pen = sr_hud2d_pixel(sr_preview_chars, cells[i].code, px & 7u, py & 7u);
                uint16_t c;
                if(!pen) continue;
                c = (uint16_t)(sr_preview_palette[((cells[i].colour * 16u + pen) & 2047u) * 2u] |
                               sr_preview_palette[((cells[i].colour * 16u + pen) & 2047u) * 2u + 1u] << 8);
                if(!(c & 0x7fffu)) continue; /* black is see-through over the 3D view (the reference's overlay) */
                im->rgb[(y * W + x) * 3] = (uint8_t)((c & 31u) * 255u / 31u);
                im->rgb[(y * W + x) * 3 + 1] = (uint8_t)((c >> 5 & 31u) * 255u / 31u);
                im->rgb[(y * W + x) * 3 + 2] = (uint8_t)((c >> 10 & 31u) * 255u / 31u);
            }
}

int sr_preview_render(const sr_course_t *course, const sr_actors_t *actors, const float view_matrix[12],
                      const char *path) {
    image_t *im = calloc(1, sizeof(*im));
    sr_view_t view = {W, H, W / 2.f, H / 2.f, SR_FOCAL_X, SR_FOCAL_Y, 0.1f, SR_LIST_BOTH, material, tri, im, NULL,
                     0x70000000u, 1.02f, {0, 0}, 0, 0}; /* no headers: the material callback */
    sr_render_stats_t st;
    uint32_t actor_tris = 0;
    FILE *f;
    int i;
    if(!im) return -1;
    memset(&st, 0, sizeof(st));
    im->course = course;
    for(i = 0; i < W * H; i++) {
        im->rgb[i * 3] = 90, im->rgb[i * 3 + 1] = 140, im->rgb[i * 3 + 2] = 200; /* sky */
    }
    if(getenv("SR_CULL")) sr_render_cull = atoi(getenv("SR_CULL"));
    sr_course_render(course, view_matrix, &view, &st);
    actor_tris = st.tris_drawn;
    if(actors) {
        sr_particle_quad_t quads[64];
        const uint32_t nq = sr_particles_quads(actors, course, &view, quads, 64);
        uint32_t q;
        sr_actors_render(actors, course, view_matrix, &view, &st);
        for(q = 0; q < nq; q++) {
            sr_screen_vertex_t sv[4];
            int k;
            for(k = 0; k < 4; k++)
                sv[k] = (sr_screen_vertex_t){quads[q].v[k].x, quads[q].v[k].y, quads[q].v[k].invz,
                                             1.f / quads[q].v[k].invz, quads[q].v[k].u, quads[q].v[k].v, quads[q].argb, 0};
            material(im, quads[q].texture, SR_TRI_CUTOUT);
            {
                const sr_screen_vertex_t a[3] = {sv[0], sv[1], sv[2]}, b[3] = {sv[1], sv[3], sv[2]};
                tri(im, a);
                tri(im, b);
            }
        }
    }
    actor_tris = st.tris_drawn - actor_tris;
    if(actors) { /* the cars' shadows, as the Dreamcast's translucent list draws them */
        view.list = SR_LIST_TRANSLUCENT;
        im->blend = 1;
        im->tex = NULL;
        sr_actors_render(actors, course, view_matrix, &view, &st);
    }
    im->blend = 0;
    { /* the palette interrupt's white wash (mode 4: each colour moved toward white by wash / 256), on the 3D */
        const uint32_t wash = sr_palette_wash();
        for(i = 0; wash && i < W * H * 3; i++) im->rgb[i] = (uint8_t)(im->rgb[i] + ((255u - im->rgb[i]) * wash >> 8));
    }
    { /* the countdown's digits, in front of the course (their own GEO window) */
        sr_countdown_tri_t ct[16];
        const uint32_t nct = sr_countdown_tris(course, W, H, ct, 16);
        uint32_t c;
        for(c = 0; c < nct; c++) {
            sr_screen_vertex_t sv[3];
            int k;
            for(k = 0; k < 3; k++)
                sv[k] = (sr_screen_vertex_t){ct[c].x[k], ct[c].y[k], 1000.f, 0.001f, ct[c].u[k], ct[c].v[k],
                                             ct[c].argb[k], 0};
            material(im, ct[c].texture, ct[c].flags);
            tri(im, sv);
        }
    }
    draw_hud(im);
    /* the tach needle over the HUD, as code/render/pvr_hud.c draws it */
    if(sr_tile_map && sr_hud_needle_tick && sr_ld32(0x202008u) - sr_hud_needle_tick <= 1u) {
        uint32_t m, t, k;
        for(m = 0; m < course->n_meshes; m++)
            if(course->meshes[m].oba == 0x801a7au)
                for(t = 0; t < course->meshes[m].n_tris; t++) {
                    sr_screen_vertex_t sv[3];
                    const float *mx = sr_hud_needle_matrix;
                    for(k = 0; k < 3; k++) {
                        const sr_course_vertex_t *p = &course->meshes[m].tris[t].v[k];
                        const float x = mx[0] * p->x + mx[3] * p->y + mx[6] * p->z + mx[9];
                        const float y = mx[1] * p->x + mx[4] * p->y + mx[7] * p->z + mx[10];
                        const float z = mx[2] * p->x + mx[5] * p->y + mx[8] * p->z + mx[11];
                        sv[k] = (sr_screen_vertex_t){(248.f + 128.f * x / z) * W / SR_HUD_W,
                                                     (192.f - 128.f * y / z) * H / SR_HUD_H, 1000.f, 0.001f, 0, 0,
                                                     0xffe08030u, 0};
                    }
                    im->tex = NULL;
                    tri(im, sv);
                }
    }
    f = fopen(path, "wb");
    if(!f) return -1;
    fprintf(f, "P6\n%d %d\n255\n", W, H);
    fwrite(im->rgb, 1, sizeof(im->rgb), f);
    fclose(f);
    fprintf(stderr, "sr-native-run: %s: %u instances drawn, %u culled, %u chunks, %u vertices, %u triangles, "
            "%u near-clipped, %u back faces culled, %u batches; %u triangles of cars and helicopter\n", path, st.instances_drawn,
            st.instances_culled, st.chunks_drawn, st.verts_transformed, st.tris_drawn, st.tris_clipped, st.tris_culled,
            st.batches, actor_tris);
    if(getenv("SR_PREVIEW_COUNT"))
        fprintf(stderr, "sr-native-run: %u triangles, %u off screen, %u of the rest clockwise\n", s_tris, s_offscreen,
                s_backfacing);
    s_tris = s_offscreen = s_backfacing = 0;
    free(im);
    return 0;
}
