/* Draw-frame geo FIFO blast @ 0x4E90 — call geo_fifo_bootstrap then post stream. */
/* source: decomp/disasm/maincpu/maincpu_004e90_200.asm */
// @rom 0x4e90 +0xb4 geo_draw_frame_entry

#include "i960_lift.h"
#include "i960_fp.h"
#include "model2_rom.h"
#include "i960_mem.h"
#include "placement_catalog_feed.h"
#include "i960_host_scene.h"
#include "lift_syms.h"
#include "model2_memory.h"

#include <stdlib.h>
#include <string.h>

/*
 * Disasm @ 0x4E90–0x5044:
 *   call geo_fifo_bootstrap
 *   GEO 0x09 ← stl (450,450)   [was broken: st_u64(0x43e10000) wrote (450,0)]
 *   copro identity / yaw from 0x20a2a4 / scale 1.0
 *   GEO 0x0A ← unit/eye triple from copro out
 *   translate + 0x05 latch
 */

void geo_draw_frame_entry(u32 arg0, u32 arg1, u32 arg2)
{
    u32 yaw;
    u32 wr_slot;
    u32 wr_ptr;
    u32 ux, uy, uz;
    uintptr_t fp_save = fp;
    u8 frame[0x50];

    (void)arg0;
    (void)arg1;
    (void)arg2;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    sp = sp + 16;

    /* @0x4E94 */
    geo_fifo_bootstrap((u32)g0, (u32)g1, (u32)g2);

    /* @0x4E98–0x4EBC: stash focal 450 into workram; yaw ← 0x20a2a4 */
    yaw = i960_ld_u32(I960_WORKRAM, 0x20a2a4, 0);
    i960_st_u32(I960_WORKRAM, 0x202270, 0, 0x43e10000u);
    i960_st_u32(I960_WORKRAM, 0x202274, 0, 0x43e10000u);

    /* @0x4EC4–0x4ED8: GEO cmd 0x09 + stl (450,450) — both lanes same float. */
    i960_mmio_write_u32(0x800090, 0);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x43e10000u);
    i960_mmio_write_u32(GEO_PRG_FIFO, 0x43e10000u);

    /* @0x4EE0–0x4F44: copro setup — identity, yaw from 0x20a2a4, pad, scale 1. */
    i960_mmio_write_u32(COPRO_FIFO, 0x11802323u);
    i960_mmio_write_u32(COPRO_FIFO, 0x12802525u);
    i960_mmio_write_u32(COPRO_FIFO, 0x15002a2au);
    i960_mmio_write_u32(COPRO_FIFO, yaw);
    i960_mmio_write_u32(COPRO_FIFO, 0x16002c2cu);
    i960_mmio_write_u32(COPRO_FIFO, 0);
    i960_mmio_write_u32(COPRO_FIFO, 0);
    i960_mmio_write_u32(COPRO_FIFO, 0x3f800000u); /* +1.0 */

    /* @0x4F4C–0x4F68: pop three floats (unit / intermediate). */
    ux = i960_ld_u32(I960_MMIO, COPRO_FIFO, 0);
    *(u32 *)(fp + 0x40) = ux;
    uy = i960_ld_u32(I960_MMIO, COPRO_FIFO, 0);
    *(u32 *)(fp + 0x44) = uy;
    uz = i960_ld_u32(I960_MMIO, COPRO_FIFO, 0);

    /* @0x4F70–0x4F8C: GEO cmd 0x0A + stl (ux,uy) + st uz */
    i960_mmio_write_u32(0x8000a0, 0);
    i960_mmio_write_u32(GEO_PRG_FIFO, ux);
    i960_mmio_write_u32(GEO_PRG_FIFO, uy);
    i960_mmio_write_u32(GEO_PRG_FIFO, uz);

    /* @0x4F94–0x4FF8: push + translate (0,0,4); update 0x20a2a4 = yaw + const. */
    i960_mmio_write_u32(COPRO_FIFO, 0x12802525u);
    i960_mmio_write_u32(COPRO_FIFO, 0x13802727u);
    {
        double yaw_f = i960_u32_to_f64(yaw);
        double addend = i960_rifl_read(0xf37bebd5u, 0x3f8acee9u);
        u32 updated = i960_f64_to_u32(yaw_f + addend);

        i960_mmio_write_u32(COPRO_FIFO, 0);
        i960_mmio_write_u32(COPRO_FIFO, 0);
        i960_mmio_write_u32(COPRO_FIFO, 0x40800000u); /* 4.0 */
        *(u32 *)(fp + 0x48) = uz;

        wr_slot = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
        wr_ptr = i960_ld_u32(I960_MMIO, 0x802008, 0);
        i960_st_u32(I960_WORKRAM, 0x20a290, 0, wr_slot + 4u);
        i960_st_u32(I960_WORKRAM, 0x20a2a4, 0, updated);
        i960_st_u32(I960_WORKRAM, wr_slot, 0, wr_ptr);
        i960_mmio_write_u32(COPRO_FIFO, 0x02800505u);
        i960_mmio_write_u32(0x801008, wr_ptr + 0x34u);
    }

    fp = fp_save;

    /*
     * Host-only: optional placement feed after ROM body (env / seeded scene).
     * Not present in disasm @ 0x4E90.
     */
    {
        const char *pf = getenv("I960_HOST_PLACEMENT_FEED");
        int feed = (pf && pf[0] && pf[0] != '0') || i960_host_scene_seeded();

        if (feed) {
            u32 cursor = i960_ld_u32(I960_WORKRAM, 0x20a2ec, 0);
            u32 batch = placement_catalog_feed_env_batch();

            if (placement_catalog_feed_source())
                placement_catalog_feed_track_batch(cursor, batch);
            else
                placement_catalog_feed_race_batch(cursor, batch);
            cursor += batch;
            if (placement_catalog_feed_source()) {
                if (cursor >= PLACEMENT_TRACK_DESERT_START + PLACEMENT_TRACK_DESERT_COUNT)
                    cursor = PLACEMENT_TRACK_DESERT_START;
            } else if (cursor >= 24u) {
                cursor = 0;
            }
            i960_st_u32(I960_WORKRAM, 0x20a2ec, 0, cursor);
            i960_st_u32(I960_WORKRAM, PLACEMENT_CURSOR, 0, cursor);
        }
    }
}
