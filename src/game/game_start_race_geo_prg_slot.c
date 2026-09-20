/* Race GEO PRG slot @ 0x21370 — cam_boot installs a staged handler EA into
 * 0x20ab50 (lda 0x5c0330 → ROM 0x21330); race_frame calls with g0=0 to
 * invoke it, then pushes the 0x202270/274 pair into the GEO PRG FIFO and
 * bumps 0x20b0b8 (MAME geo_prg_w when geoctl bit31 clear → push_geo_data).
 *
 * source: disasm/maincpu/maincpu_021370_100.asm
 * MAME ref: model2_state::geo_prg_w @ third_party/mame/.../model2.cpp */
// @rom 0x21370 +0xbc game_start_race_geo_prg_slot

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_memory.h"

#include "i960_host_staging.h"
#include "lift_syms.h"

#include <stdio.h>

void game_start_race_geo_prg_slot(u32 arg0, u32 arg1, u32 arg2)
{
    u32 handler;
    u32 pair_lo;
    u32 pair_hi;
    u32 ctr;
    u8 flags;
    static int logged;

    (void)arg1;
    (void)arg2;

    /* @0x21374: g0 != 0 → install only. */
    if (arg0 != 0u) {
        i960_st_u32(I960_WORKRAM, 0x20ab50, 0, arg0);
        if (!logged) {
            fprintf(stderr, "lift: race_geo_prg_slot install=%#x\n", arg0);
            fflush(stderr);
            logged = 1;
        }
        return;
    }

    /* @0x21384–0x213A4: snapshot 0x202270/274 across the callx (0x40(fp)). */
    pair_lo = i960_ld_u32(I960_WORKRAM, 0x202270, 0);
    pair_hi = i960_ld_u32(I960_WORKRAM, 0x202274, 0);
    handler = i960_ld_u32(I960_WORKRAM, 0x20ab50, 0);

    if (handler != 0u) {
        /* lda 0x20ab50 → g0; callx (handler). Staging resolves 0x5c0330→0x21330. */
        g0 = 0x0020ab50u;
        (void)i960_host_staging_call_lifted(handler);
    }

    /* Restore pre-call pair into 0x202270/274, then push that pair to PRG. */
    i960_st_u32(I960_WORKRAM, 0x202270, 0, pair_lo);
    i960_st_u32(I960_WORKRAM, 0x202274, 0, pair_hi);

    ctr = i960_ld_u32(I960_WORKRAM, 0x20b0b8, 0);

    /* st g14 → 0x800090; stl pair → GEO PRG FIFO @ 0x804000. */
    i960_mmio_write_u32(0x800090u, 0);
    i960_mmio_write_u32(GEO_PRG_FIFO, pair_lo);
    i960_mmio_write_u32(GEO_PRG_FIFO, pair_hi);
    i960_st_u32(I960_WORKRAM, 0x20b0b8, 0, ctr + 1u);

    /* clrbit 5 of 0x20204c; or 0x20 back if old ctr had bit5 (addo 31,1 = 32). */
    flags = (u8)i960_ld_u8(I960_WORKRAM, 0x20204c, 0);
    flags = (u8)(flags & (u8)~(1u << 5));
    i960_st_u8(I960_WORKRAM, 0x20204c, 0, flags);
    if ((ctr & (1u << 5)) != 0u) {
        flags = (u8)i960_ld_u8(I960_WORKRAM, 0x20204c, 0);
        i960_st_u8(I960_WORKRAM, 0x20204c, 0, (u8)(flags | 0x20u));
    }

    if (logged == 1) {
        fprintf(stderr, "lift: race_geo_prg_slot invoke handler=%#x\n", handler);
        fflush(stderr);
        logged = 2;
    }
}
