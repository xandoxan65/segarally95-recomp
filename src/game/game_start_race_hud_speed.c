/* Race HUD speed digits @ 0x1DF40 — hud_frame passes g0=1.
 *
 * @0x1DF64: cmpibge 0,timer,0x1e0c0 — Intel: timer <= 0 → |0x20b0b4 × 216|.
 * timer > 0 (after GO) divides prod by TGP 0x11(k/0x214124).
 * Digits into batch 0x20ac88; leading zeros blanked via boot_tile_map_fill.
 *
 * source: disasm/maincpu/maincpu_01df40_290.asm */
// @rom 0x1df40 +0x28c game_start_race_hud_speed

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>

static u32 f32_bits(double v)
{
    return (u32)i960_f64_to_u32(v);
}

void game_start_race_hud_speed(u32 arg0, u32 arg1, u32 arg2)
{
    i32 speed;
    double scaled;
    u32 batch;
    u32 hundreds;
    u32 tens;
    u32 ones;
    u32 had_hundreds;
    static int logged;

    (void)arg1;
    (void)arg2;

    if (!logged) {
        lift_log( "lift: race_hud_speed g0=%u\n", (unsigned)arg0);
        fflush(stderr);
        logged = 1;
    }

    if (arg0 == 0u) {
        g0 = 20;
        g1 = 31u + 12u;
        g2 = i960_ld_u32(I960_WORKRAM, 0x20ac84, 0);
        g3 = 0;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
        return;
    }

    if ((i32)i960_ld_u32(I960_WORKRAM, 0x20a560, 0) <= 0) {
        /* @0x1E0C0: |0x20b0b4 × 216| (countdown / timer==0). */
        scaled = i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x20b0b4, 0));
        scaled *= i960_rifl_read(0, 0x406b0000u);
        if (scaled < 0.0)
            scaled = -scaled;
        speed = (i32)scaled;
    } else {
        /* @0x1DF68: ratio = k / 0x214124; prod = 0x20b0b4 × 216; quot via TGP 0x11. */
        double inv = i960_rifl_read(0x78722053u, 0x3f66f604u);
        double ang = i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x214124, 0));
        double ratio = inv / ang;
        double span = i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x20b0b4, 0));
        double prod = span * i960_rifl_read(0, 0x406b0000u);
        u32 inv_bits;
        double denom;

        i960_mmio_write_u32(0x884000, 0x08801111u);
        i960_mmio_write_u32(0x884000, f32_bits(ratio));
        inv_bits = i960_ld_u32(I960_MMIO, 0x884000, 0);
        denom = i960_u32_to_f64(inv_bits);
        scaled = prod / denom;
        if (scaled < 0.0) {
            i960_mmio_write_u32(0x884000, 0x08801111u);
            i960_mmio_write_u32(0x884000, f32_bits(ratio));
            inv_bits = i960_ld_u32(I960_MMIO, 0x884000, 0);
            denom = i960_u32_to_f64(inv_bits);
            scaled = -(prod / denom);
        }
        speed = (i32)scaled;
    }

    if (speed < 0)
        speed = -speed;

    hundreds = (u32)(speed / 100);
    ones = (u32)(speed % 10);
    tens = (u32)((speed / 10) % 10);
    batch = i960_ld_u32(I960_WORKRAM, 0x20ac88, 0);

    if (hundreds != 0u) {
        g0 = 14;
        g1 = 31u + 12u;
        g2 = batch;
        g3 = hundreds % 10u;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
        had_hundreds = 1u;
    } else {
        boot_tile_map_fill(14u, 31u + 12u, 2u, 3u);
        had_hundreds = 0u;
    }

    if (had_hundreds != 0u || tens != 0u) {
        g0 = 16;
        g1 = 31u + 12u;
        g2 = batch;
        g3 = tens;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
    } else {
        boot_tile_map_fill(16u, 31u + 12u, 2u, 3u);
    }

    g0 = 18;
    g1 = 31u + 12u;
    g2 = batch;
    g3 = ones;
    g4 = 0;
    draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
}
