/* Palette irq fade stepper @ 0x33038 — advance scale toward target; clear mode. */
// @rom 0x33038 +0x5c geo_palette_fade_step

#include "i960_lift.h"
#include "i960_mem.h"

/*
 * ldq 0x213850 → mode, scale, target, step.
 * Disasm @ 0x33048–0x33054:
 *   scale==target → mode=0
 *   else step==0  → mode=0  (hold: after_catalogs countdown wash)
 *   else scale += step clamped toward target
 * Attract logo queues mode=4, scale=0x100, target=0, step=-8 (fade-in).
 * Keeping mode alive on step==0 left colorxlat stuck washed after practice
 * START (logo_path fade-in never promoted).
 */
void geo_palette_fade_step(u32 arg0, u32 arg1, u32 arg2)
{
    u32 mode;
    u32 scale;
    u32 target;
    i32 step;
    i32 next;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    mode = i960_ld_u32(I960_WORKRAM, 0x213850, 0);
    scale = i960_ld_u32(I960_WORKRAM, 0x213854, 0);
    target = i960_ld_u32(I960_WORKRAM, 0x213858, 0);
    step = (i32)i960_ld_u32(I960_WORKRAM, 0x21385c, 0);

    /* @0x33048 / @0x3304C fall-through: scale==target OR step==0 → mode=0. */
    if (scale == target || step == 0) {
        mode = 0u;
        /* @0x33088: stl g0 → mode+scale only (target/step unchanged). */
        i960_st_u32(I960_WORKRAM, 0x213850, 0, mode);
        i960_st_u32(I960_WORKRAM, 0x213854, 0, scale);
        return;
    }

    next = (i32)scale + step;
    if (step > 0) {
        /* @0x3305C–0x3306C: clamp up to target */
        if ((i32)target <= next)
            scale = target;
        else
            scale = (u32)next;
    } else {
        /* @0x33070–0x33080: clamp down to target */
        if ((i32)target >= next)
            scale = target;
        else
            scale = (u32)next;
    }

    i960_st_u32(I960_WORKRAM, 0x213850, 0, mode);
    i960_st_u32(I960_WORKRAM, 0x213854, 0, scale);
}
