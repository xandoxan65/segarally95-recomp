#ifndef TRACK_VIEWER_H
#define TRACK_VIEWER_H

#include <stdint.h>

/* Track viewer — lifted palette + asset export for the web viewer.
 *
 * Run:  decomp_lift --viewer track --course desert [--out DIR] [--palette-only]
 *
 * Palette: geo_renderer_init builds colorxlat (0x3C80 + 0x4350 over ROM
 * 0x5A2EB4). GEO colorbases come from lifted boot/refresh/mode5
 * (table @ 0x5FB89E → 0x01802000) — same writers attract desert uses.
 * Mesh / palette_cache bake is C-only when ported (export currently skips).
 */

typedef struct track_viewer_opts {
    const char *course;        /* e.g. "desert" */
    const char *out_root;      /* segamod2 out/ (palette_cache + scenes) */
    const char *palette_dump;  /* decomp/build/lift/palette_state */
    int palette_only;          /* skip mesh / PNG bake */
    int geo_frames;            /* optional geo_draw_frame_entry iterations (0=skip) */
    int viewer_boot;           /* 1 = attract boot path (--viewer boot) */
    int boot_frames;           /* frames to run before dump (boot viewer) */
    int live_view;             /* 1 = SDL live framebuffer (boot default) */
    int headless;              /* 1 = force PNG-only boot (--headless) */
    const char *record_path;   /* --record FILE.avi|mp4 (ffmpeg pipe, async) */
    int cgm_decode;            /* 1 = --decode-cgm isolated CGM → sys24 PNG */
    uint32_t cgm_vaddr;        /* main_data vaddr or byte offset (see CLI) */
    uint32_t cgm_mode;         /* catalog_draw_setup mode (g3); splash = 0 */
    uint32_t cgm_flags;        /* catalog_draw_setup flags (g4); splash L2 = 1 */
    int cgm_splash_preamble;   /* run inner_2 tail: palette index + splash_bind */
    const char *geo_fifo_path; /* --decode-geo-fifo FILE.bin (prg_fifo words) */
    const char *geo_summary;   /* JSON summary output path */
    int skip_practice;         /* 1 = --practice: desert START with Delta AT */
    const char *aspect;        /* "4:3" (default) or "16:9" / --widescreen */
} track_viewer_opts_t;

/* Parse argv. Returns 1=viewer mode, 2=help printed, 0=not viewer, -1=error. */
int track_viewer_cli_parse(int argc, char **argv, track_viewer_opts_t *opts);

void track_viewer_cli_help(void);

/* Lifted palette boot + optional geo feed + dump + export. Returns exit code. */
int i960_lift_track_viewer_run(const track_viewer_opts_t *opts);

/* Natural cold boot → main loop for attract/title (no desert scene hack).
 * Halts on copyright tile-script milestone; framebuffer from C sys24_tile. */
int i960_lift_boot_screen_run(const track_viewer_opts_t *opts);

/* Isolated CGM catalog → tile map via lifted catalog_draw_setup chain; sys24 PNG dump. */
int i960_lift_cgm_decode_run(const track_viewer_opts_t *opts);

/* Offline geo FIFO display-list decode → JSON summary (no boot loop). */
int i960_lift_geo_fifo_decode_run(const track_viewer_opts_t *opts);

#endif
