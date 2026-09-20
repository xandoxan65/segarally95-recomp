/* Countdown object step @ 0x3F9A0 — emit digit poses + advance 0x20ab54.
 *
 * Call site (countdown PRG @ 0x211F0): g1=ab54, g2=0.5f, g3=120.0f, g5=1,
 * g0=descriptor table 0x5c0180, g4=0x205ba20 (main_data keyframes), g6=5.
 *
 * Per 0x14-byte record: 0x3F550 → 0x215c00, TGP pose from that buffer, and
 * when record mode==0 and gate≠0 bind 0x05 + catalog draw.
 *
 * source: disasm/maincpu/maincpu_03f9a0_400.asm */
// @rom 0x3f9a0 +0x31c geo_countdown_object_step

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_memory.h"
#include "model2_rom.h"

#include "lift_syms.h"

#include <stdio.h>

static u32 f64_bits(double v)
{
    return (u32)i960_f64_to_u32(v);
}

static void tgp_pose_from_215c00(void)
{
    i960_mmio_write_u32(0x884000u, 0x13802727u);
    i960_mmio_write_u32(0x884000u, i960_ld_u32(I960_ABS, 0x00215c00u, 0));
    i960_mmio_write_u32(0x884000u, i960_ld_u32(I960_ABS, 0x00215c00u, 4));
    i960_mmio_write_u32(0x884000u, i960_ld_u32(I960_ABS, 0x00215c00u, 8));
    i960_mmio_write_u32(0x884000u, 0x15802b2bu);
    i960_mmio_write_u32(0x884000u, i960_ld_u32(I960_ABS, 0x00215c00u, 0x14));
    i960_mmio_write_u32(0x884000u, 0x15002a2au);
    i960_mmio_write_u32(0x884000u, i960_ld_u32(I960_ABS, 0x00215c00u, 0x10));
    i960_mmio_write_u32(0x884000u, 0x14802929u);
    i960_mmio_write_u32(0x884000u, i960_ld_u32(I960_ABS, 0x00215c00u, 0xc));
}

static void emit_catalog(u32 cat_ix)
{
    u32 cursor = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
    u32 geo_wr = i960_ld_u32(I960_ABS, 0x00802008u, 0);
    u32 bank;
    u32 bump;
    u32 c0, c1, c2, c3;

    if (cursor != 0u)
        i960_st_u32(I960_ABS, cursor, 0, geo_wr);
    i960_mmio_write_u32(0x884000u, 0x02800505u);
    i960_mmio_write_u32(0x801008u, geo_wr + 0x34u);

    bank = (i960_ld_u32(I960_WORKRAM, 0x202278, 0) & 3u) << 10;
    i960_st_u32(I960_ABS, 0x00800010u + bank, 0, 0);

    c0 = i960_ld_u32(I960_ABS, 0x02864b40u, cat_ix << 4);
    c1 = i960_ld_u32(I960_ABS, 0x02864b40u, (cat_ix << 4) + 4u);
    c2 = i960_ld_u32(I960_ABS, 0x02864b40u, (cat_ix << 4) + 8u);
    c3 = i960_ld_u32(I960_ABS, 0x02864b40u, (cat_ix << 4) + 12u);
    if (cursor != 0u)
        i960_st_u32(I960_WORKRAM, 0x20a290, 0, cursor + 4u);
    bump = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
    /* @0x3FB40: addo g3,g4 — g3 is catalog word3 from ldq (c3). */
    i960_st_u32(I960_WORKRAM, 0x20b940, 0, bump + c3);
    i960_mmio_write_u32(GEO_PRG_FIFO, c0);
    i960_mmio_write_u32(GEO_PRG_FIFO, c1);
    i960_mmio_write_u32(GEO_PRG_FIFO, c2);
    i960_mmio_write_u32(GEO_PRG_FIFO, c3);
}

void geo_countdown_object_step(u32 arg0, u32 arg1, u32 arg2)
{
    u32 table = arg0;
    u32 t_bits = arg1;
    u32 step_bits = arg2;
    u32 lim_bits = (u32)g3;
    u32 mesh_base = (u32)g4;
    u32 mode = (u32)g5;
    i32 remain = (i32)((u32)g6) - 1;
    u32 r4;
    u32 r6;
    u32 r12;
    static int logged;

    if (!logged) {
        fprintf(stderr, "lift: countdown_object_step emit+epilogue\n");
        fflush(stderr);
        logged = 1;
    }

    if (remain != -1) {
        r4 = table;
        r6 = table + 4u;
        r12 = table + 0x10u;

        do {
            u32 kf_off = i960_ld_u32(I960_WORKRAM, r12, 0);
            u32 flag;
            u32 rec_mode;
            u32 g13_flag;
            i32 pop_n;

            g0 = 0x00215c00u;
            g1 = mesh_base + kf_off;
            g2 = t_bits;
            g3 = step_bits;
            g4 = lim_bits;
            g5 = mode;
            geo_countdown_keyframe_eval((u32)g0, (u32)g1, (u32)g2);

            flag = i960_ld_u32(I960_WORKRAM, r4, 0);
            if (flag != 0u)
                i960_mmio_write_u32(0x884000u, 0x10002020u);

            rec_mode = i960_ld_u32(I960_WORKRAM, r6, 0);
            g13_flag = flag;

            if (rec_mode == 0u) {
                u32 gate;

                tgp_pose_from_215c00();
                gate = i960_ld_u32(I960_ABS, 0x00215c00u, 0x18);
                if (gate != 0u)
                    emit_catalog(i960_ld_u32(I960_WORKRAM, r6 + 8u, 0));
            } else if (rec_mode == 1u || rec_mode == 2u) {
                tgp_pose_from_215c00();
            }

            /* Advance records; compute pop count (@0x3FC20–0x3FC70). */
            r12 += 0x14u;
            r6 += 0x14u;
            r4 += 0x14u;
            if (remain == 0)
                pop_n = (i32)g13_flag + 1;
            else {
                u32 next_flag = i960_ld_u32(I960_WORKRAM, r4, 0);

                pop_n = (i32)g13_flag - (i32)next_flag + 1;
            }
            remain -= 1;

            if (pop_n > 0) {
                while (pop_n > 0) {
                    pop_n -= 1;
                    if (pop_n == -1)
                        break;
                    i960_mmio_write_u32(0x884000u, 0x10802121u);
                }
            }
        } while (remain != -1);
    }

    /* @0x3FC80 epilogue. */
    {
        double t = i960_u32_to_f64(t_bits) + i960_u32_to_f64(step_bits);
        double lim = i960_u32_to_f64(lim_bits);

        if (t < lim) {
            g0 = f64_bits(t);
            return;
        }
        if (mode == 1u) {
            g0 = f64_bits(lim - 1.0);
            return;
        }
        if (mode == 2u) {
            g0 = 0;
            return;
        }
        g0 = f64_bits(t);
    }
}
