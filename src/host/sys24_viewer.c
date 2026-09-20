/* SDL2 live preview: sys24 tiles + optional OpenGL geo composite. */

#include "sys24_viewer.h"
#include "lift_log.h"
#include "model2_hw.h"
#include "i960_host.h"
#include "i960_mem.h"
#include "i960_lift.h"
#include "i960_fp.h"
#include "model2_geo_lift.h"
#include "model2_rom.h"
#include "model2_nvram.h"
#include "model2_host_aspect.h"
#include "sys24_tile.h"
#include "sys24_viewer_record.h"

extern int model2_snd_host_audio_open(void);
extern void model2_snd_host_audio_close(void);

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef I960_HOST_HAVE_SDL
#include <SDL.h>
#ifdef I960_HOST_HAVE_GL
#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#endif
#if defined(__APPLE__)
#include <objc/message.h>
#include <objc/runtime.h>
#endif
#endif

enum {
    SYS24_VIEW_SCALE = 2
};

static const char VIEWER_TITLE[] = "Sega Rally Championship 95 Arcade";

static int g_wanted;
static int g_open;
static int g_paused; /* Space toggles — flip blocks so the game does not advance. */
static unsigned g_shot; /* 1-based; resets when attract cycle (0x20a780) advances. */
static unsigned g_shifter; /* 0=N, 1..4 — IN1 H-pattern → 0x202044 */
static u32 g_shot_cycle = 0xffffffffu;
static u32 g_shot_inner = 0xffffffffu;
static u32 g_shot_course = 0xffffffffu;
static u32 g_shot_tab = 0xffffffffu;
static u32 g_shot_script = 0xffffffffu;

#ifdef I960_HOST_HAVE_SDL
static SDL_Window *g_window;
#ifdef I960_HOST_HAVE_GL
static SDL_GLContext g_gl;
static GLuint g_tile_tex;
#else
static SDL_Renderer *g_renderer;
static SDL_Texture *g_texture;
#endif
static sys24_tile_state_t *g_tile;
static u32 *g_bitmap;       /* bottom (even) layers — under polygons */
static u32 *g_bitmap_pri;   /* priority (odd) layers — over polygons */
static u32 *g_tile_alpha; /* black-punched upload buffer for HUD overlay */
static u32 *g_record_bgra; /* native 496×384 BGRA for async ffmpeg pipe */
static u32 g_tile_sig;
static int g_tile_valid;
static int g_tile_tex_dirty;
static const u32 *g_tile_upload_src; /* source for next GL tex upload */
#endif

static void viewer_record_try_start_from_env(void)
{
    const char *path = getenv("I960_HOST_RECORD");

    if (!path || !*path || path[0] == '0')
        return;
    if (sys24_viewer_record_start(path, SYS24_FB_WIDTH, SYS24_FB_HEIGHT, 60) != 0)
        fprintf(stderr, "lift: I960_HOST_RECORD ignored (ffmpeg start failed)\n");
}

#ifdef I960_HOST_HAVE_SDL
/* Nearest-neighbor scale + optional Y flip (GL readback is bottom-up). */
static void viewer_scale_bgra_nn(const u8 *src, int sw, int sh, int src_flip_y,
                                u8 *dst, int dw, int dh)
{
    int y;
    int x;

    for (y = 0; y < dh; y++) {
        int sy = src_flip_y ? (sh - 1 - (y * sh / dh)) : (y * sh / dh);
        const u8 *row = src + (size_t)sy * (size_t)sw * 4u;
        u8 *out = dst + (size_t)y * (size_t)dw * 4u;

        for (x = 0; x < dw; x++) {
            int sx = x * sw / dw;
            const u8 *p = row + (size_t)sx * 4u;

            out[0] = p[0];
            out[1] = p[1];
            out[2] = p[2];
            out[3] = p[3];
            out += 4;
        }
    }
}

/* Host u32 0xAARRGGBB (LE = B,G,R,A) → packed RGB24 for ffmpeg. */
static void viewer_bgra_u32_to_rgb24(const u32 *src, u8 *dst, int n_pixels)
{
    int i;

    for (i = 0; i < n_pixels; i++) {
        u32 p = src[i];

        dst[0] = (u8)((p >> 16) & 0xffu); /* R */
        dst[1] = (u8)((p >> 8) & 0xffu);  /* G */
        dst[2] = (u8)(p & 0xffu);         /* B */
        dst += 3;
    }
}

static void viewer_record_submit_native(const u32 *bgra496)
{
    static u8 *s_rgb;
    size_t rgb_bytes;

    if (!sys24_viewer_record_active() || !bgra496)
        return;
    rgb_bytes = (size_t)SYS24_FB_WIDTH * (size_t)SYS24_FB_HEIGHT * 3u;
    if (!s_rgb) {
        s_rgb = (u8 *)malloc(rgb_bytes);
        if (!s_rgb)
            return;
    }
    viewer_bgra_u32_to_rgb24(bgra496, s_rgb,
                             SYS24_FB_WIDTH * SYS24_FB_HEIGHT);
    sys24_viewer_record_submit_bgra(s_rgb, rgb_bytes);
}

#ifdef I960_HOST_HAVE_GL
static void viewer_record_from_gl_letterbox(int vx, int vy, int vw, int vh)
{
    static u8 *s_readback;
    static size_t s_readback_bytes;
    static u8 *s_rgb;
    size_t need;
    size_t rgb_bytes;
    int win_w = 0;
    int win_h = 0;
    int dw = 0;
    int dh = 0;
    int rx;
    int ry;
    int rw;
    int rh;
    GLint pack = 0;

    if (!sys24_viewer_record_active() || !g_record_bgra)
        return;
    if (vw < 1 || vh < 1)
        return;

    SDL_GetWindowSize(g_window, &win_w, &win_h);
    SDL_GL_GetDrawableSize(g_window, &dw, &dh);
    if (win_w < 1)
        win_w = 1;
    if (win_h < 1)
        win_h = 1;
    if (dw < 1)
        dw = win_w;
    if (dh < 1)
        dh = win_h;

    /*
     * Letterbox (vx,vy) is SDL/top-left in window coords; glReadPixels is
     * bottom-left in *drawable* pixels (Retina may be 2× window size).
     */
    {
        float sx = (float)dw / (float)win_w;
        float sy = (float)dh / (float)win_h;

        rw = (int)((float)vw * sx + 0.5f);
        rh = (int)((float)vh * sy + 0.5f);
        rx = (int)((float)vx * sx + 0.5f);
        ry = dh - (int)((float)(vy + vh) * sy + 0.5f);
    }
    if (rw < 1)
        rw = 1;
    if (rh < 1)
        rh = 1;
    if (rx < 0)
        rx = 0;
    if (ry < 0)
        ry = 0;
    if (rx + rw > dw)
        rw = dw - rx;
    if (ry + rh > dh)
        rh = dh - ry;
    if (rw < 1 || rh < 1)
        return;

    need = (size_t)rw * (size_t)rh * 4u;
    if (!s_readback || s_readback_bytes < need) {
        free(s_readback);
        s_readback = (u8 *)malloc(need);
        s_readback_bytes = s_readback ? need : 0;
        if (!s_readback)
            return;
    }

    /* Match texture upload path; UNSIGNED_BYTE+BGRA can read as zero on macOS. */
    glFinish();
    glGetIntegerv(GL_PACK_ALIGNMENT, &pack);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(rx, ry, rw, rh, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, s_readback);
    if (pack > 0)
        glPixelStorei(GL_PACK_ALIGNMENT, pack);

    /* Flip GL bottom-up → top-down; scale letterbox → native 496×384. */
    viewer_scale_bgra_nn(s_readback, rw, rh, 1, (u8 *)g_record_bgra,
                         SYS24_FB_WIDTH, SYS24_FB_HEIGHT);

    rgb_bytes = (size_t)SYS24_FB_WIDTH * (size_t)SYS24_FB_HEIGHT * 3u;
    if (!s_rgb) {
        s_rgb = (u8 *)malloc(rgb_bytes);
        if (!s_rgb)
            return;
    }
    viewer_bgra_u32_to_rgb24(g_record_bgra, s_rgb,
                             SYS24_FB_WIDTH * SYS24_FB_HEIGHT);
    sys24_viewer_record_submit_bgra(s_rgb, rgb_bytes);
}
#endif
#endif

