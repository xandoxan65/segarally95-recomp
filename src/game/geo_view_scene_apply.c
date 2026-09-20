/* Scene view apply @ 0x39750 (callers: 0x1633C, 0x37EC0, …).
 * Composes FOV/pitch side channels into workram and 0x202049.
 * source: disasm/maincpu/maincpu_039750_340.asm */
// @rom 0x39750 +0x340 geo_view_scene_apply

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

#include "lift_syms.h"

static float f32(u32 bits)
{
    return (float)i960_u32_to_f64(bits);
}

static u32 bits32(float v)
{
    return (u32)i960_f64_to_u32((double)v);
}

void geo_view_scene_apply(u32 arg0, u32 arg1, u32 arg2)
{
    float angle;
    float fov;
    float r12;
    float sum_a;
    float sum_b;
    float g1f, g2f, g3f;
    float g7f;
    float table_v;
    float y;
    float acc;
    i32 idx;
    i32 code;
    i32 lim;
    i32 g6i;
    u32 slot;
    u32 ptr;
    u8 flag_01a;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* r12 = (0x214270 + 0.2) * 2; stash prior angle; bal fov_scale → g0. */
    fov = f32(i960_ld_u32(I960_WORKRAM, 0x214270, 0));
    r12 = (fov + 0.2f) * 2.f;
    i960_st_u32(I960_WORKRAM, 0x214294, 0, i960_ld_u32(I960_WORKRAM, 0x214290, 0));
    /* @0x39798: bal 0x395d8 — wheel analog replaces incoming g0. */
    g0 = geo_view_fov_scale(0, 0, 0);
    angle = f32((u32)g0);

    sum_a = f32(i960_ld_u32(I960_WORKRAM, 0x2142a0, 0))
          + f32(i960_ld_u32(I960_WORKRAM, 0x2142a4, 0))
          + f32(i960_ld_u32(I960_WORKRAM, 0x2142a8, 0))
          + f32(i960_ld_u32(I960_WORKRAM, 0x2142ac, 0));
    sum_b = f32(i960_ld_u32(I960_WORKRAM, 0x2142b0, 0))
          + f32(i960_ld_u32(I960_WORKRAM, 0x2142b4, 0))
          + f32(i960_ld_u32(I960_WORKRAM, 0x2142b8, 0))
          + f32(i960_ld_u32(I960_WORKRAM, 0x2142bc, 0));
    g3f = sum_a * 0.5f;
    g2f = sum_b * 0.5f;

    /* g1 = notbit31(25.0) * r12 */
    g1f = f32(i960_ld_u32(I960_WORKRAM, 0x214284, 0) ^ 0x80000000u) * r12;

    /* Table pointer @ 0x5D8230[idx] or fallback 0x5D832C; load float through ptr */
    {
        float mag = (angle < 0.f) ? -angle : angle;
        float scaled = mag * 63.f;

        idx = (i32)scaled;
        if (idx > 63)
            ptr = i960_ld_u32(I960_WORKRAM, 0x5d832c, 0);
        else
            ptr = i960_ld_u32(I960_WORKRAM, 0x5d8230, (u32)idx << 2);
        table_v = f32(i960_ld_u32(I960_ABS, ptr, 0));
        /* divrl vs 63.0: y = ±table/63 (sign follows angle) */
        y = table_v / 63.f;
        if (angle < 0.f)
            y = -y;
        g1f = g1f * y;
    }

    /* g7 = angle - 0x214294; g5 = notbit31(g7) */
    g7f = angle - f32(i960_ld_u32(I960_WORKRAM, 0x214294, 0));
    {
        float neg_d = f32(bits32(g7f) ^ 0x80000000u);
        float c120 = f32(i960_ld_u32(I960_WORKRAM, 0x21428c, 0));
        float c260 = f32(i960_ld_u32(I960_WORKRAM, 0x214288, 0));
        float t0 = g2f * c120;
        float t1 = neg_d * c260;
        float t2 = g3f - t0;

        g1f = g1f + t1 * r12 - t2 * r12;
    }

    acc = (f32(i960_ld_u32(I960_WORKRAM, 0x214280, 0)) + g1f) * 0.5f;

    slot = i960_ld_u32(I960_WORKRAM, 0x202008, 0) & 3u;
    i960_st_u32(I960_WORKRAM, 0x2142a0, slot << 2, bits32(g1f));

    lim = (i32)i960_ld_u32(I960_WORKRAM, 0x214274, 0);
    code = (i32)((acc / 20.f) * (float)lim);

    i960_st_u32(I960_WORKRAM, 0x214290, 0, arg0);
    slot = i960_ld_u32(I960_WORKRAM, 0x202008, 0) & 3u;
    i960_st_u32(I960_WORKRAM, 0x2142b0, slot << 2, bits32(g7f));
    i960_st_u32(I960_WORKRAM, 0x214280, 0, bits32(acc));

    g6i = (i32)i960_ld_u32(I960_WORKRAM, 0x21427c, 0);
    if (g6i == 0)
        g6i = code;
    if (g6i < code) {
        g6i = code;
        code = code + 1;
    } else if (g6i > code) {
        g6i = code;
        code = code - 1;
    }
    i960_st_u32(I960_WORKRAM, 0x21427c, 0, (u32)g6i);

    if (code > lim)
        code = lim;
    if (code < -lim)
        code = -lim;

    flag_01a = (u8)i960_ld_u8(I960_WORKRAM, 0x20201a, 0);
    if (code == 0) {
        if (flag_01a == 1u) {
            i960_st_u8(I960_WORKRAM, 0x202049, 0, 0);
            return;
        }
        if ((i960_ld_u32(I960_WORKRAM, 0x202008, 0) & 3u) == 0u)
            code = 1;
        else
            code = -1;
    }

    /* cmpibge 0,g4,0x39a54 — branch if g4 >= 0 */
    if (code >= 0) {
        i32 mag = code;

        if (mag < 0)
            mag = -mag;
        code = mag - 0x41;
    } else {
        code = code + 0x7f;
    }

    i960_st_u8(I960_WORKRAM, 0x202049, 0, (u8)code);
    if (flag_01a == 1u && i960_ld_u32(I960_WORKRAM, 0x202098, 0) != 4u)
        i960_st_u8(I960_WORKRAM, 0x202049, 0, 0);
}
