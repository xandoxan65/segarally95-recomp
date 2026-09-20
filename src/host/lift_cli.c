/* CLI for segamod2 host modes (--viewer, --help). */

#include "track_viewer.h"
#include "model2_snd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int streq(const char *a, const char *b)
{
    return a && b && strcmp(a, b) == 0;
}

static char s_default_out[512];
static char s_copro_dump[512];

static void cli_setenv(const char *key, const char *value)
{
#if defined(_WIN32)
    static char buf[768];

    snprintf(buf, sizeof(buf), "%s=%s", key, value);
    _putenv(buf);
#else
    setenv(key, value, 1);
#endif
}

static const char *optional_path(int *i, int argc, char **argv, const char *fallback)
{
    if (*i + 1 < argc && argv[*i + 1][0] != '-')
        return argv[++(*i)];
    return fallback;
}

static void defaults(track_viewer_opts_t *opts)
{
    const char *root = getenv("SEGAMOD2_ROOT");

    if (!root || !*root)
        root = "..";

    snprintf(s_default_out, sizeof(s_default_out), "%s/out", root);
    opts->course = "desert";
    opts->out_root = s_default_out;
    opts->palette_dump = "build/lift/palette_state";
    opts->palette_only = 0;
    opts->geo_frames = 0;
    opts->viewer_boot = 0;
    opts->boot_frames = 0;
    opts->live_view = 0;
    opts->headless = 0;
    opts->record_path = NULL;
    opts->cgm_decode = 0;
    opts->cgm_vaddr = 0;
    opts->cgm_mode = 0;
    opts->cgm_flags = 1;
    opts->cgm_splash_preamble = 1;
    opts->geo_fifo_path = NULL;
    opts->geo_summary = "build/lift/geo_decode_summary.json";
    opts->skip_practice = 0;
    opts->aspect = "4:3";
}

void track_viewer_cli_help(void)
{
    fprintf(stderr,
            "segamod2 — lifted Sega Rally host\n"
            "\n"
            "No arguments: SDL cold-boot viewer (copyright → attract).\n"
            "  --harness              short dispatch trace (I960_HOST_MAX_DISPATCH, default 64)\n"
            "\n"
            "Viewer:\n"
            "  --viewer track [--course desert] [--out DIR] [--palette-only] [--geo-frames N]\n"
            "  --viewer boot [--palette-dump DIR] [--headless] [--record FILE] [--practice]\n"
            "  --live                 SDL window (boot viewer default)\n"
            "  --headless             PNG dump only, no SDL window\n"
            "  --aspect 4:3|16:9      live composite (default 4:3 arcade)\n"
            "  --widescreen           alias for --aspect 16:9\n"
            "                         16:9: wider 3D HFOV; HUD/tiles stay centered 4:3\n"
            "                         also: I960_HOST_ASPECT=16:9\n"
            "  --practice             skip attract/menus → desert practice START (Delta MT)\n"
            "                         also: I960_HOST_SKIP_PRACTICE=1\n"
            "  --record FILE          pipe live frames to ffmpeg (*.avi=mjpeg, else x264)\n"
            "                         async writer; drops frames if encode lags (low latency)\n"
            "                         also: I960_HOST_RECORD=FILE or press R in the window\n"
            "\n"
            "  track: lifted palette + CGM bridge + asset export for web viewer\n"
            "  boot:  full cold boot (copyright → attract; --practice jumps to desert START)\n"
            "\n"
            "CGM decode (isolated catalog → sys24 PNG, splash call chain):\n"
            "  --decode-cgm ADDR   main_data vaddr (e.g. 0x2879db0) or byte offset\n"
            "  [--cgm-mode N]      catalog_draw_setup mode g3 (default 0)\n"
            "  [--cgm-flags N]     catalog_draw_setup flags g4 (default 1 = L2)\n"
            "  [--no-splash-tail]  skip comm_palette_index_call + splash_bind\n"
            "  [--palette-dump DIR]  output dir (default build/lift/cgm_decode)\n"
            "\n"
            "Geo FIFO decode (C DL runner — Python template port):\n"
            "  --decode-geo-fifo FILE.bin   little-endian u32 prg_fifo dump\n"
            "  [--geo-summary PATH]        JSON summary (default build/lift/geo_decode_summary.json)\n"
            "\n"
            "Logging (independent; work with --viewer boot or the default harness):\n"
            "  --log-geo [PATH]    dump PRG FIFO at exit (default build/lift/geo_fifo.bin)\n"
            "                      copro FIFO → PATH.copro or build/lift/copro_fifo.bin\n"
            "                      also: I960_GEO_DUMP / I960_COPRO_DUMP\n"
            "  --log-sound [PATH]  print 3-byte sound commands to stderr; dump MIDI ring\n"
            "                      at exit (default build/lift/sound_midi.log)\n"
            "                      also: I960_SND_LOG=1  I960_SND_DUMP=PATH\n"
            "\n"
            "  I960_PALETTE_DUMP=<dir>  palette snapshot (also --palette-dump)\n"
            "  I960_GEO_SUMMARY=<path>  dump geo mesh summary at boot end\n"
            "  I960_GEO_DUMP=<path>     PRG FIFO dump at boot end (also --log-geo)\n"
            "  I960_COPRO_DUMP=<path>   copro FIFO dump at boot end\n"
            "  I960_SND_LOG=1           stderr MIDI/command log (also --log-sound)\n"
            "  I960_SND_DUMP=<path>     MIDI byte dump at boot end\n"
            "  I960_GEO_FLAT=1          optional gray geo (default off = full colour)\n"
            "  I960_GEO_FOV_SCALE=<f>   extra uniform focal scale (0.25..4; after aspect)\n"
            "  I960_HOST_ASPECT=4:3|16:9  live geo/HUD layout (also --aspect)\n"
            "  I960_HOST_NVRAM=<path>   backup SRAM + options YAML (default build/lift/nvram.yaml)\n"
            "\n"
            "Live keys: 5=coin  1=start  F2=test/confirm  9/Down=menu select  F3=save nvram  Space=pause  R=record  Esc=quit\n"
            "           Q/[ downshift  E/] upshift  keypad 0=N 1–4=gear  (IN1 H-shifter)\n"
            "\n");
}