static u32 viewer_tile_sig(const u8 *tile_map, const u8 *char_ram, const u8 *palram)
{
    u32 h = 2166136261u;
    unsigned i;

    if (!tile_map || !palram)
        return 0;
    /* 64 KiB tile map + 16 KiB palram — sample densely enough to catch boot scripts. */
    for (i = 0; i < 65536u; i += 8u)
        h = (h ^ tile_map[i]) * 16777619u;
    for (i = 0; i < 16384u; i += 4u)
        h = (h ^ palram[i]) * 16777619u;
    if (char_ram) {
        for (i = 0; i < 4096u; i += 16u)
            h = (h ^ char_ram[i]) * 16777619u;
    }
    /* Layer scroll/regs written by boot_tile_splash_frame.
     * Changing these forces a composite redraw but must NOT rebuild layer
     * pixmaps (MAME: scroll at draw_rect time; mark_tile_dirty only on VRAM). */
    h ^= i960_ld_u32(I960_WORKRAM, 0x20b910, 0);
    h ^= i960_ld_u32(I960_WORKRAM, 0x20b914, 0);
    h ^= i960_ld_u32(I960_WORKRAM, 0x20b918, 0);
    h ^= i960_ld_u32(I960_WORKRAM, 0x20b91c, 0);
    h ^= i960_ld_u32(I960_WORKRAM, 0x202098, 0); /* main mode — select composite */
    return h;
}

int sys24_viewer_wanted(void)
{
    const char *s = getenv("I960_HOST_LIVE_VIEW");

    g_wanted = (s && *s && s[0] != '0');
    return g_wanted;
}

#ifdef I960_HOST_HAVE_SDL
/* macOS will not order a terminal-launched window above the front app
 * unless the process is activated. Pump the queue first; SDL 2.32
 * otherwise drops the raise until a later event. */
static void viewer_bring_to_front(SDL_Window *window)
{
    SDL_Event ev;

    if (!window)
        return;
    SDL_ShowWindow(window);
    while (SDL_PollEvent(&ev)) {
    }
    SDL_RaiseWindow(window);
#if defined(__APPLE__)
    {
        Class nsapp = objc_getClass("NSApplication");
        SEL shared = sel_registerName("sharedApplication");
        SEL activate = sel_registerName("activateIgnoringOtherApps:");
        id app;

        if (nsapp) {
            app = ((id (*)(Class, SEL))objc_msgSend)(nsapp, shared);
            if (app)
                ((void (*)(id, SEL, BOOL))objc_msgSend)(app, activate, YES);
        }
    }
    SDL_RaiseWindow(window);
#endif
}
#endif

