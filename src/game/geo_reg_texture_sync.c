/* Geo texture register sync @ 0x476F0 (geo_render_state_clear callee). */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_0476f0_1e0.asm */
// @rom 0x476f0 +0x1dc geo_reg_texture_sync

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"
#include "model2_memory.h"
#include "model2_hw.h"
#include "model2_geo.h"

#include <stdio.h>

/*
 * Three 17-slot uploads into parallel workram tables used later as PRG object
 * headers (marker_burst / course_prg_mode / span_table_draw):
 *   0x2172a0[i], 0x217320[i], 0x2173a0[i]
 * Desert course_prg_mode reads 0x217340 (= 0x217320[8]); a truncated lift that
 * only wrote slot 0 left that cell zero and produced flashing distant geo.
 */

enum { GEO_TEX_SYNC_SLOTS = 17 };

static void tex_sync_push_halfwords(u32 src, u32 count)
{
    u32 i;

    for (i = 0; i < count; i++) {
        u16 hw = i960_ld_u16(I960_ABS, src, 0);

        i960_mmio_write_u32(GEO_PRG_FIFO, (u32)hw);
        src += 2u;
    }
}

static void tex_sync_push_words(u32 src, u32 count)
{
    u32 i;

    for (i = 0; i < count; i++) {
        u32 w = i960_ld_u32(I960_ABS, src, 0);

        i960_mmio_write_u32(GEO_PRG_FIFO, w);
        src += 4u;
    }
}

void geo_reg_texture_sync(u32 arg0, u32 arg1, u32 arg2)
{
    u32 r4;
    u32 r5;
    u32 r6;
    u32 r7;
    u32 r8;
    u32 g0;
    u32 g1;
    u32 g2;
    u32 g4;
    u32 g5;
    u32 g7;
    u32 sum;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /*
     * Host mesh path: upload body is not display-list geometry (suppress PRG
     * ring retention). Hardware still parses geo_texture_data / geo_polygon_data
     * into geo RAM — model2_hw applies those side effects while mesh is off.
     */
    model2_hw_prg_mesh_enable(0);

    /* @0x476F0–0x47788: table @ 0x5e64c0 → slots @ 0x2172a0 (halfword push). */
    i960_mmio_write_u32(0x8000f0, (u32)g14);
    r6 = 0x5e64c0u;
    geo_vsync_wait(0, 0, 0);
    r8 = r6 + 4u;
    r5 = 0x00810000u;
    r4 = 0;
    r7 = 0x2172a0u;
    for (;;) {
        g7 = i960_ld_u32(I960_WORKRAM, r8, 0);
        r5 -= g7;
        i960_st_u32(I960_WORKRAM, r7, 0, r5);
        i960_mmio_write_u32(0x800040, (u32)g14);
        i960_mmio_write_u32(GEO_PRG_FIFO, r5);
        i960_mmio_write_u32(GEO_PRG_FIFO, g7);
        if (g7 != 0u) {
            u32 src = i960_ld_u32(I960_WORKRAM, r6, 0);

            tex_sync_push_halfwords(src, g7);
        }
        i960_mmio_write_u32(0x8000f0, (u32)g14);
        geo_vsync_wait(0, 0, 0);
        r4++;
        if (r4 >= (u32)GEO_TEX_SYNC_SLOTS)
            break;
        r6 += 8u;
        r7 += 4u;
        r8 += 8u;
    }

    /* @0x4778C–0x477B0: sum counts @ 0x5e6554. */
    sum = 0;
    r4 = 0;
    g5 = 0x5e6554u;
    for (;;) {
        r4++;
        g4 = i960_ld_u32(I960_WORKRAM, g5, 0);
        g5 += 8u;
        sum += g4;
        if (r4 >= (u32)GEO_TEX_SYNC_SLOTS)
            break;
    }

    /* @0x477B4–0x47830: table @ 0x5e6550 → slots @ 0x217320. */
    g0 = 0x5e6550u;
    g2 = g0 + 4u;
    r5 -= sum;
    i960_mmio_write_u32(0x800040, (u32)g14);
    r4 = 0;
    i960_mmio_write_u32(GEO_PRG_FIFO, r5);
    g1 = 0x217320u;
    i960_mmio_write_u32(GEO_PRG_FIFO, sum);
    for (;;) {
        g7 = i960_ld_u32(I960_WORKRAM, g2, 0);
        i960_st_u32(I960_WORKRAM, g1, 0, r5);
        if (g7 != 0u) {
            u32 src = i960_ld_u32(I960_WORKRAM, g0, 0);

            tex_sync_push_halfwords(src, g7);
        }
        r4++;
        if (r4 >= (u32)GEO_TEX_SYNC_SLOTS)
            break;
        r5 += g7;
        g0 += 8u;
        g1 += 4u;
        g2 += 8u;
    }

    /* @0x47834–0x478C8: table @ 0x5e65e0 → slots @ 0x2173a0 (word push). */
    i960_mmio_write_u32(0x8000f0, (u32)g14);
    r6 = 0x5e65e0u;
    geo_vsync_wait(0, 0, 0);
    r8 = r6 + 4u;
    r5 = 1u << 15; /* setbit 15,0 */
    r4 = 0;
    r7 = 0x2173a0u;
    for (;;) {
        g7 = i960_ld_u32(I960_WORKRAM, r8, 0);
        r5 -= g7;
        i960_st_u32(I960_WORKRAM, r7, 0, r5);
        i960_mmio_write_u32(0x800050, (u32)g14);
        i960_mmio_write_u32(GEO_PRG_FIFO, r5);
        i960_mmio_write_u32(GEO_PRG_FIFO, g7);
        if (g7 != 0u) {
            u32 src = i960_ld_u32(I960_WORKRAM, r6, 0);

            tex_sync_push_words(src, g7);
        }
        i960_mmio_write_u32(0x8000f0, (u32)g14);
        geo_vsync_wait(0, 0, 0);
        r4++;
        if (r4 >= (u32)GEO_TEX_SYNC_SLOTS)
            break;
        r6 += 8u;
        r7 += 4u;
        r8 += 8u;
    }

    {
        static int s_logged;
        u32 slot8 = i960_ld_u32(I960_WORKRAM, 0x217340, 0);
        u32 peek = 0;
        u32 expect = i960_ld_u32(I960_ABS, 0x029e0f68u, 0);

        (void)model2_geo_peek_polygon_ram0(0x7c11u, &peek);
        if (!s_logged) {
            fprintf(stderr,
                    "lift: geo_reg_texture_sync filled tables; "
                    "0x2172a0=%#x 0x217340=%#x 0x2173a0=%#x "
                    "poly_ram0[0x7c11]=%#x expect_src=%#x%s\n",
                    (unsigned)i960_ld_u32(I960_WORKRAM, 0x2172a0, 0),
                    (unsigned)slot8,
                    (unsigned)i960_ld_u32(I960_WORKRAM, 0x2173a0, 0),
                    (unsigned)peek, (unsigned)expect,
                    (peek == expect) ? " OK" : " MISMATCH");
            s_logged = 1;
        }
    }

    model2_hw_prg_mesh_enable(1);
}
