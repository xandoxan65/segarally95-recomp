/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
// @rom 0x4658 +0x98 geo_reg_bootstrap

#include "i960_lift.h"
#include "model2_rom.h"
#include "i960_mem.h"

void geo_reg_bootstrap(void * arg0, u32 arg1)
{
    (void)arg0;
    (void)arg1;

    g2 = g14;
    g14 = 0;

    /* Host: skip ROM→geo register table blast @ 0x800000; program key geo regs only. */
    i960_mmio_write_u32(0x800414, 4);
    i960_mmio_write_u32(0x800814, 4);
    i960_mmio_write_u32(0x800418, (u32)g14);
    i960_mmio_write_u32(0x800c14, 4);
    i960_mmio_write_u32(0x800818, (u32)g14);
    i960_mmio_write_u32(0x800c18, (u32)g14);
    {
        u8 *z = model2_ram_mut(0x0181c000);
        if (z)
            *z = 0xff;
    }
    (void)g2;
}