int sys24_viewer_open(const char *title)
{
    if (!sys24_viewer_wanted())
        return 0;

#ifndef I960_HOST_HAVE_SDL
    fprintf(stderr,
            "lift: live view requested but segamod2 was built without SDL2 "
            "(install SDL2 dev package and rebuild)\n");
    return -1;
#else
    const int win_h = SYS24_FB_HEIGHT * SYS24_VIEW_SCALE;
    const int win_w = model2_host_aspect_is_widescreen()
                          ? (int)((float)win_h * (16.f / 9.f) + 0.5f)
                          : (SYS24_FB_WIDTH * SYS24_VIEW_SCALE);
    size_t fb_bytes;

    if (g_open)
        return 0;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "lift: SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

#ifdef I960_HOST_HAVE_GL
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    /* MAME geo fillmap — host approximates with stencil (first texel wins). */
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
#if defined(__APPLE__)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif
    g_window = SDL_CreateWindow(
        VIEWER_TITLE,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        win_w,
        win_h,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
#else
    g_window = SDL_CreateWindow(
        VIEWER_TITLE,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        win_w,
        win_h,
        SDL_WINDOW_RESIZABLE);
#endif
    if (!g_window) {
        fprintf(stderr, "lift: SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

#ifdef I960_HOST_HAVE_GL
    g_gl = SDL_GL_CreateContext(g_window);
    if (!g_gl) {
        fprintf(stderr, "lift: SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(g_window);
        g_window = NULL;
        SDL_Quit();
        return -1;
    }
    /* Host timer paces frames; do not also block on display vsync. */
    SDL_GL_SetSwapInterval(0);
    lift_log( "lift: live viewer OpenGL composite enabled (%dx%d%s)\n",
            win_w, win_h,
            model2_host_aspect_is_widescreen() ? " aspect=16:9" : "");
    glGenTextures(1, &g_tile_tex);
    glBindTexture(GL_TEXTURE_2D, g_tile_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    /* ARGB8888 host words ↔ BGRA + REV (pen 0 / clear stay transparent over geo). */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SYS24_FB_WIDTH, SYS24_FB_HEIGHT, 0, GL_BGRA,
                 GL_UNSIGNED_INT_8_8_8_8_REV, NULL);
    (void)model2_geo_init_from_lift();
#else
    g_renderer = SDL_CreateRenderer(
        g_window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!g_renderer)
        g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED);
    if (!g_renderer) {
        fprintf(stderr, "lift: SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(g_window);
        g_window = NULL;
        SDL_Quit();
        return -1;
    }
    SDL_RenderSetLogicalSize(g_renderer, SYS24_FB_WIDTH, SYS24_FB_HEIGHT);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    g_texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_ARGB8888,
                                  SDL_TEXTUREACCESS_STREAMING, SYS24_FB_WIDTH,
                                  SYS24_FB_HEIGHT);
    if (!g_texture) {
        fprintf(stderr, "lift: SDL_CreateTexture failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(g_renderer);
        SDL_DestroyWindow(g_window);
        g_renderer = NULL;
        g_window = NULL;
        SDL_Quit();
        return -1;
    }
#endif

    g_tile = sys24_tile_create(SYS24_TILE_MASK_M2);
    if (!g_tile) {
        fprintf(stderr, "lift: sys24_tile_create failed\n");
        sys24_viewer_shutdown();
        return -1;
    }

    fb_bytes = (size_t)SYS24_FB_WIDTH * (size_t)SYS24_FB_HEIGHT * sizeof(u32);
    g_bitmap = (u32 *)malloc(fb_bytes);
    g_bitmap_pri = (u32 *)malloc(fb_bytes);
    g_record_bgra = (u32 *)malloc(fb_bytes);
    if (!g_bitmap || !g_bitmap_pri || !g_record_bgra) {
        fprintf(stderr, "lift: live view framebuffer alloc failed\n");
        sys24_viewer_shutdown();
        return -1;
    }

    g_open = 1;
    viewer_bring_to_front(g_window);
    (void)model2_snd_host_audio_open();
    viewer_record_try_start_from_env();
    lift_log(
            "lift: live view — %dx%d (scale %dx)%s, 5=coin 1=start F2=test/confirm 9/Down=menu F3=nvram, Space=pause, R=record, Esc=quit\n",
            SYS24_FB_WIDTH,
            SYS24_FB_HEIGHT,
            SYS24_VIEW_SCALE,
#ifdef I960_HOST_HAVE_GL
            ", OpenGL geo composite"
#else
            ""
#endif
    );
    return 0;
#endif
}

#ifdef I960_HOST_HAVE_SDL
static int viewer_is_splash_hold(u32 inner, u32 frame)
{
    /*
     * Splash bind seeds a negative hold in 0x20a808:
     *   inner2 → 0xfffffe5c (−420), inner4/6 → 0xffffff88 (−120).
     * Hold while signed frame < 0.
     */
    return (inner == 2u || inner == 4u || inner == 6u) && (i32)frame < 0;
}

/* Operator menu (main mode 4) is pure Sys24 tiles — no 3D underlay. */
static int viewer_is_test_menu(void)
{
    return i960_ld_u32(I960_WORKRAM, 0x202098, 0) == 4u;
}

/*
 * Game-start (main mode 3) and attract share MAME model2_v composite order:
 * even tilemaps under polygons, odd (priority) tilemaps over — see flip().
 */
static int viewer_is_game_start(void)
{
    return i960_ld_u32(I960_WORKRAM, 0x202098, 0) == 3u;
}

/*
 * Practice scene table @ 0x5BA880 slots [2]/3]: car_display @ 0x14D60 /
 * car_select @ 0x15200 (staged 0x5B3D60 / 0x5B4200). Champ table @ 0x5BA820
 * never reaches those. Detect either staged or ROM form.
 */
static int viewer_is_car_select_scene(void)
{
    u32 frame;
    u32 handler;
    u32 rom;
    const u32 tables[2] = { 0x005ba880u, 0x005ba820u };
    int t;

    if (!viewer_is_game_start())
        return 0;

    frame = i960_ld_u32(I960_WORKRAM, 0x2020ac, 0) & 15u;
    for (t = 0; t < 2; t++) {
        handler = i960_ld_u32(I960_WORKRAM, tables[t], frame << 2);
        if (handler == 0u)
            handler = model2_workram_mirror_u32(tables[t] + (frame << 2));
        if (handler == 0x00014d60u || handler == 0x00015200u
            || handler == 0x005b3d60u || handler == 0x005b4200u)
            return 1;
        /* Staged callx: 0x5A0000 + (rom - 0x1000). */
        if (handler >= 0x005a0000u && handler < 0x005c0000u) {
            rom = 0x1000u + (handler - 0x005a0000u);
            if (rom == 0x00014d60u || rom == 0x00015200u)
                return 1;
        }
    }
    return 0;
}

/* attract_hud_setup @ 0x1B940 / staged 0x5BA940 — START CGM, Sys24 only. */
static int viewer_is_start_banner(void)
{
    u32 frame;
    const u32 tables[2] = { 0x005ba820u, 0x005ba880u }; /* champ / practice */
    int t;

    /* Tables are always readable; only arm this during mode-3 game start. */
    if (!viewer_is_game_start())
        return 0;

    frame = i960_ld_u32(I960_WORKRAM, 0x2020ac, 0) & 15u;
    for (t = 0; t < 2; t++) {
        u32 handler = i960_ld_u32(I960_WORKRAM, tables[t], frame << 2);

        if (handler == 0u)
            handler = model2_workram_mirror_u32(tables[t] + (frame << 2));
        if (handler == 0x005ba940u || handler == 0x0001b940u)
            return 1;
        /* Staged callx: 0x5A0000 + (rom - 0x1000). */
        if (handler >= 0x005a0000u && handler < 0x005c0000u
            && (0x1000u + (handler - 0x005a0000u)) == 0x0001b940u)
            return 1;
    }
    return 0;
}

static int viewer_tiles_opaque(u32 inner, u32 frame)
{
    return viewer_is_splash_hold(inner, frame) || viewer_is_test_menu()
        || viewer_is_start_banner();
}

static void viewer_note_shot(void)
{
    u32 cycle = i960_ld_u32(I960_WORKRAM, 0x20a780, 0);
    u32 inner = i960_ld_u32(I960_WORKRAM, 0x20209c, 0);
    u32 course = i960_host_race_course_index();
    u32 tab = i960_ld_u32(I960_WORKRAM, 0x20a800, 0);
    u32 script = i960_ld_u32(I960_WORKRAM, 0x20a7c4, 0);
    u32 frame = i960_ld_u32(I960_WORKRAM, 0x20a808, 0);
    u32 phase = viewer_is_splash_hold(inner, frame) ? 0u : 1u;
    static u32 s_phase = 0xffffffffu;

    /* Attract loop close @ inner_8 bumps 0x20a780 — restart shot list. */
    if (cycle != g_shot_cycle) {
        g_shot_cycle = cycle;
        g_shot = 0;
        g_shot_inner = 0xffffffffu;
        g_shot_course = 0xffffffffu;
        g_shot_tab = 0xffffffffu;
        g_shot_script = 0xffffffffu;
        s_phase = 0xffffffffu;
    }

    /*
     * New shot on inner (covers splash 2/4/6), course, camera tab, script,
     * or splash→animate within the same inner.
     */
    if (inner != g_shot_inner || course != g_shot_course || tab != g_shot_tab
        || script != g_shot_script || phase != s_phase) {
        g_shot_inner = inner;
        g_shot_course = course;
        g_shot_tab = tab;
        g_shot_script = script;
        s_phase = phase;
        g_shot++;
        /*
         * Latch-backed shot id (not a ROM index). Log workram camera state so
         * shot N can be tied to script_finish mode + descriptor row from
         * disasm, without guessing from mesh/table shape.
         */
        {
            u32 cam = i960_ld_u32(I960_WORKRAM, 0x20a7f4, 0);
            u32 desc = i960_ld_u32(I960_WORKRAM, 0x20a80c, 0);
            u32 link = i960_ld_u32(I960_WORKRAM, 0x20a7fc, 0);
            float sx = 0.f, sy = 0.f, sz = 0.f;

            if (desc) {
                u32 row = desc + tab * 36u;

                sx = (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, row, 0));
                sy = (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, row, 4));
                sz = (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, row, 8));
            }
            lift_log(
                    "lift: shot=%u phase=%s course=%u tab=%u cam=%u "
                    "inner=%u script=%u desc=%#x link=%#x "
                    "scene=(%.3g,%.3g,%.3g)\n",
                    g_shot,
                    phase ? "animate" : "splash",
                    (unsigned)course, (unsigned)tab,
                    (unsigned)(cam & 0xffu), (unsigned)inner,
                    (unsigned)script, (unsigned)desc, (unsigned)link,
                    sx, sy, sz);
        }
    }
}

static void viewer_update_title(void)
{
    char title[224];
    u32 main_mode = i960_ld_u32(I960_WORKRAM, 0x202098, 0);
    u32 cam_mode = i960_ld_u32(I960_WORKRAM, 0x20a7f4, 0);
    u32 inner = i960_ld_u32(I960_WORKRAM, 0x20209c, 0);
    u32 frame = i960_ld_u32(I960_WORKRAM, 0x20a808, 0);
    u16 plays = (u16)i960_ld_u16(I960_ABS, 0x01d00022u, 0);
    u16 coins = (u16)i960_ld_u16(I960_ABS, 0x01d00020u, 0);
    int splash = viewer_is_splash_hold(inner, frame);
    int test_menu = viewer_is_test_menu();

    if (!g_window)
        return;

    if (!lift_verbose_enabled()) {
        SDL_SetWindowTitle(g_window, VIEWER_TITLE);
        return;
    }

    viewer_note_shot();
    if (test_menu)
        snprintf(title, sizeof(title),
                 "%s%s — mode %u/%u  TEST MENU",
                 VIEWER_TITLE,
                 g_paused ? " [PAUSED]" : "",
                 (unsigned)(main_mode & 15u),
                 (unsigned)(inner & 15u));
    else if (splash)
        snprintf(title, sizeof(title),
                 "%s%s — mode %u/%u  shot %u  splash  course %u",
                 VIEWER_TITLE,
                 g_paused ? " [PAUSED]" : "",
                 (unsigned)(main_mode & 15u),
                 (unsigned)(inner & 15u),
                 g_shot,
                 (unsigned)g_shot_course);
    else
        snprintf(title, sizeof(title),
                 "%s%s — mode %u/%u  shot %u  course %u  cam %u  credit %u/%u",
                 VIEWER_TITLE,
                 g_paused ? " [PAUSED]" : "",
                 (unsigned)(main_mode & 15u),
                 (unsigned)(inner & 15u),
                 g_shot,
                 (unsigned)g_shot_course,
                 (unsigned)(cam_mode & 0xffu),
                 (unsigned)coins,
                 (unsigned)plays);
    SDL_SetWindowTitle(g_window, title);
}

static void viewer_log_scene_ids(const char *why)
{
    u32 cam = i960_ld_u32(I960_WORKRAM, 0x20a7f4, 0);
    u32 inner = i960_ld_u32(I960_WORKRAM, 0x20209c, 0);
    u32 frame = i960_ld_u32(I960_WORKRAM, 0x20a808, 0);

    viewer_note_shot();
    lift_log(
            "lift: %s shot=%u%s course=%u cam=%u inner=%u\n",
            why, g_shot,
            viewer_is_splash_hold(inner, frame) ? " splash" : "",
            (unsigned)g_shot_course,
            (unsigned)(cam & 0xffu),
            (unsigned)inner);
}
#endif

int sys24_viewer_poll_events(void)
{
    if (!g_open)
        return 0;

#ifndef I960_HOST_HAVE_SDL
    return 0;
#else
    SDL_Event ev;
    int in_test;
    int race_go;

    in_test = (i960_ld_u32(I960_WORKRAM, 0x202098, 0) == 4u);
    race_go = ((i32)i960_ld_u32(I960_WORKRAM, 0x214120, 0) > 0);

    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_QUIT)
            return 1;
        if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) {
            int pressed = (ev.type == SDL_KEYDOWN);

            /* Cabinet: 5=COIN1, 1=START1, F2=TEST, 9=SERVICE1.
             * In test menu (mode 4): wheel/pedals also select —
             *   ←/→/↓ move (SERVICE1), ↑ confirm (TEST). */
            if (ev.key.keysym.sym == SDLK_5 && !ev.key.repeat)
                model2_io_in0_set_mask(MODEL2_IO_IN0_COIN1, pressed);
            if (ev.key.keysym.sym == SDLK_1 && !ev.key.repeat)
                model2_io_in0_set_mask(MODEL2_IO_IN0_START1, pressed);
            if ((ev.key.keysym.sym == SDLK_9
                 || (in_test && (ev.key.keysym.sym == SDLK_DOWN
                                 || ev.key.keysym.sym == SDLK_LEFT
                                 || ev.key.keysym.sym == SDLK_RIGHT)))
                && !ev.key.repeat)
                model2_io_in0_set_mask(MODEL2_IO_IN0_SERVICE1, pressed);
            if (ev.key.keysym.sym == SDLK_F2 && !ev.key.repeat) {
                model2_io_in0_set_mask(MODEL2_IO_IN0_TEST, pressed);
                if (pressed && !in_test)
                    model2_io_request_test_menu();
            }
            if (in_test && ev.key.keysym.sym == SDLK_UP && !ev.key.repeat)
                model2_io_in0_set_mask(MODEL2_IO_IN0_TEST, pressed);
            if (ev.key.keysym.sym == SDLK_v && !ev.key.repeat)
                model2_io_in0_set_mask(MODEL2_IO_IN0_VR, pressed);

            /*
             * Menu analog: ←/→ wheel steps, keyup latches while 2020a8==0.
             * After GO (214120>0) held keys drive analog each frame below —
             * 0x202050/51/52 from io_poll @ 0x26C0 (ldob 0x01C0001E).
             */
            if (!race_go && !in_test && !i960_host_skip_practice()) {
                if (ev.key.keysym.sym == SDLK_LEFT && pressed) {
                    u8 steer = model2_io_analog_get(MODEL2_IO_AN_STEER);
                    const u8 step = 0x08u;

                    model2_io_analog_set(MODEL2_IO_AN_STEER,
                                        steer > step ? (u8)(steer - step)
                                                     : 0x00u);
                }
                if (ev.key.keysym.sym == SDLK_RIGHT && pressed) {
                    u8 steer = model2_io_analog_get(MODEL2_IO_AN_STEER);
                    const u8 step = 0x08u;

                    model2_io_analog_set(MODEL2_IO_AN_STEER,
                                        steer < (u8)(0xffu - step)
                                            ? (u8)(steer + step)
                                            : 0xffu);
                }
                if ((ev.key.keysym.sym == SDLK_LEFT
                     || ev.key.keysym.sym == SDLK_RIGHT)
                    && !pressed
                    && i960_ld_u32(I960_WORKRAM, 0x2020a8, 0) != 0u)
                    model2_io_analog_set(MODEL2_IO_AN_STEER, 0x80u);
                if (ev.key.keysym.sym == SDLK_UP)
                    model2_io_analog_set(MODEL2_IO_AN_ACCEL,
                                        pressed ? 0xe0u : 0x00u);
                if (ev.key.keysym.sym == SDLK_DOWN)
                    model2_io_analog_set(MODEL2_IO_AN_BRAKE,
                                        pressed ? 0xe0u : 0x00u);
                if (ev.key.keysym.sym == SDLK_a && !ev.key.repeat)
                    model2_io_analog_set(MODEL2_IO_AN_ACCEL,
                                        pressed ? 0xe0u : 0x00u);
                if (ev.key.keysym.sym == SDLK_z && !ev.key.repeat)
                    model2_io_analog_set(MODEL2_IO_AN_BRAKE,
                                        pressed ? 0xe0u : 0x00u);
            }
            if (in_test && ev.key.keysym.sym == SDLK_a && !ev.key.repeat)
                model2_io_in0_set_mask(MODEL2_IO_IN0_TEST, pressed);
            if (in_test && ev.key.keysym.sym == SDLK_z && !ev.key.repeat)
                model2_io_in0_set_mask(MODEL2_IO_IN0_SERVICE1, pressed);
            /*
             * H-shifter @ IN1 bits 4–6 (io_poll lut 0x5A14B0 → 0x202044).
             * Q/[ down, E/] up; keypad 0=N, 1–4 direct. Latch, not tap.
             */
            if (pressed && !ev.key.repeat) {
                if (ev.key.keysym.sym == SDLK_q
                    || ev.key.keysym.sym == SDLK_LEFTBRACKET) {
                    if (g_shifter > 0u)
                        g_shifter--;
                } else if (ev.key.keysym.sym == SDLK_e
                           || ev.key.keysym.sym == SDLK_RIGHTBRACKET) {
                    if (g_shifter < 4u)
                        g_shifter++;
                } else if (ev.key.keysym.sym == SDLK_KP_0)
                    g_shifter = 0;
                else if (ev.key.keysym.sym == SDLK_KP_1)
                    g_shifter = 1u;
                else if (ev.key.keysym.sym == SDLK_KP_2)
                    g_shifter = 2u;
                else if (ev.key.keysym.sym == SDLK_KP_3)
                    g_shifter = 3u;
                else if (ev.key.keysym.sym == SDLK_KP_4)
                    g_shifter = 4u;
            }
        }
        if (ev.type == SDL_KEYDOWN) {
            if (ev.key.keysym.sym == SDLK_ESCAPE)
                return 1;
            if (ev.key.keysym.sym == SDLK_F3 && !ev.key.repeat) {
                if (model2_nvram_save(NULL) != 0)
                    fprintf(stderr, "lift: F3 nvram save failed\n");
            }
            if (ev.key.keysym.sym == SDLK_SPACE && !ev.key.repeat) {
                g_paused = !g_paused;
                viewer_log_scene_ids(g_paused ? "pause" : "resume");
                viewer_update_title();
            }
            if (ev.key.keysym.sym == SDLK_r && !ev.key.repeat) {
                if (sys24_viewer_record_active()) {
                    sys24_viewer_record_stop();
                } else {
                    const char *path = getenv("I960_HOST_RECORD");

                    if (!path || !*path || path[0] == '0')
                        path = "build/lift/attract_capture.avi";
                    if (sys24_viewer_record_start(path, SYS24_FB_WIDTH,
                                                  SYS24_FB_HEIGHT, 60) != 0)
                        fprintf(stderr, "lift: record start failed\n");
                }
            }
        }
    }
    /* --practice: poll can smash analog after skip_tick; re-hold Delta AT. */
    i960_host_skip_practice_hold_inputs();
    model2_io_shifter_set(g_shifter);
    /*
     * After GO: hold ←/→ wheel, ↑/W/A throttle, ↓/S/Z brake. io_poll copies
     * the mux into 0x202050/51/52; fov_scale / matrix_prep / control_byte
     * read those (gated on 214120 in the race leaves).
     */
    if ((i32)i960_ld_u32(I960_WORKRAM, 0x214120, 0) > 0 && !in_test) {
        const Uint8 *ks = SDL_GetKeyboardState(NULL);
        int left = ks[SDL_SCANCODE_LEFT] != 0;
        int right = ks[SDL_SCANCODE_RIGHT] != 0;
        int throttle = ks[SDL_SCANCODE_UP] || ks[SDL_SCANCODE_W]
            || ks[SDL_SCANCODE_A];
        int brake = ks[SDL_SCANCODE_DOWN] || ks[SDL_SCANCODE_S]
            || ks[SDL_SCANCODE_Z];
        u8 steer = 0x80u;

        if (left && !right)
            steer = 0x20u;
        else if (right && !left)
            steer = 0xe0u;
        model2_io_analog_set(MODEL2_IO_AN_STEER, steer);
        model2_io_analog_set(MODEL2_IO_AN_ACCEL, throttle ? 0xe0u : 0x00u);
        model2_io_analog_set(MODEL2_IO_AN_BRAKE, brake ? 0xe0u : 0x00u);
    }
    return 0;
