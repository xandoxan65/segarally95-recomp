/* Desert race RNG jitter leaf @ 0x41BE0 — optional position nudge.
 *
 * Called from node_frame when gate in {2,3,6,7}. Walks 0x216a88 entries
 * (stride 0x1c via lda) while entry float > −1.5; on first empty slot,
 * stores XYZ from g0 and (15−scale)*k into the entry.
 *
 * xyz may be a guest VA (callx) or a host private-frame pointer (C caller).
 *
 * source: /tmp/dasm_41be0/maincpu_041be0_c0.asm */
// @rom 0x41be0 +0xc0 game_start_race_desert_rng_jitter

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_rom.h"

#include <string.h>

void game_start_race_desert_rng_jitter(u32 xyz, u32 scale_in, u32 arg2)
{
    u32 entry = 0x216a88u;
    u32 dest = entry - 24u; /* @0x41C04: subo 24,g6,g7 */
    u32 idx = 0;
    double lim15 = i960_rifl_read(0, 0x402e0000u); /* 15.0 */
    double neg15 = i960_rifl_read(0, 0xbff80000u); /* −1.5 */
    double k = i960_rifl_read(0x851eb852u, 0x3fb851ebu);
    double s = i960_u32_to_f64(scale_in);
    u8 *xyz_host = NULL;
    u32 x, y, z;

    (void)arg2;
    g14 = 0;

    /* Host private-frame XYZ when guest map misses (desert node_frame). */
    if (model2_ram_mut(xyz) == NULL && xyz != 0u)
        xyz_host = (u8 *)(uintptr_t)xyz;

    /* @0x41BE0: if scale >= 15, ret. */
    if (s >= lim15)
        return;

    for (;;) {
        double v = i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, entry, 0));

        if (v <= neg15)
            break;

        idx += 1u;
        /* cmpi idx,0x3f / ble continue — else fall off to ret @ 0x41C48. */
        if ((i32)idx > 0x3f)
            return;

        /* lda 0x1c(g6),g6 — EA advance, not a pointer load. */
        entry += 0x1cu;
        dest += 28u;
    }

    if (xyz_host != NULL) {
        memcpy(&x, xyz_host + 0, 4);
        memcpy(&y, xyz_host + 4, 4);
        memcpy(&z, xyz_host + 8, 4);
    } else {
        x = i960_ld_u32(I960_ABS, xyz, 0);
        y = i960_ld_u32(I960_ABS, xyz, 4);
        z = i960_ld_u32(I960_ABS, xyz, 8);
    }

    /* @0x41C4C–0x41C9C: (15 − scale) * k → store XYZ + scaled float. */
    {
        double scaled = (lim15 - s) * k;

        i960_st_u32(I960_WORKRAM, dest, 0, x);
        i960_st_u32(I960_WORKRAM, dest, 4, y);
        i960_st_u32(I960_WORKRAM, dest, 8, z);
        i960_st_u32(I960_WORKRAM, entry, 0, (u32)i960_f64_to_u32(scaled));
    }
}
