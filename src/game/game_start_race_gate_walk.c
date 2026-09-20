/* Race start-gate walker @ 0x227B0 — callx 0x20afa0 for each cam slot.
 * Walks parallel lists at 0x2140d0 / 0x213980; callx gate handler with
 * g0=*cam_slot, g1=*obj_slot. Sets sticky hit flag; bumps 0x20afa4;
 * on hit && afa4>=30: texture apply + clear afa4 (g14=0).
 * source: disasm/maincpu/maincpu_0227b0_a0.asm */
// @rom 0x227b0 +0x9c game_start_race_gate_walk

#include "i960_lift.h"
#include "i960_mem.h"
#include "i960_host_staging.h"

#include "lift_syms.h"

void game_start_race_gate_walk(u32 arg0, u32 arg1, u32 arg2)
{
    u32 head;
    u32 cam_ea;
    u32 obj_ea;
    u32 hit;
    u32 handler;
    u32 afa4;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    head = i960_ld_u32(I960_WORKRAM, 0x2140d0, 0);
    hit = 0;
    cam_ea = 0x2140d0u;
    obj_ea = 0x213980u;
    if (head == 0u)
        goto done_walk;

    do {
        g0 = i960_ld_u32(I960_WORKRAM, cam_ea, 0);
        g1 = i960_ld_u32(I960_WORKRAM, obj_ea, 0);
        handler = i960_ld_u32(I960_WORKRAM, 0x20afa0, 0);
        if (handler != 0u) {
            if (!i960_host_staging_call_lifted(handler))
                i960_call_indirect(handler);
        }
        if ((u32)g0 != 0u) {
            u32 cam_val = i960_ld_u32(I960_WORKRAM, cam_ea, 0);
            u32 cam_ref = i960_ld_u32(I960_WORKRAM, 0x2140cc, 0);

            if (cam_val == cam_ref) {
                hit = 1u;
            } else {
                u32 obj0 = i960_ld_u32(I960_WORKRAM, 0x213980, 0);

                if ((u32)g0 == obj0)
                    hit = 1u;
            }
        }
        cam_ea += 4u;
        obj_ea += 4u;
        head = i960_ld_u32(I960_WORKRAM, cam_ea, 0);
    } while (head != 0u);

done_walk:
    afa4 = i960_ld_u32(I960_WORKRAM, 0x20afa4, 0) + 1u;
    i960_st_u32(I960_WORKRAM, 0x20afa4, 0, afa4);
    /* @0x22830: be if hit==0; else if afa4 < 30 skip cue. */
    if (hit == 0u)
        return;
    if ((i32)afa4 < 30)
        return;
    g0 = 0x48u;
    g1 = 0x7fu;
    tile_texture_descriptor_apply(0x48u, 0x7fu, 0);
    g14 = 0;
    i960_st_u32(I960_WORKRAM, 0x20afa4, 0, 0u);
}
