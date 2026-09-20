/* FOV scale from 0x202050 @ 0x395D8 (bal helper for view matrix seed). */
// @rom 0x395d8 +0x90 geo_view_fov_scale

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

#include <math.h>

u32 geo_view_fov_scale(u32 arg0, u32 arg1, u32 arg2)
{
    i32 raw;
    u32 mag;
    double scaled;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* bal return: save link in g0 then clear g14 (disasm @ 0x395D0). */
    g0 = g14;
    g14 = 0;

    /*
     * @0x395E0–0x395F0: ldob 0x202050; and 0xff; lda 0xffffff80(g4)
     * → signed (byte − 0x80). Neutral steer is 0x80 → 0.
     * Sign-extending the byte treated center as −128 and drove continuous
     * pitch_smooth / scene_apply deflection with hands off the wheel.
     */
    {
        u8 byte = (u8)i960_ld_u8(I960_WORKRAM, 0x202050, 0);

        raw = (i32)byte - 0x80;
    }
    mag = (raw < 0) ? (u32)(-raw) : (u32)raw;
    if (mag > 0x50u) /* lda 0x50 — clamp */
        mag = 0x50u;
    fp0 = (double)(i32)mag;
    fp1 = i960_rifl_read(0xae147ae1u, 0x3feae147u);
    fp0 = fp0 * fp1;
    fp1 = i960_rifl_read(0, 0x40540000u); /* 80.0 */
    fp0 = fp0 / fp1;
    scaled = fp0;
    if (raw < 0)
        scaled = -scaled;
    g0 = i960_f64_to_u32(scaled);
    return (u32)g0;
}
