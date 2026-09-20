/* Scene row print @ 0x5C50 — cursor seed + printf or script gate by row index.
 * source: disasm/maincpu/maincpu_005c50_64.asm */
// @rom 0x5c50 +0x64 geo_scene_row_print

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <string.h>

void geo_scene_row_print(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    u32 idx = arg0;
    u32 rows[4];
    u32 fmt;
    /*
     * Private frame — same host rule as string_prep / script_finish. Do not
     * fp=sp then I960_WORKRAM with a truncated host pointer.
     */
    u8 frame[0x60];

    (void)arg1;
    (void)arg2;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;

    rows[0] = 0x5a4bb0u;
    rows[1] = 0x5a4bd0u;
    rows[2] = 0x5a4bf0u;
    rows[3] = 0x5a4c30u;
    *(u32 *)(fp + 0x40) = rows[0];
    *(u32 *)(fp + 0x44) = rows[1];
    *(u32 *)(fp + 0x48) = rows[2];
    *(u32 *)(fp + 0x4c) = rows[3];

    tile_cursor_seed(17u, 31u + 13u);
    fmt = (idx < 4u) ? rows[idx] : 0;
    g0 = fmt;
    if (idx == 2u || idx == 3u) {
        /* Isolate printf from this frame's 0x40(fp) slots. */
        fp = 0;
        libc_printf((const char *)(uintptr_t)fmt, (u32)g1, (u32)g2);
        fp = (uintptr_t)frame;
    } else {
        boot_tile_script_gate((u32)g0, (u32)g1, (u32)g2);
    }

    fp = fp_save;
}
