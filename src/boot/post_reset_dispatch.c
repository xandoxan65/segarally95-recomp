/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_00000570_80.asm */
// @rom 0x570 +0x80 post_reset_dispatch

#include "i960_lift.h"
#include "i960_host.h"
#include "model2_rom.h"
#include "i960_mem.h"

/* @rom 0x5c0 — flushreg + seed prior-frame slots before callx dispatch loop */
static void post_reset_flushreg(void)
{
    /* lift: flushreg @ 0x5c0 — host no-op */
    pfp = 7 | pfp;
    g0 = 0;
    *(u32 *)(fp - 0x10) = (u32)g0;
    g0 = (uintptr_t)i960_vaddr_ptr(0x3f001000);
    *(u32 *)(fp - 0xc) = (u32)g0;
}

u32 post_reset_dispatch(u32 arg0, u32 arg1, u32 arg2)
{
    g1 = (uintptr_t)arg1;
    g2 = (uintptr_t)arg2;

    g0 = 1 << 6;
    sp = sp + g0;
    g6 = (uintptr_t)i960_vaddr_ptr(0xff000004);
    g5 = (uintptr_t)(model2_maincpu_rom + 0x564);
    i960_synmov(g6, g5);
    post_reset_flushreg();
    fp = (uintptr_t)(model2_crx_ram + 0x840);
    pfp = fp - 0x40;
    sp = fp + 0x40;
    g14 = 0;

    for (;;) {
        if (i960_host_dispatch_halted())
            break;
        g4 = i960_ld_u32(I960_ROM, 0x1000, 0);
        i960_call_indirect(g4);
    }
    return 0;
}