#endif
}

#ifdef I960_HOST_HAVE_SDL
#ifdef I960_HOST_HAVE_GL
/* Largest centered rect of `aspect` that fits in the SDL window (letterbox). */
static void viewer_letterbox(int win_w, int win_h, float aspect,
                             int *out_x, int *out_y, int *out_w, int *out_h)
{
    int vw;
    int vh;

    if (win_w < 1)
        win_w = 1;
    if (win_h < 1)
        win_h = 1;
    if (!(aspect > 0.f))
        aspect = (float)SYS24_FB_WIDTH / (float)SYS24_FB_HEIGHT;

    vw = win_w;
    vh = (int)((float)vw / aspect + 0.5f);
    if (vh > win_h) {
        vh = win_h;
        vw = (int)((float)vh * aspect + 0.5f);
    }
    if (vw < 1)
        vw = 1;
    if (vh < 1)
        vh = 1;

    *out_x = (win_w - vw) / 2;
    *out_y = (win_h - vh) / 2;
    *out_w = vw;
    *out_h = vh;
}

/* Window-space letterbox → drawable GL viewport (bottom-left origin). */
static void viewer_letterbox_to_drawable(int win_w, int win_h, int dw, int dh,
                                         int vx, int vy, int vw, int vh,
                                         int *dx, int *dy, int *dww, int *dhh)
{
    float sx = (float)dw / (float)win_w;
    float sy = (float)dh / (float)win_h;

    *dww = (int)((float)vw * sx + 0.5f);
    *dhh = (int)((float)vh * sy + 0.5f);
    *dx = (int)((float)vx * sx + 0.5f);
    *dy = dh - (int)((float)(vy + vh) * sy + 0.5f);
}

