/* Car road-attach @ 0x2C8D0 — called from 0x2C850 after packing.
 *
 * Seeds node road slots from table pointer @ node+0xec, samples the course
 * span table @ 0x5dce7c[course*16], TGP 0x2f normalize, index_a/b, then
 * pose_publish @ 0x2BC08 into node+0x8c.
 *
 * Callees: road_angles @ 0x2F520; road_span @ 0x311E0 (+ basis @ 0x31320).
 *
 * source: disasm/maincpu/maincpu_02c8d0_400.asm */
// @rom 0x2c8d0 +0x354 game_start_race_obj_road_attach

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "i960_host.h"
#include "model2_rom.h"

#include "lift_syms.h"

#include <stdio.h>
#include <string.h>

static u32 f_neg(u32 bits)
{
    return bits ^ 0x80000000u;
}

void game_start_race_obj_road_attach(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    /* 0xa0 for attach temps; span_basis writes through 0xa0(fp). */
    u8 frame[0xc0];
    u32 node = arg0;
    u32 course;
    u32 pose;
    u32 span_base;
    u32 road;
    u32 i;
    u32 bank;
    u32 flags51;
    u32 e4;
    u32 row;
    u32 dx, dy, dz;
    u32 nrm_x, nrm_y, nrm_z;
    u32 xyz;
    static int logged;

    (void)arg1;
    (void)arg2;

    if (node == 0u)
        return;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    /* @0x2C8D0: lda 0x50(sp),sp — private frame for 0x40/0x70 temps. */
    sp = sp + 0x50u;

    if (!logged) {
        lift_log( "lift: race_obj_road_attach node=%#x\n", node);
        fflush(stderr);
        logged = 1;
    }

    course = i960_host_race_course_index();
    pose = i960_ld_u32(I960_ABS, node, 0x8c);
    span_base = model2_workram_mirror_u32(0x5dce7cu + (course << 4));

    i960_st_u32(I960_ABS, node, 0xe0, 0u);
    i960_st_u32(I960_ABS, node, 0xe8, 2u);

    /* @0x2C8F4–0x2C948: copy up to 8 words from node+0xec into 0x20cb00 bank. */
    flags51 = i960_ld_u8(I960_ABS, pose + 0x51u, 0);
    road = i960_ld_u32(I960_ABS, node, 0xec);
    bank = 0x0020cb00u + (((flags51 << 3) & (15u << 5)));
    for (i = 0; i < 8u; i++) {
        u32 w = i960_ld_u32(I960_ABS, road, 0);
        u32 nxt = i960_ld_u32(I960_ABS, road, 4);

        i960_st_u32(I960_WORKRAM, bank, 0, w);
        if (i960_u32_to_f64(nxt) > 0.0)
            road = road + 4u;
        bank += 4u;
    }

    /* @0x2C94C–0x2C9B4: span row → delta; TGP 0x2f normalize. */
    e4 = i960_ld_u32(I960_ABS, node, 0xe4);
    row = span_base + (e4 << 4);
    {
        u32 x0 = i960_ld_u32(I960_ABS, row, 0);
        u32 y0 = i960_ld_u32(I960_ABS, row, 4);
        u32 z0 = i960_ld_u32(I960_ABS, row, 8);
        u32 x1 = i960_ld_u32(I960_ABS, row, 0x10);
        u32 y1 = i960_ld_u32(I960_ABS, row, 0x14);
        u32 z1 = i960_ld_u32(I960_ABS, row, 0x18);

        dx = (u32)i960_f64_to_u32(i960_u32_to_f64(x1) - i960_u32_to_f64(x0));
        dy = (u32)i960_f64_to_u32(i960_u32_to_f64(y1) - i960_u32_to_f64(y0));
        dz = (u32)i960_f64_to_u32(i960_u32_to_f64(z1) - i960_u32_to_f64(z0));
    }
    memcpy(frame + 0x70u, &dx, 4);
    memcpy(frame + 0x74u, &dy, 4);
    memcpy(frame + 0x78u, &dz, 4);

    i960_mmio_write_u32(0x884000, 0x17802f2fu);
    i960_mmio_write_u32(0x884000, dx);
    i960_mmio_write_u32(0x884000, dy);
    i960_mmio_write_u32(0x884000, dz);
    nrm_x = i960_mmio_read_u32(0x884000);
    nrm_y = i960_mmio_read_u32(0x884000);
    nrm_z = i960_mmio_read_u32(0x884000);
    i960_st_u32(I960_ABS, node + 0x9cu, 0, nrm_x);
    i960_st_u32(I960_ABS, node, 0xa0, nrm_y);
    i960_st_u32(I960_ABS, node, 0xa4, nrm_z);

    xyz = node + 0x14u;
    g0 = course;
    g1 = (u32)i960_ld_u16(I960_ABS, node, 0x88);
    g2 = xyz;
    g3 = 0;
    geo_view_table_index_a((void *)(uintptr_t)g0, (u32)g1, (void *)(uintptr_t)g2);

    /*
     * Disasm: lda 0x40(fp),g1 — guest stack out-buffer. Host fp is a private
     * frame pointer; pass frame+0x40 as a host pointer (index_b detects
     * unmapped VA and uses arg1 as host). Never truncate fp to u32 — that
     * yields a wild 32-bit address (crash at ~0x6fdfxxxx on arm64).
     */
    geo_view_table_index_b((void *)(uintptr_t)xyz, frame + 0x40u, 0u);

    /* @0x2CA1C: call 0x2F520 — g0=sample @ 0x40(fp), g1=normal, g2=angles. */
    game_start_race_obj_road_angles(frame + 0x40u, node + 0x9cu, node + 0x38u);
    {
        static int yaw_logged;

        if (!yaw_logged) {
            lift_log(
                    "lift: road_attach course=%u span=%u yaw=%.3g "
                    "pitch=%.3g node=%#x\n",
                    (unsigned)course,
                    (unsigned)i960_ld_u16(I960_ABS, node, 0x88),
                    i960_u32_to_f64(i960_ld_u32(I960_ABS, node, 0x3c)),
                    i960_u32_to_f64(i960_ld_u32(I960_ABS, node, 0x38)),
                    (unsigned)node);
            fflush(stderr);
            yaw_logged = 1;
        }
    }

    /* @0x2CA20–0x2CA88: flag merge on node+16/+17 and pose+0x51. */
    {
        u8 d8 = i960_ld_u8(I960_WORKRAM, 0x2139d8, 0);
        u8 n16 = i960_ld_u8(I960_ABS, node + 16u, 0);
        u8 n17 = i960_ld_u8(I960_ABS, node + 17u, 0);
        u8 p51;

        i960_st_u16(I960_ABS, node, 0x12, 0);
        n17 = (u8)(n17 | (u8)(3u << 6));
        n16 = (u8)((n16 & 0x6fu) | (u8)(d8 << 7));
        i960_st_u8(I960_ABS, node + 16u, 0, n16);
        n17 = (u8)(n17 & 0xc3u);
        p51 = i960_ld_u8(I960_ABS, pose + 0x51u, 0);
        p51 = (u8)(p51 & (u8)(31u + 29u));
        i960_st_u32(I960_ABS, node, 0xac, 0u);
        i960_st_u8(I960_ABS, node + 17u, 0, (u8)(n17 | p51));
    }

    /* @0x2CC38 clear helper (bal): zero select node fields. */
    i960_st_u32(I960_ABS, node, 0x74, 0u);
    i960_st_u32(I960_ABS, node, 0x20, 0u);
    i960_st_u32(I960_ABS, node, 0x24, 0u);
    i960_st_u32(I960_ABS, node, 0x28, 0u);
    i960_st_u32(I960_ABS, node, 0x44, 0u);
    i960_st_u32(I960_ABS, node, 0x48, 0u);
    i960_st_u32(I960_ABS, node, 0x4c, 0u);
    i960_st_u32(I960_ABS, node, 0xd0, 0u);
    i960_st_u32(I960_ABS, node, 0xd4, 0u);

    /* @0x2CA8C–0x2CB90: TGP push/identity/0x54/0x27/0x2c then 0x311e0. */
    i960_mmio_write_u32(0x884000, 0x10002020u);
    i960_mmio_write_u32(0x884000, 0x12802525u);
    {
        u32 a = dx, b = dy, c = dz;

        memcpy(&a, frame + 0x70u, 4);
        memcpy(&b, frame + 0x74u, 4);
        memcpy(&c, frame + 0x78u, 4);
        i960_mmio_write_u32(0x884000, 0x2a005454u);
        i960_mmio_write_u32(0x884000, a);
        i960_mmio_write_u32(0x884000, 0u);
        i960_mmio_write_u32(0x884000, c);
    }
    {
        u32 sx = i960_ld_u32(I960_ABS, row, 0);
        u32 sz = i960_ld_u32(I960_ABS, row, 8);

        i960_mmio_write_u32(0x884000, 0x13802727u);
        i960_mmio_write_u32(0x884000, f_neg(sx));
        i960_mmio_write_u32(0x884000, 0u);
        i960_mmio_write_u32(0x884000, f_neg(sz));
    }
    {
        u32 px = i960_ld_u32(I960_ABS, xyz, 0);
        u32 py = i960_ld_u32(I960_ABS, node, 0x18);
        u32 pz = i960_ld_u32(I960_ABS, node, 0x1c);
        u32 ox, oy, oz;

        i960_mmio_write_u32(0x884000, 0x16002c2cu);
        i960_mmio_write_u32(0x884000, px);
        i960_mmio_write_u32(0x884000, py);
        i960_mmio_write_u32(0x884000, pz);
        ox = i960_mmio_read_u32(0x884000);
        oy = i960_mmio_read_u32(0x884000);
        oz = i960_mmio_read_u32(0x884000);
        memcpy(frame + 0x80u, &ox, 4);
        memcpy(frame + 0x84u, &oy, 4);
        memcpy(frame + 0x88u, &oz, 4);
        i960_st_u32(I960_ABS, node, 0xa8, ox);
        i960_mmio_write_u32(0x884000, 0x10802121u);
    }

    g0 = node;
    g1 = span_base;
    game_start_race_obj_road_span((u32)g0, (u32)g1, 0);

    /* @0x2CB94–0x2CC04: clear motion slots; seed float defaults. */
    i960_st_u32(I960_ABS, node, 0x80, 0u);
    i960_st_u32(I960_ABS, node, 0x84, 0u);
    i960_st_u32(I960_ABS, node, 0xc0, 0u);
    i960_st_u32(I960_ABS, node, 0xbc, 0u);
    i960_st_u32(I960_ABS, node, 0xb8, 0u);
    i960_st_u32(I960_ABS, node, 0xb4, 0u);
    i960_st_u32(I960_ABS, node, 0xc4, 0u);
    i960_st_u32(I960_ABS, node, 0xc8, 0u);
    i960_st_u32(I960_ABS, node, 0xcc, 0u);
    i960_st_u32(I960_ABS, node, 0x2c, 0u);
    i960_st_u32(I960_ABS, node, 0x70, 0x3e4ccccdu);
    i960_st_u32(I960_ABS, node, 0x6c, 0x3e4ccccdu);
    i960_st_u32(I960_ABS, node, 0x68, 0x3e4ccccdu);
    i960_st_u32(I960_ABS, node, 0x64, 0x3e4ccccdu);
    i960_st_u32(I960_ABS, node, 0x50, 0x447a0000u);
    i960_st_u32(I960_ABS, node, 0x54, 0x448e8000u);
    i960_st_u32(I960_ABS, node, 0x58, 0x448e8000u);
    i960_st_u32(I960_ABS, node, 0x5c, 0x43960000u);
    i960_st_u32(I960_ABS, node, 0x30, 0u);
    i960_st_u32(I960_ABS, node, 0x34, 0u);

    game_start_race_obj_pose_publish(node, 0, 0);

    {
        u8 n17 = i960_ld_u8(I960_ABS, node + 17u, 0);

        i960_st_u32(I960_ABS, node, 4, 0x005cacf0u);
        i960_st_u32(I960_ABS, node, 0xe8, (u32)(n17 >> 6));
    }

    fp = fp_save;
    sp = sp_save;
}
