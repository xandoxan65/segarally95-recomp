/* Slot word pack @ 0x2B0B0 — probe table 0x5DCAC8 slots via geo_view_slot_word_probe.
 * source: disasm/maincpu/maincpu_02b0b0_c4.asm */
// @rom 0x2b0b0 +0xc4 geo_view_slot_word_pack

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "i960_host.h"

#include "lift_syms.h"

u32 geo_view_slot_word_pack(u32 arg0, u32 arg1, u32 arg2)
{
    u32 tab;
    u32 base;
    i32 r7s;
    u32 r6v;
    u32 r11v;
    u32 r10v;
    i32 r5s;
    i32 r8s;
    i32 r4s;
    u32 g4v;

    (void)arg2;

    tab = i960_host_race_course_index();
    tab = tab + (tab << 1);
    base = i960_ld_u32(I960_WORKRAM, 0x5dcac8u + (tab << 2), 0);
    r7s = (i32)(short)i960_ld_u16(I960_WORKRAM, arg1 + 2u, 0);
    r12 = arg0;
    r9 = arg1;
    r11v = i960_ld_u32(I960_WORKRAM, base + 4u, 0);
    r10v = i960_ld_u32(I960_WORKRAM, base + 8u, 0);
    r6v = i960_ld_u32(I960_WORKRAM, base, 0);

    if (r7s < 0) {
        do {
            r7s += (i32)r6v;
        } while (r7s < 0);
    }
    if ((i32)r6v != 0)
        r7s = r7s % (i32)r6v;

    r5s = 0;
    if ((i32)r6v <= 0)
        goto fail;

    for (;;) {
        if (r5s & 1)
            r8s = (r5s >> 1) + 1;
        else
            r8s = (-r5s) >> 1;
        g4v = (u32)(r7s + r8s + (i32)r6v);
        r4s = (i32)g4v % (i32)r6v;
        g0 = r11v;
        g1 = r10v + (((u32)r4s + ((u32)r4s << 1)) << 2);
        g2 = arg0;
        g0 = geo_view_slot_word_probe((u32)g0, (u32)g1, (void *)(uintptr_t)g2);
        if ((u8)g0 != 0) {
            /* success @ 0x2b158 */
            u32 half = i960_ld_u16(I960_WORKRAM, arg1 + 2u, 0);
            i960_st_u16(I960_WORKRAM, arg1, 0, (u16)r4s);
            i960_st_u16(I960_WORKRAM, arg1 + 2u, 0, (u16)(half + (u32)r8s));
            g0 = (u32)r4s;
            return (u32)g0;
        }
        r5s += 1;
        if (r5s >= (i32)r6v)
            break;
    }

fail:
    i960_st_u16(I960_WORKRAM, arg1 + 2u, 0, (u16)0xffff);
    i960_st_u16(I960_WORKRAM, arg1, 0, (u16)0xffff);
    g0 = (u32)-1;
    return (u32)g0;
}
