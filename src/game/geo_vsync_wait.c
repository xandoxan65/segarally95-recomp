/* Geo frame sync @ 0x4758 — waits on video ctl bit, advances frame counter. */
// @rom 0x4758 +0x88 geo_vsync_wait

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_hw.h"
#include "lift_syms.h"
#include "model2_memory.h"
#include "i960_host.h"

void geo_vsync_wait(u32 arg0, u32 arg1, u32 arg2)
{
    u32 frame;
    u32 geo_read;
    u32 geo_write;
    u32 delta;
    u32 latched_bit2;
    u32 ctl;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    frame = i960_ld_u32(I960_WORKRAM, 0x202260, 0);
    geo_read = frame << 15;
    i960_mmio_write_u32(GEO_READ_START, geo_read);

    geo_write = i960_mmio_read_u32(0x802008);
    delta = geo_write - geo_read;
    i960_st_u32(I960_WORKRAM, 0x20a284, 0, delta);

    /* @0x4788: ldob 0x98000c,g4; @0x4794–4798: latch bit 2 in g5 */
    ctl = i960_mmio_read_u8(0x98000c);
    latched_bit2 = (ctl >> 2) & 1u;

    /*
     * @0x479C–0x47B0: cmpibe until videoctl bit 2 toggles (ld/shro/and loop).
     *
     * Host divergence (primitive only): real hardware busy-spins on MMIO reads.
     * We block on a condvar fed by the host video timer thread that toggles bit 2
     * at refresh rate — same wait semantics, no CPU burn. See model2_hw_vsync.c.
     */
    model2_hw_vsync_wait_bit2_toggle(latched_bit2);
    model2_hw_vsync_frame_done();

    /*
     * irq_init_major @ 0x1490 each vblank: clear 0x202004 (tex pump yield
     * gate @ 0x3A50), bump 0x202008, coin sense/frame, then splash. Host has
     * no timer IRQ — run that path here so IN0 (key 5) can debounce into
     * 0x202040 and AB10 can credit 0x01D00020. Bit5 of 0x202008 gates CREDIT
     * stamp draw vs erase (BEE0/BFA0).
     */
    i960_st_u32(I960_WORKRAM, 0x202004, 0, 0);
    {
        u32 tick = i960_ld_u32(I960_WORKRAM, 0x202008, 0);

        i960_st_u32(I960_WORKRAM, 0x202008, 0, tick + 1u);
    }
    game_io_coin_sense(0, 0, 0);
    game_coin_frame(0, 0, 0);
    /*
     * irq_init_major @ 0x14DC always call 0x26980 — drains 0x26918 colorbase
     * ring via 0x268B0. Do not gate on I960_HOST_BOOT_SCREEN: attract desert
     * leading enqueues scratch rows that never commit without this.
     * @0x14E0: call 0x33180 geo_palette_irq_frame (mode machine → 0x333C8
     * GEO colorbase refresh from 0x5FB89E).
     */
    boot_tile_splash_frame(0, 0, 0);
    geo_palette_irq_frame(0, 0, 0);
    /* irq_timer0_latch @ 0x25C68 — host has no 25 MHz timer0. */
    irq_timer0_drain_pending();

    frame++;
    if (frame >= 2u)
        frame = 0;
    i960_st_u32(I960_WORKRAM, 0x202260, 0, frame);
    geo_read = frame << 15;
    i960_mmio_write_u32(GEO_WRITE_START, geo_read);
}
