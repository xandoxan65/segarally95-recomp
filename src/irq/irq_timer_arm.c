/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_00001650_100.asm */
// @rom 0x1650 +0x100 irq_timer_arm

#include "i960_lift.h"
#include "i960_mem.h"

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret  callee r4 */
/* abi: u32 arg0=g0, u32 arg1=g1, u32 arg2=g2 → void */
/* call site: caller 0x18fc */
/* call site: caller 0x1910 */
/* call site: caller 0x1924 */

extern void irq_timer0_latch(u32 arg0, u32 arg1);

void irq_timer_arm(u32 arg0, u32 arg1, u32 arg2)
{
    u32 mask;
    u32 bit = arg0;

    g0 = (uintptr_t)arg0;
    g1 = (uintptr_t)arg1;
    g2 = (uintptr_t)arg2;

    /* andnot g0, 0x20200c — clear this IRQ bit while reprogramming. */
    mask = i960_ld_u32(I960_WORKRAM, 0x20200c, 0);
    mask &= ~bit;
    i960_st_u32(I960_WORKRAM, 0x20200c, 0, mask);
    mask = i960_ld_u32(I960_WORKRAM, 0x20200c, 0);
    i960_mmio_write_u32(0xe80004, mask); /* irq_enable */

    /* cmpi g0,8 / be 0x16bc; cmpobl 8,g0 / cmpibe 4 → timer0 @ 0x169c.
     * Leaf g14 is 0; host g14 is not a timer value. */
    if (bit == 8u) {
        i960_mmio_write_u32(0xf00004, 0); /* timers */
        i960_mmio_write_u32(0xf00004, 0xfffffu);
    } else if (bit > 8u) {
        if (bit == 16u) {
            i960_mmio_write_u32(0xf00008, 0);
            i960_st_u32(I960_WORKRAM, 0x202004, 0, 1u);
            i960_mmio_write_u32(0xf00008, 0xfffffu);
        } else if (bit == 32u) {
            i960_mmio_write_u32(0xf0000c, 0);
            i960_mmio_write_u32(0xf0000c, 0xfffffu);
        }
    } else if (bit == 4u) {
        i960_mmio_write_u32(0xf00000, 0);
        irq_timer0_latch((u32)g0, (u32)g2);
        /* lda 0xc350,g6 — period, not a ROM pointer. */
        i960_mmio_write_u32(0xf00000, 0xc350u);
    }

    mask = i960_ld_u32(I960_WORKRAM, 0x20200c, 0);
    mask |= bit;
    i960_st_u32(I960_WORKRAM, 0x20200c, 0, mask);
    mask = i960_ld_u32(I960_WORKRAM, 0x20200c, 0);
    i960_mmio_write_u32(0xe80004, mask); /* irq_enable */
    i960_mmio_write_u32(0xe80000, ~bit); /* irq_request */
}
