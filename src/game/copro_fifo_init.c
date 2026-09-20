/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_00004ca8_80.asm */
// @rom 0x4ca8 +0x80 copro_fifo_init

#include "i960_lift.h"
#include "model2_rom.h"
#include "i960_mem.h"

/* convention: kind=leaf_bx  args g0,g1,g2  link g14→g2 bx */
/* abi: u32 arg0=g0, u32 arg1=g1 → void */

void copro_fifo_init(u32 arg0, u32 arg1)
{
    u32 idx;
    const u8 *src;
    u8 *dst;

    g0 = (uintptr_t)arg0;
    g1 = (uintptr_t)arg1;
    g2 = g14;
    g14 = 0;

    g1 = (uintptr_t)i960_vaddr_ptr(0x1000202);
    i960_mmio_write_u32(0x884000, (u32)g1);
    g0 = i960_ld_u32(I960_MMIO, 0x884000, 0);

    src = model2_rom_at(0x91fe00);
    dst = model2_ram_mut(0x1d001e6);
    if (src && dst) {
        for (idx = 0; idx <= 0x7f; idx++) {
            u16 word = (u16)(src[0] | ((u16)src[1] << 8));
            *(u16 *)(dst + idx * 2) = word;
            src += 4;
        }
    }

    g4 = i960_ld_u16(I960_ROM, 0x1d001e4, 0);
    g1 = (uintptr_t)i960_vaddr_ptr(0x800101);
    i960_mmio_write_u32(0x884000, (u32)g1);
    i960_st_u16(I960_ROM, 0x1d001e4, 0, (u16)(g4 + g0));
    (void)g2;
}
