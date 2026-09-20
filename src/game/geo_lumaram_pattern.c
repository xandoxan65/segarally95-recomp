/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: disasm @ 0x4590 (maincpu_004590_80.asm) */
// @rom 0x4590 +0x80 geo_lumaram_pattern

#include "i960_lift.h"
#include "model2_rom.h"
#include "i960_mem.h"
#include "model2_memory.h"

/* Fills lumaram lookup stripes @ 0x12800000 from a walking pattern.
 * Hardware: stos every +4 with umask32(0xff) → compact m_lumaram[i]. */

void geo_lumaram_pattern(u32 arg0, u32 arg1, u32 arg2)
{
    u32 g6;
    u32 g7;
    u32 ea;

    g0 = (uintptr_t)arg0;
    g1 = (uintptr_t)arg1;
    g2 = (uintptr_t)arg2;

    /* @0x4598–0x45C8: 0x80 halfword stores, stride 4, value (i>>1). */
    g6 = 0;
    g7 = 0;
    ea = LUMARAM_BASE;
    while (g7 <= 0x7fu) {
        u16 v = (u16)((g6 & 0xffffu) >> 1);

        i960_st_u16(I960_ABS, ea, 0, v);
        ea += 4u;
        g7++;
        g6++;
    }

    /* @0x45CC–0x460C: second stripe; skip counter when ==0x7e; skip nibble 0xF. */
    g6 = 0;
    g7 = 0;
    while (g7 <= 0x7fu) {
        u16 v = (u16)((g6 & 0xffffu) >> 1);

        i960_st_u16(I960_ABS, ea, 0, v);
        ea += 4u;
        if ((g6 & 0xffffu) != 0x7eu) {
            g6++;
            if ((g6 & 15u) == 15u)
                g6++;
        }
        g7++;
    }
}
