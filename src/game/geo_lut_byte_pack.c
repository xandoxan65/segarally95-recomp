/* CRX LUT byte pack @ 0x37A8 (geo_crx_lut_upload inner bal). */
// @rom 0x37a8 +0x48 geo_lut_byte_pack

#include "i960_lift.h"

#include <math.h>

u32 geo_lut_byte_pack(u32 index)
{
    double v;
    u32 out;

    g0 = g14;
    g14 = 0;
    if (index == 0) {
        g0 = 0x80u;
        return (u32)g0;
    }
    v = log2((double)index) + 128.0;
    out = (u32)(int)v;
    if ((i32)out <= 0)
        out = 0;
    if (out > 128u)
        out = 128u;
    g0 = out | 0x80u;
    return (u32)g0;
}
