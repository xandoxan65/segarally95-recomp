/* Asset export — C path only; no host-script bridge. */

#include "track_viewer.h"

#include <stdio.h>

int track_viewer_export_assets(const track_viewer_opts_t *opts)
{
    (void)opts;
    /*
     * Mesh / palette_cache bake belongs in lifted C (catalog + TEX + palram),
     * not a Python module. Until that port exists, skip export and keep the
     * palette dump from the lifted IRQ / boot path.
     */
    fprintf(stderr, "lift: track viewer export skipped (C bake not lifted yet)\n");
    return 0;
}
