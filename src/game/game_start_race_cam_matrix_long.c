/* Race cam matrix long path @ 0x203D0 — per-frame when cam_matrix g0!=0
 * (hud_frame). Draws the START CGM slot, seeds GEO window/PRG words, then
 * TGP push→identity→translate(−185,−127,128)→scale(40,40,40)→roll→0x05
 * matrix emit + catalog row @ 0x2865130 (MAME geo_prg_w / TGP marker 0x05).
 *
 * source: disasm/maincpu/maincpu_0203d0_300.asm
 * MAME ref: geo_prg_w, geo opcode contracts in model2_v.cpp (reference only). */
// @rom 0x203d0 +0x300 game_start_race_cam_matrix_long

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_memory.h"
#include "model2_rom.h"

#include "lift_syms.h"

#include <stdio.h>

static u32 f32_bits(double v)
{
    return (u32)i960_f64_to_u32(v);
}

void game_start_race_cam_matrix_long(u32 arg0, u32 arg1, u32 arg2)
{
    u32 cam;
    u32 slot;
    u32 ang;
    double ang_d;
    double scaled;
    double roll;
    u32 roll_bits;
    u32 geo_wr;
    u32 cursor;
    u32 cat0, cat1, cat2, cat3;
    u32 bank;
    u32 bump;
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    if (!logged) {
        lift_log( "lift: race_cam_matrix_long\n");
        fflush(stderr);
        logged = 1;
    }

    cam = i960_ld_u32(I960_WORKRAM, 0x2140cc, 0);
    if (cam == 0u) {
        static int skip_logged;

        if (!skip_logged) {
            lift_log( "lift: race_cam_matrix_long skip (2140cc=0)\n");
            fflush(stderr);
            skip_logged = 1;
        }
        return;
    }

    /* @0x203D0–0x20400: draw_scene slot = cam+0x78 + 1. */
    slot = i960_ld_u32(I960_ABS, cam, 0x78) + 1u;
    g0 = 10;
    g1 = 31u + 11u; /* 42 */
    g2 = i960_ld_u32(I960_WORKRAM, 0x20ac78, 0);
    g3 = slot;
    g4 = 0;
    draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

    /* @0x20404–0x20428: GEO window + PRG words (stq r4 + two st r6). */
    i960_mmio_write_u32(0x800030u, 0);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x80u);           /* setbit 7,0 */
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x01f00200u);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x00f80140u);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x00f80140u);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x00f80140u);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x00f80140u);

    /* Seed pair consumed by geo_prg_slot FIFO push after callx returns. */
    i960_st_u32(I960_WORKRAM, 0x202270, 0, 0x43000000u); /* 128.0f */
    i960_st_u32(I960_WORKRAM, 0x202274, 0, 0x43000000u);

    i960_mmio_write_u32(0x800090u, 0);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x43000000u);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x43000000u);

    /* @0x2046C–0x204FC: TGP 0x20 push, 0x25 I, 0x27 T, 0x28 scale. */
    i960_mmio_write_u32(0x884000, 0x10002020u);
    i960_mmio_write_u32(0x884000, 0x12802525u);
    i960_mmio_write_u32(0x884000, 0x13802727u);
    i960_mmio_write_u32(0x884000, 0xc3390000u); /* -185 */
    i960_mmio_write_u32(0x884000, 0xc2fe0000u); /* -127 */
    i960_mmio_write_u32(0x884000, 0x43000000u); /* +128 */
    i960_mmio_write_u32(0x884000, 0x14002828u);
    i960_mmio_write_u32(0x884000, 0x42200000u); /* 40 */
    i960_mmio_write_u32(0x884000, 0x42200000u);
    i960_mmio_write_u32(0x884000, 0x42200000u);

    /* @0x20504–0x20620: roll from cam+0x7c via disasm double compares. */
    ang = i960_ld_u32(I960_ABS, cam, 0x7c);
    ang_d = i960_u32_to_f64(ang);
    /* cmprl vs 0x40890000:00000000 → 800.0 */
    if (ang_d < i960_rifl_read(0, 0x40890000u))
        ang_d = i960_u32_to_f64(0x44480000u); /* 800.0f */
    /* / 1.1 (0x3ff19999_9999999a) */
    scaled = ang_d / i960_rifl_read(0x9999999au, 0x3ff19999u);

    {
        double bound = i960_rifl_read(0, 0x40af4000u);     /* ≈2220 as double */
        double quarter = i960_rifl_read(0x54442d18u, 0x3fe921fbu); /* π/4 */
        double pi = i960_rifl_read(0x54442d18u, 0x3ff921fbu);

        if (scaled < bound) {
            /* @0x205CC: (scaled * π/4) / bound − π */
            roll = (scaled * quarter) / bound - pi;
        } else {
            /* @0x20584: ((scaled − bound) * π/4) / 0x408f4000 − π/4 */
            roll = ((scaled - bound) * quarter)
                / i960_rifl_read(0, 0x408f4000u) - quarter;
        }
    }
    roll_bits = f32_bits(roll);

    /* @0x20628: TGP 0x2b roll. */
    i960_mmio_write_u32(0x884000, 0x15802b2bu);
    i960_mmio_write_u32(0x884000, roll_bits);

    /* @0x20640–0x20688: capture geo write ptr, TGP 0x05, poke +0x34. */
    cursor = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
    geo_wr = i960_ld_u32(I960_ABS, 0x00802008u, 0);
    if (cursor != 0u)
        i960_st_u32(I960_ABS, cursor, 0, geo_wr);
    i960_mmio_write_u32(0x884000, 0x02800505u);
    i960_mmio_write_u32(0x801008u, geo_wr + 0x34u);

    /* @0x20670: lda 0x800010[(0x202278&3)<<10]; st g14. */
    bank = (i960_ld_u32(I960_WORKRAM, 0x202278, 0) & 3u) << 10;
    i960_mmio_write_u32(0x00800010u + bank, 0);

    /* Catalog row @ 0x2865130 → PRG; TGP 0x21 pop; bump cursors.
     * main_data — I960_ROM / model2_rom_at, not workram_mirror (zeros). */
    cat0 = i960_ld_u32(I960_ROM, 0x02865130u, 0);
    cat1 = i960_ld_u32(I960_ROM, 0x02865130u, 4);
    cat2 = i960_ld_u32(I960_ROM, 0x02865130u, 8);
    cat3 = i960_ld_u32(I960_ROM, 0x02865130u, 12);
    {
        static int cat_logged;

        if (!cat_logged) {
            lift_log(
                    "lift: tach catalog 0x2865130 %08x %08x %08x %08x\n",
                    cat0, cat1, cat2, cat3);
            fflush(stderr);
            cat_logged = 1;
        }
    }
    bump = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
    i960_mmio_write_u32(GEO_PRG_FIFO, cat0);
    i960_mmio_write_u32(GEO_PRG_FIFO, cat1);
    i960_mmio_write_u32(GEO_PRG_FIFO, cat2);
    i960_mmio_write_u32(GEO_PRG_FIFO, cat3);
    i960_mmio_write_u32(0x884000, 0x10802121u);

    /* @0x206B8: always addo 4 to the 0x20a290 cursor (even if it was 0). */
    i960_st_u32(I960_WORKRAM, 0x20a290, 0, cursor + 4u);
    /* @0x206C4: addo g3,g4 — g3 is catalog word3 after ldq, not the Sys24 slot. */
    i960_st_u32(I960_WORKRAM, 0x20b940, 0, bump + cat3);
}
