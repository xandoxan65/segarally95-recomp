#ifndef TRACK_VIEWER_H
#define TRACK_VIEWER_H

#include <stdint.h>

/* Host viewer options (boot screen, CGM decode, geo FIFO decode). */

typedef struct track_viewer_opts {
    const char *palette_dump;  /* --palette-dump DIR; NULL = do not write a snapshot */
    int viewer_boot;           /* 1 = cold-boot viewer (default unless --harness) */
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

/* Natural cold boot → main loop for attract/title (no desert scene hack).
 * Halts on copyright tile-script milestone; framebuffer from C sys24_tile. */
int i960_lift_boot_screen_run(const track_viewer_opts_t *opts);

/* Isolated CGM catalog → tile map via lifted catalog_draw_setup chain; sys24 PNG dump. */
int i960_lift_cgm_decode_run(const track_viewer_opts_t *opts);

/* Offline geo FIFO display-list decode → JSON summary (no boot loop). */
int i960_lift_geo_fifo_decode_run(const track_viewer_opts_t *opts);

#endif
