/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_00004ac0_f8.asm */
// @rom 0x4ac0 +0xf8 geo_fifo_bootstrap

#include "i960_lift.h"
#include "model2_rom.h"
#include "i960_mem.h"

#include "lift_syms.h"

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret */
/* abi: u32 arg0=g0, u32 arg1=g1, u32 arg2=g2 → void */

void geo_fifo_bootstrap(u32 arg0, u32 arg1, u32 arg2)
{
    g0 = (uintptr_t)arg0;
    g1 = (uintptr_t)arg1;
    g2 = (uintptr_t)arg2;

    /* ROM st g14 — float/int 0 on this path (cleared link). */
    i960_mmio_write_u32(0x800160, 0u); /* geo_regs */;
    g4 = 0x44800000u;
    i960_mmio_write_u32(0x804000, (u32)g4); /* geo_prg_fifo */;
    i960_mmio_write_u32(0x800070, 0u); /* geo_regs */;
    g4 = 1;
    i960_mmio_write_u32(0x804000, (u32)g4); /* geo_prg_fifo */;
    i960_mmio_write_u32(0x800080, 0u); /* geo_regs */;
    g0 = 0;
    g4 = 0x41000000u;
    g1 = 22;
    i960_mmio_write_u32(0x804000, (u32)g4); /* geo_prg_fifo */;
    g2 = 0x5a39c0u; /* lda 0x5a39c0 — matrix payload in workram */
    geo_fifo_matrix_emit((u32)g0, (u32)g1, (u32)g2);
    r4 = 0x80u; /* setbit 7, 0 */
    r5 = 0x01f00200u;
    i960_mmio_write_u32(0x800030, 0u); /* geo_regs */;
    r6 = 0x0f80140u;
    r7 = r6;
    /* stq r4 @ 0x4B38 emits r4,r5,r6,r7 to the same FIFO port. */
    i960_mmio_write_u32(0x804000, (u32)r4);
    i960_mmio_write_u32(0x804000, (u32)r5);
    i960_mmio_write_u32(0x804000, (u32)r6);
    i960_mmio_write_u32(0x804000, (u32)r7);
    /* Two following st r6 @ 0x4B40 / 0x4B48 complete four eye centers. */
    i960_mmio_write_u32(0x804000, (u32)r6); /* geo_prg_fifo */;
    i960_mmio_write_u32(0x804000, (u32)r6); /* geo_prg_fifo */;
    g4 = 0x43af0000u;
    i960_st_u32(I960_WORKRAM, 0x202270, 0, (u32)g4);
    i960_st_u32(I960_WORKRAM, 0x202274, 0, (u32)g4);
    i960_mmio_write_u32(0x800090, 0u); /* geo_regs */;
    r8 = 0x43af0000u;
    r9 = r8;
    i960_mmio_write_u32(0x804000, (u32)r8);
    i960_mmio_write_u32(0x804000, (u32)r9);
    i960_mmio_write_u32(0x8000a0, 0u); /* geo_regs */;
    r10 = 0;
    r11 = 0xbf800000u;
    i960_mmio_write_u32(0x804000, (u32)r10);
    i960_mmio_write_u32(0x804000, (u32)r11);
    g4 = 0x3f34fdf4u;
    i960_st_u32(I960_WORKRAM, 0x20b940, 0, 0);
    i960_mmio_write_u32(0x804000, (u32)g4); /* geo_prg_fifo */;
}
