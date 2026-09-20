/* Road span step @ 0x311E0 — bal from road_attach 0x2CB90.
 *
 * Projects car XZ onto span_base[index*16], advances *(node+0xe4) when t≥1
 * (wrap via integer @ 0x5dce70[course*16]), stores lerped Y at node+0x94,
 * then calls road_span_basis @ 0x31320 with g3=&node+0x90.
 *
 * Uses the *caller's* fp for temps (lda 0x20(sp) only). g0=node, g1=span_base.
 *
 * source: disasm/maincpu/maincpu_0311e0_140.asm */
// @rom 0x311e0 +0x130 game_start_race_obj_road_span

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "i960_host.h"
#include "model2_rom.h"

#include "lift_syms.h"

#include <stdio.h>

void game_start_race_obj_road_span(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t sp_save = sp;
    u32 node = arg0;
    u32 span_base = arg1;
    u32 index;
    u32 row;
    u32 x0, z0, x1, z1;
    u32 cx, cz;
    u32 dx, dz;
    u32 rx, rz;
    u32 t;
    u32 max_n;
    u32 course;
    u32 y0, y1;
    u32 y;
    u32 basis_ix;
    u32 basis_t;

    (void)arg2;

    /* @0x311E0: lda 0x20(sp),sp — keep caller fp. */
    sp = sp + 0x20u;
    g14 = 0;

    if (node == 0u || span_base == 0u) {
        sp = sp_save;
        return;
    }

    index = i960_ld_u32(I960_ABS, node, 0xe4u);
    row = span_base + (index << 4);

    x0 = i960_ld_u32(I960_ABS, row, 0);
    x1 = i960_ld_u32(I960_ABS, row, 0x10u);
    z1 = i960_ld_u32(I960_ABS, row, 0x18u);
    z0 = i960_ld_u32(I960_ABS, row, 8u);
    cx = i960_ld_u32(I960_ABS, node, 0x14u);
    cz = i960_ld_u32(I960_ABS, node, 0x1cu);

    dx = (u32)i960_f64_to_u32(i960_u32_to_f64(x1) - i960_u32_to_f64(x0));
    dz = (u32)i960_f64_to_u32(i960_u32_to_f64(z1) - i960_u32_to_f64(z0));
    rx = (u32)i960_f64_to_u32(i960_u32_to_f64(cx) - i960_u32_to_f64(x0));
    rz = (u32)i960_f64_to_u32(i960_u32_to_f64(cz) - i960_u32_to_f64(z0));

    {
        u32 num = (u32)i960_f64_to_u32(
            i960_u32_to_f64(dx) * i960_u32_to_f64(rx)
            + i960_u32_to_f64(dz) * i960_u32_to_f64(rz));
        u32 den = (u32)i960_f64_to_u32(
            i960_u32_to_f64(dx) * i960_u32_to_f64(dx)
            + i960_u32_to_f64(dz) * i960_u32_to_f64(dz));

        t = (u32)i960_f64_to_u32(i960_u32_to_f64(num) / i960_u32_to_f64(den));
    }

    course = i960_host_race_course_index();
    max_n = i960_ld_u32(I960_WORKRAM, 0x5dce70u + (course << 4), 0);

    *(u32 *)(fp + 0x58u) = rz;
    *(u32 *)(fp + 0x50u) = rx;
    *(u32 *)(fp + 0x48u) = dz;
    *(u32 *)(fp + 0x40u) = dx;

    if (!(i960_u32_to_f64(t) >= 0.0))
        t = 0;
    if (i960_u32_to_f64(t) > 1.0) {
        /* cvtzri / cvtir @ 0x31270 — trunc toward zero. */
        i32 ti = (i32)i960_u32_to_f64(t);
        u32 tf = (u32)i960_f64_to_u32((double)ti);

        index = (u32)((i32)index + ti);
        t = (u32)i960_f64_to_u32(i960_u32_to_f64(t) - i960_u32_to_f64(tf));
        if ((i32)index >= (i32)max_n)
            index = (u32)((i32)index - (i32)max_n);
    }

    i960_st_u32(I960_ABS, node, 0xe4u, index);
    row = span_base + (index << 4);

    y1 = i960_ld_u32(I960_ABS, row, 0x14u);
    y0 = i960_ld_u32(I960_ABS, row, 4u);
    y = (u32)i960_f64_to_u32(
        (i960_u32_to_f64(y1) - i960_u32_to_f64(y0)) * i960_u32_to_f64(t)
        + i960_u32_to_f64(y0));
    i960_st_u32(I960_ABS, node, 0x94u, y);
    {
        static int logged;

        if (!logged) {
            fprintf(stderr,
                    "lift: road_span node=%#x idx=%u t=%.4g y=%.4g max=%u\n",
                    node, index, i960_u32_to_f64(t), i960_u32_to_f64(y),
                    max_n);
            fflush(stderr);
            logged = 1;
        }
    }

    /*
     * @0x312B0–0x312F4: basis parameter = t+0.5; if >1 then
     *   basis_t = (t+0.5)-1 (subrl +1.0,fp0 → src2-src1), basis_ix++.
     */
    {
        double tp = i960_u32_to_f64(t) + 0.5;

        basis_ix = i960_ld_u32(I960_ABS, node, 0xe4u);
        basis_t = (u32)i960_f64_to_u32(tp);
        if (tp > 1.0) {
            basis_t = (u32)i960_f64_to_u32(tp - 1.0);
            basis_ix = basis_ix + 1u;
            if ((i32)basis_ix >= (i32)max_n)
                basis_ix = 0;
        }
    }

    /* g3 = &node+0x90 (output); g2 = *(node+0xa8) from prior 0x2c. */
    g3 = node + 0x90u;
    g2 = i960_ld_u32(I960_ABS, node, 0xa8u);
    g0 = span_base + (basis_ix << 4);
    g1 = basis_t;
    game_start_race_obj_road_span_basis((u32)g0, (u32)g1, (u32)g2);
    sp = sp_save;
}