/*
 * Sys24 over / under 3D: clear/pen-0 pixels are transparent (copybitmap_trans
 * key 0). Splash holds are full-frame CGM — keep black pens opaque.
 * Host words are 0xAARRGGBB. Uploads from g_tile_upload_src (or g_bitmap).
 */
static void viewer_tile_tex_upload(int keep_black)
{
    size_t n = (size_t)SYS24_FB_WIDTH * (size_t)SYS24_FB_HEIGHT;
    size_t i;
    unsigned opaque = 0;
    const u32 *src = g_tile_upload_src ? g_tile_upload_src : g_bitmap;

    if (!src)
        return;

    if (keep_black) {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SYS24_FB_WIDTH, SYS24_FB_HEIGHT,
                        GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, src);
        return;
    }

    if (!g_tile_alpha)
        g_tile_alpha = (u32 *)malloc(n * sizeof(u32));
    if (!g_tile_alpha) {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SYS24_FB_WIDTH, SYS24_FB_HEIGHT, GL_BGRA,
                        GL_UNSIGNED_INT_8_8_8_8_REV, src);
        return;
    }
    for (i = 0; i < n; i++) {
        u32 p = src[i];

        if ((p & 0x00ffffffu) == 0u)
            g_tile_alpha[i] = 0u;
        else {
            g_tile_alpha[i] = p | 0xff000000u;
            opaque++;
        }
    }
    {
        static unsigned s_hud_log;

        if (opaque > 0u && s_hud_log < 4u) {
            lift_log( "lift: sys24 HUD overlay opaque_px=%u / %u\n", opaque,
                    (unsigned)n);
            s_hud_log++;
        }
    }
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SYS24_FB_WIDTH, SYS24_FB_HEIGHT, GL_BGRA,
                    GL_UNSIGNED_INT_8_8_8_8_REV, g_tile_alpha);
}

