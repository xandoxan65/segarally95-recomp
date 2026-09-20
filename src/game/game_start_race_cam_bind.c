/* Race camera slot bind @ 0x20310 — install or invoke handler at 0x20ab4c.
 *
 * Called every game_start_race_frame with g0=0 after TGP 0x23/0x25. The
 * installed pointer is a staged workram PC (desert 0x5BEA20 → ROM 0x1FA20;
 * chase 0x5BEDC0 → 0x1FDC0; sentinel 0x5BE1D0 → seed @ 0x1F1D0).
 *
 * Install (g0 != 0): st g0 → 0x20ab4c. Seed @ 0x1F1D0 runs ONLY when
 * installing the sentinel (cmpibne g0, 0x5BE1D0 → skip). Desert/chase
 * installs must not re-seed — chase cam_rebind installs every frame, and
 * seed_tail's t-based yaw was causing continuous slow pans instead of the
 * out-window chase view.
 *
 * Invoke (g0 == 0): optional bit7(0x202064) toggle; callx *(0x20ab4c).
 *
 * source: disasm/maincpu/maincpu_020310_80.asm */
// @rom 0x20310 +0x74 game_start_race_cam_bind

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_mem.h"
#include "i960_host_staging.h"
#include "model2_rom.h"

#include "lift_syms.h"

#include <stdio.h>

#define CAM_SLOT       0x0020ab4cu
#define CAM_SENTINEL   0x005be1d0u

void game_start_race_cam_bind(u32 arg0, u32 arg1, u32 arg2)
{
    u32 handler = arg0;
    static int logged_install;
    static int logged_invoke;

    (void)arg1;
    (void)arg2;

    if (handler != 0u) {
        u32 prev = i960_ld_u32(I960_WORKRAM, CAM_SLOT, 0);

        i960_st_u32(I960_WORKRAM, CAM_SLOT, 0, handler);
        if (!logged_install || prev != handler) {
            lift_log(
                    "lift: race_cam_bind install handler=0x%x (rom 0x%x)%s\n",
                    (unsigned)handler,
                    (unsigned)i960_host_resolve_call_target(handler),
                    prev != handler && logged_install ? " (rebind)" : "");
            fflush(stderr);
            logged_install = 1;
        }
        /* @0x20324: cmpibne g0, sentinel → ret; seed only for sentinel. */
        if (handler == CAM_SENTINEL) {
            g0 = CAM_SLOT;
            game_start_race_cam_seed((u32)g0, 0, 0);
        }
        return;
    }

    /* Invoke path (race_frame): bit7 of 0x202064 can clear object_pen gate. */
    {
        u8 flags = i960_ld_u8(I960_WORKRAM, 0x202064, 0);

        if ((flags & 0x80u) != 0u) {
            u32 phase = i960_ld_u32(I960_WORKRAM, 0x2140c0, 0);

            phase = (phase + 1u) & 1u;
            i960_st_u32(I960_WORKRAM, 0x2140c0, 0, phase);
            if (phase != 0u)
                i960_st_u32(I960_WORKRAM, 0x2139d8, 0, (u32)g14);
        }
    }

    handler = i960_ld_u32(I960_WORKRAM, CAM_SLOT, 0);
    if (handler == 0u)
        return;

    if (!logged_invoke) {
        lift_log(
                "lift: race_cam_bind invoke handler=0x%x (rom 0x%x)\n",
                (unsigned)handler,
                (unsigned)i960_host_resolve_call_target(handler));
        fflush(stderr);
        logged_invoke = 1;
    }

    g0 = CAM_SLOT;
    if (!i960_host_staging_call_lifted(handler))
        i960_call_indirect(handler);
}
