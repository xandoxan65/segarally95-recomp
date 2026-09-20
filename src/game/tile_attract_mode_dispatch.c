/* Mode-1 attract tile dispatch @ 0xCC70 (workram 0x5ABC70, table @ 0x5ABC50). */
// @rom 0xcc70 +0x50 tile_attract_mode_dispatch

#include "i960_lift.h"
#include "i960_mem.h"
#include "i960_host_staging.h"
#include "i960_host.h"
#include "model2_rom.h"

#define TILE_ATTRACT_JUMP_TABLE  0x005ABC50u

static u32 tile_attract_handler_for_index(u32 index)
{
    u32 handler;

    if (index >= 6u)
        return 0;
    handler = i960_ld_u32(I960_WORKRAM, TILE_ATTRACT_JUMP_TABLE, index << 2);
    if (handler == 0)
        handler = model2_workram_mirror_u32(TILE_ATTRACT_JUMP_TABLE + (index << 2));
    return handler;
}

void tile_attract_mode_dispatch(u32 arg0, u32 arg1, u32 arg2)
{
    u32 index;
    u32 handler;
    u32 main_mode;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0xCC70: ld 20209c & 0x87; ld 5abc50[index]; callx or fall through @0xCC90. */
    index = i960_ld_u32(I960_WORKRAM, 0x20209c, 0) & 0x87u;
    handler = tile_attract_handler_for_index(index);
    if (handler != 0) {
        if (!i960_host_staging_call_lifted(handler))
            i960_call_indirect(handler);
        return;
    }

    /* @0xCC90: no handler — bump main mode and clear phase index. */
    main_mode = i960_ld_u32(I960_WORKRAM, 0x202098, 0);
    i960_st_u8(I960_WORKRAM, 0x20a580, 0, (u8)g14);
    i960_st_u32(I960_WORKRAM, 0x20209c, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x202098, 0, main_mode + 1);
}