static void draw_tile_quad(int vx, int vy, int vw, int vh, int fullscreen_opaque,
                           int keep_black, const u32 *src)
{
    glViewport(vx, vy, vw, vh);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    if (fullscreen_opaque) {
        glDisable(GL_BLEND);
    } else {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_tile_tex);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    g_tile_upload_src = src;
    viewer_tile_tex_upload(keep_black || fullscreen_opaque);
    g_tile_upload_src = NULL;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, SYS24_FB_WIDTH, SYS24_FB_HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glColor4f(1.f, 1.f, 1.f, 1.f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.f, 0.f);
    glVertex2f(0.f, 0.f);
    glTexCoord2f(1.f, 0.f);
    glVertex2f((float)SYS24_FB_WIDTH, 0.f);
    glTexCoord2f(1.f, 1.f);
    glVertex2f((float)SYS24_FB_WIDTH, (float)SYS24_FB_HEIGHT);
    glTexCoord2f(0.f, 1.f);
    glVertex2f(0.f, (float)SYS24_FB_HEIGHT);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}

static void geo_load_model2_projection(const model2_geo_projection_t *p, float znear)
{
    float width = (float)(p->viewport[2] - p->viewport[0]);
    float height = (float)(p->viewport[3] - p->viewport[1]);
    float xoff = 2.f * ((float)p->center[0] - (float)p->viewport[0]) / width - 1.f;
    float yoff = 1.f
               - 2.f * ((float)p->viewport[3] - (float)p->center[1]) / height;
    float m[16];

    memset(m, 0, sizeof(m));
    m[0] = 2.f / width;
    m[5] = 2.f / height;
    m[8] = xoff;
    m[9] = yoff;
    /*
     * Positive-Z infinite projection. MAME's Model 2 raster path clips the
     * view frustum and polygons wholly behind Z=0, but has no GL-style far
     * plane. A finite viewer far plane discarded valid track geometry.
     */
    m[10] = 1.f;
    m[11] = 1.f; /* clip W = positive Model 2 camera-space Z */
    m[14] = -2.f * znear;
    glLoadMatrixf(m);
}

