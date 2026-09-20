/* Sin mix @ 0x38A18: g0 + sin(g0 * pi) * scale (scale from 0x5D6688/8C).
 * Copro marker 0x0A801515 (host sin). bal from geo_view_frame_update.
 * source: disasm/maincpu/maincpu_038a18_b4.asm */
// @rom 0x38a18 +0xb4 geo_view_sin_mix

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

u32 geo_view_sin_mix(u32 arg0, u32 arg1, u32 arg2)
{
    float x;
    float ang;
    float scale;
    float s;
    u32 scale_addr;

    (void)arg1;
    (void)arg2;

    x = (float)i960_u32_to_f64(arg0);
    ang = x * (float)M_PI; /* lda 0x400921FB / 0x54442D18 */

    i960_mmio_write_u32(0x884000, 0x0a801515u);
    i960_mmio_write_u32(0x884000, (u32)i960_f64_to_u32((double)ang));

    scale_addr = (x > 0.f) ? 0x5d6688u : 0x5d668cu;
    scale = (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, scale_addr, 0));
    s = (float)i960_u32_to_f64(i960_mmio_read_u32(0x884000));
    g0 = (u32)i960_f64_to_u32((double)(x + s * scale));
    return (u32)g0;
}
