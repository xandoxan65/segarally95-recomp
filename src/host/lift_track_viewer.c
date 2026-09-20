/* Unified track viewer: lifted palette init + palette dump + asset export. */

#include "track_viewer.h"
#include "i960_host.h"
#include "i960_host_scene.h"
#include "i960_lift.h"
#include "model2_hw.h"
#include "model2_geo.h"
#include "model2_snd.h"
#include "i960_mem.h"
#include "model2_memory.h"
#include "model2_rom.h"
#include "placement_catalog_feed.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void geo_renderer_init(u32 arg0, u32 arg1, u32 arg2);
extern void geo_draw_frame_entry(u32 arg0, u32 arg1, u32 arg2);

int track_viewer_export_assets(const track_viewer_opts_t *opts);

static void seed_track_placement_env(void)
{
    /* geo_draw_frame_entry reads these when --geo-frames > 0. */
#if defined(_WIN32)
    _putenv("I960_HOST_PLACEMENT_SOURCE=track");
    _putenv("I960_HOST_PLACEMENT_FEED=1");
#else
    setenv("I960_HOST_PLACEMENT_SOURCE", "track", 1);
    setenv("I960_HOST_PLACEMENT_FEED", "1", 1);
#endif
    i960_st_u32(I960_WORKRAM, 0x20a2ec, 0, PLACEMENT_TRACK_DESERT_START);
    i960_st_u32(I960_WORKRAM, PLACEMENT_CURSOR, 0, PLACEMENT_TRACK_DESERT_START);
}

int i960_lift_track_viewer_run(const track_viewer_opts_t *opts)
{
    const char *final_dump;
    int i;
    int rc = 0;

    if (!opts) {
        fprintf(stderr, "lift: track viewer: null opts\n");
        return 1;
    }

    fprintf(stderr,
            "lift: track viewer course=%s out=%s dump=%s%s\n",
            opts->course ? opts->course : "?",
            opts->out_root ? opts->out_root : "?",
            opts->palette_dump ? opts->palette_dump : "?",
            opts->palette_only ? " (palette-only)" : "");

    final_dump = opts->palette_dump;

    /* Lifted palette upload + lumaram init @ geo_renderer_init (boot table). */
    geo_renderer_init(0, 0, 0);

    /* Optional geo draw frames through lifted geo_draw_frame_entry. */
    if (opts->geo_frames > 0) {
        const char *dump;
        const char *cdump;
        const char *summary;

        i960_host_seed_scene_draw();
        seed_track_placement_env();
        for (i = 0; i < opts->geo_frames; i++)
            geo_draw_frame_entry(0, 0, 0);

        dump = getenv("I960_GEO_DUMP");
        cdump = getenv("I960_COPRO_DUMP");
        summary = getenv("I960_GEO_SUMMARY");
        if (dump && *dump)
            model2_hw_dump(dump);
        if (cdump && *cdump)
            model2_hw_dump_copro(cdump);
        if (summary && *summary) {
            model2_geo_decode();
            model2_geo_dump_summary(summary);
            fprintf(stderr, "lift: track geo mesh verts=%u tris=%u prg=%u copro=%u\n",
                    model2_geo_vertex_count(),
                    model2_geo_triangle_count(),
                    model2_hw_prg_total(),
                    model2_hw_copro_total());
        }
        {
            const char *sdump = getenv("I960_SND_DUMP");

            if (sdump && *sdump) {
                model2_snd_dump(sdump);
                fprintf(stderr, "lift: sound dump %s (%u bytes)\n",
                        sdump, model2_snd_total());
            }
        }
    }

    if (final_dump && *final_dump)
        model2_palette_state_dump(final_dump);

    if (!opts->palette_only) {
        if (track_viewer_export_assets(opts) != 0)
            rc = 1;
    }

    g0 = (u32)rc;
    fprintf(stderr, "lift: track viewer done (rc=%d)\n", rc);
    return rc;
}
