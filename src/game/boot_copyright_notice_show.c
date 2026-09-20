/* Copyright / region-warning tile scripts @ 0x1A990 (game_inner_dispatch submode 1). */
// @rom 0x1a990 +0x80 boot_copyright_notice_show

#include "i960_lift.h"
#include "lift_syms.h"

static const u32 s_copyright_scripts[] = {
    0x005b97e0u,
    0x005b9810u,
    0x005b9840u,
    0x005b9870u,
    0x005b98a0u,
    0x005b98d0u,
    0x005b9900u,
    0x005b9930u,
    0x005b9960u,
};

void boot_copyright_notice_show(u32 arg0, u32 arg1, u32 arg2)
{
    unsigned i;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x1A990 mov 11,g0; mov 12,g1; bal 0x26e18 */
    tile_cursor_seed(11, 12);

    for (i = 0; i < sizeof(s_copyright_scripts) / sizeof(s_copyright_scripts[0]); i++)
        boot_tile_script_run(s_copyright_scripts[i]);
}
