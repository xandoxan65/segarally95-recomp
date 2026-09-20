/* Offline prg_fifo → C geo DL decode (golden-compare entry). */

#include "track_viewer.h"
#include "lift_log.h"
#include "model2_geo.h"

#include <stdio.h>

int i960_lift_geo_fifo_decode_run(const track_viewer_opts_t *opts)
{
    const char *summary;
    int verts;

    if (!opts || !opts->geo_fifo_path)
        return 1;

    summary = opts->geo_summary;
    if (!summary || !*summary)
        summary = "build/lift/geo_decode_summary.json";

    lift_log( "lift: decode-geo-fifo %s → %s\n", opts->geo_fifo_path, summary);
    (void)model2_geo_init(NULL);
    verts = model2_geo_decode_file(opts->geo_fifo_path, 1);
    if (verts < 0) {
        /* Soft retry without requiring geo_end (truncated dumps). */
        verts = model2_geo_decode_file(opts->geo_fifo_path, 0);
    }
    if (verts < 0) {
        fprintf(stderr, "lift: geo FIFO decode failed\n");
        return 1;
    }
    if (model2_geo_dump_summary(summary) != 0) {
        lift_log( "lift: geo summary write failed\n");
        return 1;
    }
    lift_log( "lift: geo FIFO decode ok verts=%d tris=%u\n", verts,
            model2_geo_triangle_count());
    model2_geo_shutdown();
    return 0;
}