int track_viewer_cli_parse(int argc, char **argv, track_viewer_opts_t *opts)
{
    int i;
    int viewer = 0;
    int harness = 0;

    if (!opts)
        return -1;
    defaults(opts);

    for (i = 1; i < argc; i++) {
        if (streq(argv[i], "--harness")) {
            harness = 1;
            continue;
        }
        if (streq(argv[i], "--help") || streq(argv[i], "-h")) {
            track_viewer_cli_help();
            return 2;
        }
        if (streq(argv[i], "--viewer")) {
            viewer = 1;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                if (streq(argv[i + 1], "track") || streq(argv[i + 1], "track-desert")) {
                    i++;
                    if (streq(argv[i], "track-desert"))
                        opts->course = "desert";
                } else if (streq(argv[i + 1], "boot")) {
                    i++;
                    opts->viewer_boot = 1;
                    opts->live_view = 1;
                }
            }
            continue;
        }
        if (streq(argv[i], "--frames") && i + 1 < argc) {
            opts->boot_frames = (int)strtol(argv[++i], NULL, 0);
            continue;
        }
        if (streq(argv[i], "--course") && i + 1 < argc) {
            opts->course = argv[++i];
            continue;
        }
        if (streq(argv[i], "--out") && i + 1 < argc) {
            opts->out_root = argv[++i];
            continue;
        }
        if (streq(argv[i], "--palette-dump") && i + 1 < argc) {
            opts->palette_dump = argv[++i];
            continue;
        }
        if (streq(argv[i], "--palette-only")) {
            opts->palette_only = 1;
            continue;
        }
        if (streq(argv[i], "--geo-frames") && i + 1 < argc) {
            opts->geo_frames = (int)strtol(argv[++i], NULL, 0);
            continue;
        }
        if (streq(argv[i], "--live")) {
            opts->live_view = 1;
            opts->headless = 0;
            continue;
        }
        if (streq(argv[i], "--headless")) {
            opts->headless = 1;
            opts->live_view = 0;
            continue;
        }
        if (streq(argv[i], "--widescreen")) {
            opts->aspect = "16:9";
            continue;
        }
        if (streq(argv[i], "--aspect") && i + 1 < argc) {
            const char *a = argv[++i];

            if (streq(a, "4:3") || streq(a, "4x3") || streq(a, "arcade")) {
                opts->aspect = "4:3";
            } else if (streq(a, "16:9") || streq(a, "16x9") || streq(a, "widescreen")
                       || streq(a, "wide")) {
                opts->aspect = "16:9";
            } else {
                fprintf(stderr,
                        "lift: --aspect expects 4:3 or 16:9 (got %s)\n", a);
                return -1;
            }
            continue;
        }
        if (streq(argv[i], "--record") && i + 1 < argc) {
            opts->record_path = argv[++i];
            continue;
        }
        if (streq(argv[i], "--practice") || streq(argv[i], "--skip-to-practice")) {
            opts->skip_practice = 1;
            opts->viewer_boot = 1;
            viewer = 1;
            if (!opts->headless)
                opts->live_view = 1;
            continue;
        }
        if (streq(argv[i], "--decode-cgm") && i + 1 < argc) {
            opts->cgm_decode = 1;
            opts->cgm_vaddr = (uint32_t)strtoul(argv[++i], NULL, 0);
            if (streq(opts->palette_dump, "build/lift/palette_state"))
                opts->palette_dump = "build/lift/cgm_decode";
            viewer = 1;
            continue;
        }
        if (streq(argv[i], "--cgm-mode") && i + 1 < argc) {
            opts->cgm_mode = (uint32_t)strtoul(argv[++i], NULL, 0);
            continue;
        }
        if (streq(argv[i], "--cgm-flags") && i + 1 < argc) {
            opts->cgm_flags = (uint32_t)strtoul(argv[++i], NULL, 0);
            continue;
        }
        if (streq(argv[i], "--no-splash-tail")) {
            opts->cgm_splash_preamble = 0;
            continue;
        }
        if (streq(argv[i], "--decode-geo-fifo") && i + 1 < argc) {
            opts->geo_fifo_path = argv[++i];
            viewer = 1;
            continue;
        }
        if (streq(argv[i], "--geo-summary") && i + 1 < argc) {
            opts->geo_summary = argv[++i];
            continue;
        }
        if (streq(argv[i], "--log-geo") || streq(argv[i], "--geo-log")) {
            const char *path = optional_path(&i, argc, argv, "build/lift/geo_fifo.bin");

            cli_setenv("I960_GEO_DUMP", path);
            snprintf(s_copro_dump, sizeof(s_copro_dump), "%s.copro", path);
            if (streq(path, "build/lift/geo_fifo.bin"))
                cli_setenv("I960_COPRO_DUMP", "build/lift/copro_fifo.bin");
            else
                cli_setenv("I960_COPRO_DUMP", s_copro_dump);
            continue;
        }
        if (streq(argv[i], "--log-sound") || streq(argv[i], "--sound-log")) {
            const char *path = optional_path(&i, argc, argv, "build/lift/sound_midi.log");

            cli_setenv("I960_SND_LOG", "1");
            cli_setenv("I960_SND_DUMP", path);
            model2_snd_set_log(1);
            fprintf(stderr,
                    "lift: sound logging enabled — commands/MIDI on stderr, dump %s\n",
                    path);
            continue;
        }
        fprintf(stderr, "lift: unknown option: %s (try --help)\n", argv[i]);
        return -1;
    }

    /* Bare `segamod2` is the live boot viewer — the furthest runnable uplift. */
    if (!viewer && !harness) {
        opts->viewer_boot = 1;
        opts->live_view = 1;
        if (!opts->palette_dump || streq(opts->palette_dump, "build/lift/palette_state"))
            opts->palette_dump = "build/lift/boot_copyright";
        viewer = 1;
    }

    if (opts->viewer_boot && opts->headless)
        opts->live_view = 0;
    if (opts->record_path && *opts->record_path)
        opts->live_view = 1; /* recording needs the present path */

    /* Publish aspect for GEO latch + live viewer (same pattern as --live). */
    if (opts->aspect && *opts->aspect) {
#if defined(_WIN32)
        {
            static char aspect_env[64];

            snprintf(aspect_env, sizeof(aspect_env), "I960_HOST_ASPECT=%s",
                     opts->aspect);
            _putenv(aspect_env);
        }
#else
        setenv("I960_HOST_ASPECT", opts->aspect, 1);
#endif
    }

    return viewer ? 1 : 0;
}
