/* Copy layer regs + blit/scroll frame @ 0x26980. */
// @rom 0x26980 +0x48 boot_tile_splash_frame

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"

void boot_tile_splash_frame(u32 arg0, u32 arg1, u32 arg2)
{
    u32 layer_dest;
    u32 layer_src;
    u32 idx;
    u16 word;
    u32 handler;
    u32 guard;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    layer_dest = 0x0100a000u;
    layer_src = 0x0020b914u;
    for (idx = 0; idx <= 7u; idx++) {
        word = i960_ld_u16(I960_WORKRAM, layer_src, 0);
        layer_src += 2u;
        i960_st_u16(I960_ABS, layer_dest, 0, word);
        layer_dest += 2u;
    }

    /* Direct calls — safe when i960_call_rom is halted (milestone dump). */
    boot_tile_blit_chain(0, 0, 0);
    /*
     * ROM @ 0x268B0 drains at most 16 colorbase ring entries per call (one
     * vblank). Host often presents once after a burst of 0x26918 enqueues
     * (splash CGM queues 40+ slots; tiles need the last banks). Drain until
     * empty so the presented frame matches post-N-vblank hardware.
     */
    guard = 0x80u;
    do {
        boot_tile_scroll_copy(0, 0, 0);
    } while (i960_ld_u32(I960_WORKRAM, 0x20b1f0, 0) != 0 && --guard != 0u);

    handler = i960_ld_u32(I960_WORKRAM, 0x20b910, 0);
    if (handler != 0)
        i960_call_indirect(handler);
}
