/* Object XZ span test @ 0x22850 — used by game_start_race_obj_scan.
 *
 * If (g0>>16)==(g2>>16), submit delta(self,other) through TGP 0x2c805959
 * with thresholds from 0x214360/364/368; return 1 when readback > 0.
 * Else compare signed high halves: return 1 when (g2>>16) <= (g0>>16).
 *
 * g1/g3 are pointers to xyz floats (self / other).
 *
 * source: disasm/maincpu/maincpu_022850_f0.asm */
// @rom 0x22850 +0xe0 game_start_race_obj_span_test

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

#include <string.h>

/* g1/g3 are host frame pointers (lda 0x40/50(fp)); keep full uintptr_t —
 * casting through u32 truncates arm64 and SEGVs when hi16(+0x54) matches. */
void game_start_race_obj_span_test(u32 self_z, void *self_xyz, u32 other_z)
{
    uintptr_t fp_save = fp;
    u8 frame[0x60];
    u32 self_hi = self_z >> 16;
    u32 other_hi = other_z >> 16;
    u32 *self = (u32 *)self_xyz;
    u32 *other = (u32 *)g3;
    double dx, dy, dz;
    u32 hit;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;

    if (self_hi != other_hi) {
        /* @0x22918: shri 16 both; cmpible other, self → 1. */
        i32 a = (i32)other_z >> 16;
        i32 b = (i32)self_z >> 16;

        g0 = ((i32)a <= (i32)b) ? 1u : 0u;
        fp = fp_save;
        return;
    }

    if (self == NULL || other == NULL) {
        g0 = 0;
        fp = fp_save;
        return;
    }

    /* @0x22860–0x22880: delta = other − self on x/y/z. */
    dx = i960_u32_to_f64(other[0]) - i960_u32_to_f64(self[0]);
    dy = i960_u32_to_f64(other[1]) - i960_u32_to_f64(self[1]);
    dz = i960_u32_to_f64(other[2]) - i960_u32_to_f64(self[2]);

    /* TGP 0x2c805959 — firmware @ 0x530 interleaved 3-term dot. */
    i960_mmio_write_u32(0x884000u, 0x2c805959u);
    i960_mmio_write_u32(0x884000u, (u32)i960_f64_to_u32(dx));
    i960_mmio_write_u32(0x884000u, i960_ld_u32(I960_WORKRAM, 0x214360, 0));
    i960_mmio_write_u32(0x884000u, (u32)i960_f64_to_u32(dz));
    i960_mmio_write_u32(0x884000u, i960_ld_u32(I960_WORKRAM, 0x214364, 0));
    i960_mmio_write_u32(0x884000u, (u32)i960_f64_to_u32(dy));
    i960_mmio_write_u32(0x884000u, i960_ld_u32(I960_WORKRAM, 0x214368, 0));
    hit = i960_mmio_read_u32(0x884000u);

    if (i960_u32_to_f64(hit) > 0.0)
        g0 = 1u;
    else
        g0 = 0u;
    fp = fp_save;
}
