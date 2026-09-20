/* Mode-2 dispatch @ 0xFA00 (workram table slot 0x5AEA00).
 * Comm-board attract slot scan + inner-mode handler on 0x20209C. */
// @rom 0xfa00 +0x2b0 comm_attract_board_dispatch

#include "i960_lift.h"
#include "i960_mem.h"
#include "i960_host_staging.h"
#include "model2_hw.h"
#include "lift_syms.h"
#include "model2_rom.h"

#include <stdio.h>

#define COMM_INNER_JUMP_TABLE  0x005AE480u

static u32 comm_inner_handler_for_index(u32 index)
{
    u32 handler;

    index &= 15u;
    handler = i960_ld_u32(I960_WORKRAM, COMM_INNER_JUMP_TABLE, index << 2);
    if (handler == 0)
        handler = model2_workram_mirror_u32(COMM_INNER_JUMP_TABLE + (index << 2));
    return handler;
}

static void comm_attract_scene_hook(void)
{
    u32 scene_hook;

    /* @0xFD9C–0xFDA8: callx (20a78c) after inner handler when non-zero. */
    scene_hook = i960_ld_u32(I960_WORKRAM, 0x20a78c, 0);
    if (scene_hook == 0)
        return;
    g0 = scene_hook;
    if (!i960_host_staging_call_lifted(scene_hook))
        i960_call_indirect(scene_hook);
}

static void comm_list_walk_type5(void)
{
    u32 slots;
    u32 link;
    u32 end;

    slots = i960_ld_u8(I960_WORKRAM, 0x20a581, 0);
    if (slots == 0)
        return;

    link = 0x1a121c2u;
    end = link + (((slots << 3) - slots) << 6);

    while (link < end) {
        if ((i960_ld_u16(I960_ABS, link, 0) & 0xffu) == 5u)
            return;
        link = i960_ld_u32(I960_ABS, link, 0x1c0);
        if (link == 0 || link >= end)
            return;
    }
}

/* @0xFCD8–0xFDAC inner-mode tail: wait gate @ 0x20A784 then call 0x5AE480[inner&15]. */
static void comm_attract_inner_tail(void)
{
    u32 board_type;
    u32 inner;
    u32 wait_key;
    u32 wait;
    u32 handler;
    u32 frame;

    board_type = i960_ld_u32(I960_WORKRAM, 0x20a530, 0);
    inner = i960_ld_u32(I960_WORKRAM, 0x20209c, 0);
    wait_key = inner;

    if (board_type != 3u && board_type != 0u && inner >= 10u
        && i960_ld_u32(I960_WORKRAM, 0x20a550, 0) == 2u)
        wait_key = 11u;

    wait = i960_ld_u32(I960_WORKRAM, 0x20a784, 0);
    if (wait_key != wait)
        i960_st_u32(I960_WORKRAM, 0x20a788, 0, 1u);
    else
        /*
         * @0xFD48: st g14,0x20a788 — guest link is 0 after leaf ``mov 0,g14``.
         * Host g14 is not that link; storing it left the splash gate sticky and
         * re-ran splash bind (inners 2/4/6) between every animate scene.
         */
        i960_st_u32(I960_WORKRAM, 0x20a788, 0, 0u);

    /* @0xFD50: jump table index is wait_key (may be forced to 11), not raw inner. */
    handler = comm_inner_handler_for_index(wait_key);
    i960_st_u32(I960_WORKRAM, 0x20a784, 0, wait_key);
    i960_st_u32(I960_WORKRAM, 0x20209c, 0, wait_key);

    if (handler != 0) {
        if (!i960_host_staging_call_lifted(handler))
            i960_call_indirect(handler);
    } else {
        i960_st_u32(I960_WORKRAM, 0x20209c, 0, (u32)g14);
    }

    frame = i960_ld_u32(I960_WORKRAM, 0x20a808, 0);
    i960_st_u32(I960_WORKRAM, 0x20a808, 0, frame + 1u);

    {
        static u32 s_last_inner = 0xffffffffu;
        if (wait_key != s_last_inner) {
            fprintf(stderr,
                    "lift: attract inner %u → %u (script=%u copro=%u)\n",
                    (unsigned)s_last_inner, (unsigned)wait_key,
                    (unsigned)i960_ld_u32(I960_WORKRAM, 0x20a7c4, 0),
                    model2_hw_copro_count());
            s_last_inner = wait_key;
        }
    }

    comm_attract_scene_hook();
    comm_attract_board_frame(0, 0, 0);
}

void comm_attract_board_dispatch(u32 arg0, u32 arg1, u32 arg2)
{
    u32 inner;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    i960_st_u32(I960_WORKRAM, 0x20a7d0, 0, (u32)g14);
    comm_list_walk_type5();

    /* @0xFA3C: inner < 10 → @0xFB90; inner == 9 does phase scan then may FCD8.
     * Host: phase_setup for 9, then always run inner_tail (jump table + HUD hook).
     * inner >= 10: ROM slot-scan then FCD8; host still runs inner_tail so CREDIT
     * slots 11/12 remain reachable when 20a550 forces wait_key 11. */
    inner = i960_ld_u32(I960_WORKRAM, 0x20209c, 0);
    if (inner == 9u) {
        u32 slots = i960_ld_u8(I960_WORKRAM, 0x20a581, 0);
        if (slots != 0)
            comm_attract_phase_setup(0, 0, 0);
    }
    comm_attract_inner_tail();
}
