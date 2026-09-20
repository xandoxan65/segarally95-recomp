/* Boot harness: cold boot through attract/title tile path.
 *
 * Flow (disasm @ 0x1AA30): SOUND INITIALIZE → countdown → copyright → attract modes.
 * Headless: stops after I960_HOST_MAX_DISPATCH (default 20000) or milestone halt.
 * Live (--live): runs until the SDL window is closed (Escape / quit).
 */

#include "track_viewer.h"
#include "i960_host.h"
#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_rom.h"
#include "model2_nvram.h"
#include "model2_geo_lift.h"
#include "sys24_viewer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BOOT_HEADLESS_DISPATCH_LIMIT 150000u

static void set_live_view_env(int on)
{
#if defined(_WIN32)
    _putenv(on ? "I960_HOST_LIVE_VIEW=1" : "I960_HOST_LIVE_VIEW=0");
#else
    setenv("I960_HOST_LIVE_VIEW", on ? "1" : "0", 1);
#endif
}

static void set_boot_env(const char *key, const char *value)
{
#if defined(_WIN32)
    static char buf[512];
    snprintf(buf, sizeof(buf), "%s=%s", key, value);
    _putenv(buf);
#else
    setenv(key, value, 1);
#endif
}

int i960_lift_boot_screen_run(const track_viewer_opts_t *opts)
{
    const char *dump;
    char dispatch_limit[16];

    if (!opts) {
        fprintf(stderr, "lift: boot screen: null opts\n");
        return 1;
    }

    dump = opts->palette_dump;
    if (!dump || !*dump)
        dump = "build/lift/boot_copyright";

    fprintf(stderr,
            "lift: boot screen — cold boot, dump=%s%s%s\n",
            dump,
            opts->live_view ? " (live SDL — close window to exit)" : "",
            opts->skip_practice ? " (--practice desert / Delta MT)" : "");

    if (opts->live_view) {
        set_live_view_env(1);
        set_boot_env("I960_HOST_VIDEO_SYNC", "1");
        set_boot_env("I960_HOST_BOOT_FAST_COUNTDOWN",
                     opts->skip_practice ? "1" : "0");
        set_boot_env("I960_HOST_MAX_DISPATCH", "0");
        set_boot_env("I960_HOST_MILESTONE_BOOT", "");
        if (opts->record_path && *opts->record_path)
            set_boot_env("I960_HOST_RECORD", opts->record_path);
    } else {
        set_live_view_env(0);
        set_boot_env("I960_HOST_BOOT_FAST_COUNTDOWN", "1");
        snprintf(dispatch_limit, sizeof(dispatch_limit), "%u",
                 BOOT_HEADLESS_DISPATCH_LIMIT);
        set_boot_env("I960_HOST_MAX_DISPATCH", dispatch_limit);
        /* Preserve I960_HOST_MILESTONE_BOOT when set on the command line. */
        if (!getenv("I960_HOST_MILESTONE_BOOT"))
            set_boot_env("I960_HOST_MILESTONE_BOOT", "");
    }

    if (opts->skip_practice)
        set_boot_env("I960_HOST_SKIP_PRACTICE", "1");

    if (opts->aspect && *opts->aspect)
        set_boot_env("I960_HOST_ASPECT", opts->aspect);

    set_boot_env("I960_HOST_BOOT_SCREEN", "1");
    set_boot_env("I960_HOST_TRACE_QUIET", "1");

    if (dump && *dump) {
#if defined(_WIN32)
        {
            static char palette_env[512];
            snprintf(palette_env, sizeof(palette_env), "I960_PALETTE_DUMP=%s", dump);
            _putenv(palette_env);
        }
#else
        setenv("I960_PALETTE_DUMP", dump, 1);
#endif
    }

    i960_host_trace_init();

    /*
     * Headless geo decode: start the FIFO worker before boot so vsync frame
     * latches can decode carousel slices while words are still in the ring.
     */
    {
        const char *prg = getenv("I960_GEO_PRG_DECODE");
        /* Default on; set I960_GEO_PRG_DECODE=0 to skip. */
        if (!(prg && prg[0] == '0'))
            (void)model2_geo_init_from_lift();
    }

    /* Load battery-backed SRAM + COUNTRY/coin options before cold boot. */
    (void)model2_nvram_load(NULL);

    if (opts->live_view && sys24_viewer_open("segamod2 — boot screen") != 0)
        return 1;
    if (opts->live_view)
        i960_host_frame_present();

    boot_entry_host(0, 0, 0);
    i960_host_run_post_reset(0, 0, 0);

    if (dump && *dump)
        model2_palette_state_dump(dump);

    /* Summary while geo mesh is still alive (viewer shutdown frees it). */
    i960_host_trace_summary();

    sys24_viewer_shutdown();

    fprintf(stderr, "lift: boot screen ended\n");
    g0 = 0;
    return 0;
}