static void draw_geo_layer(int vx, int vy, int vw, int vh,
                           const model2_geo_projection_t *hwproj, int have_hwproj,
                           int clear_color)
{
    unsigned nverts = 0;
    unsigned ntris = 0;
    const float *xyzuv = NULL;
    const model2_geo_tri_mat_t *mats = NULL;
    int have_tex = 0;
    unsigned npos = 0;
    unsigned nidx = 0;
    float *pos = NULL;
    unsigned *idx = NULL;

    have_tex = (model2_geo_lock_textured(&xyzuv, &nverts, &mats, &ntris) == 0
                && xyzuv && ntris > 0u);
    if (!have_tex) {
        /* Fall back to legacy XYZ+index flat shade. */
        if (model2_geo_copy_buffers(&pos, &npos, &idx, &nidx) != 0 || !pos
            || npos < 3u) {
            free(pos);
            free(idx);
            return;
        }
    }

    glViewport(vx, vy, vw, vh);
    glEnable(GL_SCISSOR_TEST);
    glScissor(vx, vy, vw, vh);
    if (clear_color) {
        glClearColor(0.04f, 0.05f, 0.08f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    } else {
        /* Over tile BG — reset depth+stencil so fillmap starts clean. */
        glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }
    glDisable(GL_SCISSOR_TEST);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glClearStencil(0);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (have_hwproj) {
        static int logged_projection;
        float hw_width = (float)(hwproj->viewport[2] - hwproj->viewport[0]);
        float hw_height = (float)(hwproj->viewport[3] - hwproj->viewport[1]);
        float hfov = 2.f * atanf(hw_width / (2.f * hwproj->focal_x))
                   * 180.f / 3.14159265f;
        float vfov = 2.f * atanf(hw_height / (2.f * hwproj->focal_y))
                   * 180.f / 3.14159265f;

        if (!logged_projection) {
            lift_log(
                    "lift: GEO projection focal=(%.3g,%.3g) window=%dx%d "
                    "center=(%d,%d) fov=(%.2f,%.2f)deg viewport=%dx%d "
                    "(camera-space MV)\n",
                    hwproj->focal_x, hwproj->focal_y,
                    (int)hw_width, (int)hw_height,
                    hwproj->center[0], hwproj->center[1], hfov, vfov,
                    vw, vh);
            logged_projection = 1;
        } else {
            static int s_cx = -1, s_cy = -1;
            static float s_fx = -1.f;

            if (hwproj->center[0] != s_cx || hwproj->center[1] != s_cy
                || hwproj->focal_x != s_fx) {
                lift_log(
                        "lift: GEO projection latch center=(%d,%d) "
                        "focal=(%.3g,%.3g)\n",
                        hwproj->center[0], hwproj->center[1],
                        hwproj->focal_x, hwproj->focal_y);
                s_cx = hwproj->center[0];
                s_cy = hwproj->center[1];
                s_fx = hwproj->focal_x;
            }
        }
        /*
         * Attract object draws post-compose pose onto the current TGP view
         * (copy_catalog: 0x20 / translate+rotate / draw / 0x21). Decoded
         * verts are already camera-space; loading the latched view again
         * double-transforms and makes camera motion look wrong.
         */
        geo_load_model2_projection(hwproj, 0.01f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    } else {
        static int logged_no_hwproj;

        /*
         * Transform-stack rule: no invented FOV / look-at / auto-frame.
         * Without GEO 0x09/0x03, draw identity — debugger waits for hw
         * projection rather than fabricating a second camera.
         */
        if (!logged_no_hwproj) {
            lift_log(
                    "lift: no GEO 0x09/0x03 projection yet — identity "
                    "GL (no invented FOV/look-at)\n");
            logged_no_hwproj = 1;
        }
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }

    if (have_tex) {
        static int logged_tex;
        if (!logged_tex) {
            lift_log( "lift: geo textured draw tris=%u verts=%u\n", ntris,
                    nverts);
            logged_tex = 1;
        }
        model2_geo_draw_textured(xyzuv, nverts, mats, ntris);
        model2_geo_unlock();
    } else if (idx && nidx >= 3u) {
        unsigned i, n = npos / 3u;
        glDisable(GL_TEXTURE_2D);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glBegin(GL_TRIANGLES);
        for (i = 0; i + 2u < nidx; i += 3u) {
            unsigned vi0 = idx[i];
            unsigned vi1 = idx[i + 1u];
            unsigned vi2 = idx[i + 2u];
            unsigned i0, i1, i2;
            float x0, y0, z0, x1, y1, z1, x2, y2, z2;
            float ax, ay, az, bx, by, bz, nx, ny, nz, nl, shade;

            if (vi0 >= n || vi1 >= n || vi2 >= n)
                continue;
            i0 = vi0 * 3u;
            i1 = vi1 * 3u;
            i2 = vi2 * 3u;
            x0 = pos[i0];
            y0 = pos[i0 + 1u];
            z0 = pos[i0 + 2u];
            x1 = pos[i1];
            y1 = pos[i1 + 1u];
            z1 = pos[i1 + 2u];
            x2 = pos[i2];
            y2 = pos[i2 + 1u];
            z2 = pos[i2 + 2u];
            ax = x1 - x0;
            ay = y1 - y0;
            az = z1 - z0;
            bx = x2 - x0;
            by = y2 - y0;
            bz = z2 - z0;
            nx = ay * bz - az * by;
            ny = az * bx - ax * bz;
            nz = ax * by - ay * bx;
            nl = sqrtf(nx * nx + ny * ny + nz * nz);

            if (nl > 1e-6f) {
                nx /= nl;
                ny /= nl;
                nz /= nl;
            }
            shade = 0.35f + 0.65f * fabsf(nx * 0.4f + ny * 0.8f + nz * 0.4f);
            if (shade > 1.f)
                shade = 1.f;
            glColor4f(0.45f * shade, 0.75f * shade, 1.f * shade, 1.f);
            glVertex3f(x0, y0, z0);
            glVertex3f(x1, y1, z1);
            glVertex3f(x2, y2, z2);
        }
        glEnd();
    } else if (pos && npos >= 3u) {
        unsigned i, n = npos / 3u;
        glPointSize(3.f);
        glColor4f(0.9f, 0.95f, 1.f, 0.9f);
        glBegin(GL_POINTS);
        for (i = 0; i < n; i++)
            glVertex3f(pos[i * 3u], pos[i * 3u + 1u], pos[i * 3u + 2u]);
        glEnd();
    }

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    free(pos);
    free(idx);
}
#endif
#endif

int sys24_viewer_flip(const u8 *tile_map, const u8 *char_ram, const u8 *palram)
{
#ifndef I960_HOST_HAVE_SDL
    (void)tile_map;
    (void)char_ram;
    (void)palram;
    return 0;
#else
    if (!g_open)
        return 0;

    if (sys24_viewer_poll_events() != 0)
        return 1;

    /*
     * While paused, do not kick geo decode or advance tiles from new game
     * state — flip is called from geo_vsync_wait, so blocking here freezes
     * the i960 loop on the current frame.
     */
    {
        u32 inner = i960_ld_u32(I960_WORKRAM, 0x20209c, 0);
        u32 frame = i960_ld_u32(I960_WORKRAM, 0x20a808, 0);
        int opaque2d = viewer_tiles_opaque(inner, frame);
        static int s_was_opaque2d;

        /*
         * Splash / operator menu are Sys24 tiles only. Drop stale PRG mesh and
         * skip geo kick so transparent black cells do not reveal the track.
         */
        if (opaque2d && !s_was_opaque2d)
            model2_geo_clear();
        if (opaque2d != s_was_opaque2d) {
            g_tile_valid = 0;
            s_was_opaque2d = opaque2d;
        }

        if (!g_paused && !opaque2d)
            model2_geo_kick();

        {
            u32 sig = viewer_tile_sig(tile_map, char_ram, palram);

            if (!g_paused && (!g_tile_valid || sig != g_tile_sig)) {
                sys24_tile_bind(g_tile, tile_map, char_ram);
                /*
                 * Attract ranking leaves ctrl bit14 (0x4000) in 0x20b91c →
                 * tile_ram[0x5004] (pairs 0–1 only). game_start_* only
                 * clrbit15, so special-window stays armed and MAME segs24
                 * skips odd layers of those pairs. Strip bit14 on game-start.
                 * Car-select BG @ 0x01004000 is pair index 2 (0x20b918/920);
                 * car_display zeros that scroll/ctrl before the g4=1 CGM.
                 */
                {
                    u8 *reg = NULL;
                    u16 ctrl_save = 0;
                    int strip_ranking = 0;

                    if (viewer_is_game_start()) {
                        reg = model2_ram_mut(0x0100a008u);
                        if (reg) {
                            u16 ctrl = (u16)reg[0] | ((u16)reg[1] << 8);

                            ctrl_save = ctrl;
                            if (ctrl & 0x4000u) {
                                ctrl = (u16)(ctrl & ~0x4000u);
                                reg[0] = (u8)ctrl;
                                reg[1] = (u8)(ctrl >> 8);
                                strip_ranking = 1;
                            }
                        }
                    }
                    if (opaque2d) {
                        /* Splash/test: single opaque ALL-layer pass. */
                        sys24_tile_draw_layers_rgb32(
                            g_tile, g_bitmap, palram, 0xff000000u, SYS24_PASS_ALL);
                    } else {
                        /*
                         * MAME model2_v: even layers under polygons, odd over.
                         * Transparent clear (0) — pen-0 punches like
                         * copybitmap_trans.
                         */
                        sys24_tile_draw_layers_rgb32(
                            g_tile, g_bitmap, palram, 0x00000000u,
                            SYS24_PASS_BOTTOM);
                        if (g_bitmap_pri)
                            sys24_tile_draw_layers_rgb32(
                                g_tile, g_bitmap_pri, palram, 0x00000000u,
                                SYS24_PASS_PRIORITY);
                        if (viewer_is_game_start() && g_bitmap_pri) {
                            static int s_pri_log;
                            static int s_car_sel_log;
                            size_t n = (size_t)SYS24_FB_WIDTH
                                     * (size_t)SYS24_FB_HEIGHT;
                            size_t i;
                            unsigned opaque = 0;
                            unsigned third[3] = {0, 0, 0};

                            if (viewer_is_car_select_scene() && s_car_sel_log < 2) {
                                u16 reg[8];
                                unsigned ri;
                                u32 nz2 = 0, nz0 = 0;
                                u8 *rr = model2_ram_mut(0x0100a000u);
                                u8 *map0 = model2_ram_mut(0x01000000u);
                                u8 *map2 = model2_ram_mut(0x01004000u);

                                for (ri = 0; ri < 8u; ri++) {
                                    reg[ri] = 0;
                                    if (rr)
                                        reg[ri] = (u16)rr[ri * 2u]
                                                | ((u16)rr[ri * 2u + 1u] << 8);
                                }
                                if (map0) {
                                    for (ri = 0; ri < 0x2000u; ri++) {
                                        u16 t = (u16)map0[ri * 2u]
                                              | ((u16)map0[ri * 2u + 1u] << 8);
                                        if (t)
                                            nz0++;
                                    }
                                }
                                if (map2) {
                                    for (ri = 0; ri < 0x2000u; ri++) {
                                        u16 t = (u16)map2[ri * 2u]
                                              | ((u16)map2[ri * 2u + 1u] << 8);
                                        if (t)
                                            nz2++;
                                    }
                                }
                                lift_log(
                                        "lift: car_select scroll "
                                        "h=%#x/%#x/%#x/%#x v=%#x/%#x/%#x/%#x "
                                        "strip14=%d map0_nz=%u map2_nz=%u "
                                        "(BG@0x4000 uses h[2]/v[2])\n",
                                        (unsigned)reg[0], (unsigned)reg[1],
                                        (unsigned)reg[2], (unsigned)reg[3],
                                        (unsigned)reg[4], (unsigned)reg[5],
                                        (unsigned)reg[6], (unsigned)reg[7],
                                        strip_ranking, nz0, nz2);
                                s_car_sel_log++;
                            }
                            for (i = 0; i < n; i++) {
                                if ((g_bitmap_pri[i] & 0x00ffffffu) != 0u) {
                                    unsigned col =
                                        (unsigned)(i % SYS24_FB_WIDTH);

                                    opaque++;
                                    if (col < SYS24_FB_WIDTH / 3u)
                                        third[0]++;
                                    else if (col < 2u * SYS24_FB_WIDTH / 3u)
                                        third[1]++;
                                    else
                                        third[2]++;
                                }
                            }
                            if (s_pri_log < 6) {
                                lift_log(
                                        "lift: mode3 priority opaque_px=%u / %u "
                                        "x_thirds=%u/%u/%u\n",
                                        opaque, (unsigned)n,
                                        third[0], third[1], third[2]);
                                s_pri_log++;
                            }
                        }
#ifndef I960_HOST_HAVE_GL
                        /* Software path has no geo: fold priority over bottom. */
                        if (g_bitmap_pri) {
                            size_t n = (size_t)SYS24_FB_WIDTH
                                     * (size_t)SYS24_FB_HEIGHT;
                            size_t i;

                            for (i = 0; i < n; i++) {
                                u32 p = g_bitmap_pri[i];

                                if ((p & 0x00ffffffu) != 0u)
                                    g_bitmap[i] = p | 0xff000000u;
                            }
                        }
#endif
                    }
                    if (strip_ranking && reg) {
                        reg[0] = (u8)ctrl_save;
                        reg[1] = (u8)(ctrl_save >> 8);
                    }
                }
                g_tile_sig = sig;
                g_tile_valid = 1;
                g_tile_tex_dirty = 1;
            }
        }

#ifdef I960_HOST_HAVE_GL
        {
            int win_w = 0, win_h = 0;
            int dw = 0, dh = 0;
            int gvx, gvy, gvw, gvh;
            int hvx, hvy, hvw, hvh;
            int gdx, gdy, gdww, gdhh;
            int hdx, hdy, hdww, hdhh;
            model2_geo_projection_t hwproj;
            int have_hwproj;
            float geo_aspect;
            float hud_aspect;
            static int logged_aspect;

            SDL_GL_MakeCurrent(g_window, g_gl);
            SDL_GetWindowSize(g_window, &win_w, &win_h);
            SDL_GL_GetDrawableSize(g_window, &dw, &dh);
            if (win_w < 1)
                win_w = 1;
            if (win_h < 1)
                win_h = 1;
            if (dw < 1)
                dw = win_w;
            if (dh < 1)
                dh = win_h;

            have_hwproj = model2_geo_projection(&hwproj);
            /*
             * 4:3: geo + HUD share the Sys24 letterbox.
             * 16:9: geo fills widescreen; HUD/tiles stay centered Sys24
             * proportions (no horizontal stretch). GEO 0x03 still drives
             * the projection matrix only — not the SDL layout.
             */
            geo_aspect = model2_host_geo_aspect();
            hud_aspect = model2_host_hud_aspect();
            viewer_letterbox(win_w, win_h, geo_aspect, &gvx, &gvy, &gvw, &gvh);
            viewer_letterbox(win_w, win_h, hud_aspect, &hvx, &hvy, &hvw, &hvh);
            viewer_letterbox_to_drawable(win_w, win_h, dw, dh, gvx, gvy, gvw, gvh,
                                         &gdx, &gdy, &gdww, &gdhh);
            viewer_letterbox_to_drawable(win_w, win_h, dw, dh, hvx, hvy, hvw, hvh,
                                         &hdx, &hdy, &hdww, &hdhh);

            if (!logged_aspect) {
                lift_log(
                        "lift: composite geo=%dx%d hud=%dx%d aspect=%s\n",
                        gvw, gvh, hvw, hvh,
                        model2_host_aspect_is_widescreen() ? "16:9" : "4:3");
                logged_aspect = 1;
            }

            /* Pillarbox / letterbox bars outside the Model 2 raster. */
            glViewport(0, 0, dw, dh);
            glClearColor(0.f, 0.f, 0.f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

            /*
             * MAME model2_v screen_update:
             *   even tilemaps → polygons → odd/priority tilemaps.
             * Game-start CGMs land on priority via draw_scene bit15; pen-0
             * punches reveal TGP underneath. No per-screen draw-order
             * exceptions — car-select panel X is ROM-baked in the catalogs.
             *
             * Geo must never clear COLOR when overlaying bottom tiles.
             * Widescreen: tiles use hud rect; geo uses full geo rect.
             */
            if (opaque2d) {
                draw_tile_quad(hdx, hdy, hdww, hdhh, 1, 1, g_bitmap);
            } else {
                draw_tile_quad(hdx, hdy, hdww, hdhh, 0, 0, g_bitmap);
                draw_geo_layer(gdx, gdy, gdww, gdhh,
                               have_hwproj ? &hwproj : NULL, have_hwproj, 0);
                if (g_bitmap_pri)
                    draw_tile_quad(hdx, hdy, hdww, hdhh, 0, 0, g_bitmap_pri);
            }
            g_tile_tex_dirty = 0;
            /* Capture geo letterbox (includes side pillars in 16:9). */
            viewer_record_from_gl_letterbox(gvx, gvy, gvw, gvh);
            SDL_GL_SwapWindow(g_window);
        }
#else
        {
            void *pixels;
            int pitch;
            if (SDL_LockTexture(g_texture, NULL, &pixels, &pitch) != 0) {
                fprintf(stderr, "lift: SDL_LockTexture failed: %s\n", SDL_GetError());
                return 0;
            }
            {
                const u8 *src = (const u8 *)g_bitmap;
                u8 *dst = (u8 *)pixels;
                int y;
                for (y = 0; y < SYS24_FB_HEIGHT; y++) {
                    memcpy(dst + (size_t)y * (size_t)pitch,
                           src + (size_t)y * (size_t)SYS24_FB_WIDTH * 4u,
                           (size_t)SYS24_FB_WIDTH * 4u);
                }
            }
            SDL_UnlockTexture(g_texture);
            viewer_record_submit_native(g_bitmap);
            SDL_RenderClear(g_renderer);
            SDL_RenderCopy(g_renderer, g_texture, NULL, NULL);
            SDL_RenderPresent(g_renderer);
        }
#endif
    }

    viewer_update_title();

    /* Space: hold here so vsync_wait / game loop do not advance. */
    while (g_paused) {
        SDL_Delay(16);
        if (sys24_viewer_poll_events() != 0)
            return 1;
#ifdef I960_HOST_HAVE_GL
        SDL_GL_SwapWindow(g_window);
#else
        SDL_RenderClear(g_renderer);
        SDL_RenderCopy(g_renderer, g_texture, NULL, NULL);
        SDL_RenderPresent(g_renderer);
#endif
        viewer_update_title();
    }

    return 0;
#endif
}

int sys24_viewer_present(const u8 *tile_map, const u8 *char_ram, const u8 *palram)
{
    if (!g_open)
        return 0;

#ifndef I960_HOST_HAVE_SDL
    (void)tile_map;
    (void)char_ram;
    (void)palram;
    return 0;
#else
    return sys24_viewer_flip(tile_map, char_ram, palram);
#endif
}

void sys24_viewer_shutdown(void)
{
    /* Persist NVRAM while workram / backup SRAM are still live. */
    (void)model2_nvram_save(NULL);
    sys24_viewer_record_stop();
#ifdef I960_HOST_HAVE_SDL
#ifdef I960_HOST_HAVE_GL
    if (g_tile_tex) {
        glDeleteTextures(1, &g_tile_tex);
        g_tile_tex = 0;
    }
    if (g_gl) {
        SDL_GL_DeleteContext(g_gl);
        g_gl = NULL;
    }
#else
    if (g_texture) {
        SDL_DestroyTexture(g_texture);
        g_texture = NULL;
    }
    if (g_renderer) {
        SDL_DestroyRenderer(g_renderer);
        g_renderer = NULL;
    }
#endif
    if (g_window) {
        SDL_DestroyWindow(g_window);
        g_window = NULL;
    }
    model2_snd_host_audio_close();
    if (g_open)
        SDL_Quit();
    sys24_tile_destroy(g_tile);
    g_tile = NULL;
    free(g_bitmap);
    g_bitmap = NULL;
    free(g_bitmap_pri);
    g_bitmap_pri = NULL;
    free(g_tile_alpha);
    g_tile_alpha = NULL;
    free(g_record_bgra);
    g_record_bgra = NULL;
#endif
    g_open = 0;
    model2_geo_shutdown();
    model2_hw_vsync_hw_shutdown();
}
