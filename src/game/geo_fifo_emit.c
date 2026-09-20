/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_03eca8_6c.asm */
// @rom 0x3eca8 +0x6c geo_fifo_emit

#include "i960_lift.h"
#include "i960_fp.h"
#include "model2_rom.h"
#include "i960_mem.h"

/* convention: kind=leaf_bx  args g1,g2,g0  link g14→g0 bx */
/* abi: u32 arg0=g1, u32 arg1=g2 → void */

/* pointers: g4=u32, g6=unsigned short * */

void geo_fifo_emit(u32 arg0, u32 arg1)
{
    g1 = (uintptr_t)arg0;
    g2 = (uintptr_t)arg1;

    g0 = g14;
    g14 = 0;
    g5 = i960_ld_u32(I960_WORKRAM, 0x214374, 0);
    if ((unsigned char)g5 == 0)
        goto L_0003ed10;
    g4 = i960_ld_u32(I960_WORKRAM, 0x214370, 0);
    i960_mmio_write_u32(0x800040, (u32)g14); /* geo_regs */;
    i960_mmio_write_u32(0x804000, (u32)g4); /* geo_prg_fifo */;
    i960_mmio_write_u32(0x804000, (u32)g5); /* geo_prg_fifo */;
    g5 = g5 - 1;
    g7 = 0 - 1;
    /* Disasm: lda 0x214380,g6 then ldos loop — CRX work window. */
    g6 = 0x214380u;
    if (g5 == g7)
        goto L_0003ed10;
    do {
        g4 = i960_ld_u16(I960_ABS, (u32)g6, 0);
        g5 = g5 - 1;
        g6 = g6 + 0x2;
        i960_mmio_write_u32(0x804000, (u32)g4); /* geo_prg_fifo */;
    } while (g5 != g7);

    L_0003ed10:
        /* Host C path (game_dispatch_main) has no bal link in g14. */
        if (g0 != 0)
            i960_call_indirect(g0);
        return;
}
