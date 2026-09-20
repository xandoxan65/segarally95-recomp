/* Race countdown PRG handler @ 0x211F0 — installed by slot2 phase 2 as
 * 0x5c01f0 into 0x20ab50. geo_prg_slot invokes this each race_frame.
 *
 * Calls hud_frame, pushes GEO window/PRG words for the start-light object,
 * steps 0x20ab54 via 0x3F9A0, and when ab54 > 95 switches PRG to 0x5c0340.
 *
 * source: disasm/maincpu/maincpu_0211f0_140.asm */
// @rom 0x211f0 +0x138 game_start_race_countdown_prg

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_memory.h"

#include "lift_syms.h"

#include <stdio.h>

void game_start_race_countdown_prg(u32 arg0, u32 arg1, u32 arg2)
{
    u32 ab54;
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    if (!logged) {
        lift_log( "lift: race_countdown_prg\n");
        fflush(stderr);
        logged = 1;
    }

    /* @0x211F0: per-frame HUD/matrix. */
    game_start_race_hud_frame(1, 0, 0);

    /* @0x211F4–0x21244: GEO window + pair latch (start-light / banner path). */
    i960_mmio_write_u32(0x800030u, 0u);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x80u);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x01f00200u);
    /* stq r4 + two st r6 @ 0x21214–0x21224 — six window words. */
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x00f80140u);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x00f80140u);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x00f80140u);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x00f80140u);
    i960_st_u32(I960_WORKRAM, 0x202270, 0, 0x44000000u);
    i960_st_u32(I960_WORKRAM, 0x202274, 0, 0x44000000u);

    ab54 = i960_ld_u32(I960_WORKRAM, 0x20ab54, 0);
    i960_mmio_write_u32(0x800090u, 0u);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x44000000u);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x44000000u);
    i960_mmio_write_u32(0x8000a0u, 0u);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0u);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0xbf34fdf4u);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x3f34fdf4u);

    i960_mmio_write_u32(0x884000u, 0x10002020u);
    i960_mmio_write_u32(0x884000u, 0x12802525u);

    /* @0x212E4: countdown step — g0=table, g1=ab54, g2=0.5, g3=120, g5=1, g6=5. */
    g0 = 0x005c0180u;
    g1 = ab54;
    g2 = 0x3f000000u; /* 0.5f */
    g3 = 0x42f00000u; /* 120.0f */
    g4 = 0x0205ba20u;
    g5 = 1u;
    g6 = 5u;
    geo_countdown_object_step((u32)g0, (u32)g1, (u32)g2);
    i960_st_u32(I960_WORKRAM, 0x20ab54, 0, (u32)g0);

    /* @0x212F0–0x21314: ab54 > 95.0 → switch PRG to 0x5c0340. */
    if (i960_u32_to_f64((u32)g0) > i960_rifl_read(0, 0x4057c000u)) {
        g0 = 0x005c0340u;
        game_start_race_geo_prg_slot((u32)g0, 0, 0);
    }

    i960_mmio_write_u32(0x884000u, 0x10802121u);
}
