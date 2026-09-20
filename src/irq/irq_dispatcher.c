/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_000018d8_d8.asm */
// @rom 0x18d8 +0xd8 irq_dispatcher

#include "i960_lift.h"
#include "i960_mem.h"

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret */
/* abi: u32 arg0=g0, u32 arg1=g1, u32 arg2=g2 → void */

/* pointers: fp=u32 *, g4=u32 */

extern void irq_mask_helper(u32 arg0, u32 arg1, u32 arg2);
extern void irq_timer_arm(u32 arg0, u32 arg1, u32 arg2);
extern void irq_dispatcher(u32 arg0, u32 arg1, u32 arg2);

void irq_dispatcher(u32 arg0, u32 arg1, u32 arg2)
{
    g0 = (uintptr_t)arg0;
    g1 = (uintptr_t)arg1;
    g2 = (uintptr_t)arg2;

    L_000018d8:
        g4 = i960_ld_u32(I960_MMIO, 0xe80000, 0); /* irq_request */;
        *(u32 *)(fp + 0x40) = (u32)g4;
        g4 = *(u32 *)(fp + 0x40);
        g4 = g4 & r8;
        /* lift: cmpibe 0, g4, 0x1990 @ 0x18ec */
        g4 = *(u32 *)(fp + 0x40);
        if (!((g4 >> 2) & 1))
            goto L_00001904;
        g0 = 4;
        irq_timer_arm(g0, g1, g2);
    goto L_000018d8;

    L_00001904:
        g4 = *(u32 *)(fp + 0x40);
        if (!((g4 >> 3) & 1))
            goto L_00001918;
        g0 = 8;
        irq_timer_arm(g0, g1, g2);
    goto L_000018d8;

    L_00001918:
        g4 = *(u32 *)(fp + 0x40);
        if (!((g4 >> 4) & 1))
            goto L_0000192c;
        g0 = 16;
        irq_timer_arm(g0, g1, g2);
    goto L_000018d8;

    L_0000192c:
        g4 = *(u32 *)(fp + 0x40);
        if (!((g4 >> 5) & 1))
            goto L_00001940;
        g0 = 31 + 1;
        irq_timer_arm(g0, g1, g2);
    goto L_000018d8;

    L_00001940:
        g4 = *(u32 *)(fp + 0x40);
        if (!((g4 >> 6) & 1))
            goto L_00001954;
        /* lift: setbit 6, 0, g0 @ 0x1948 */
        irq_mask_helper(g0, g1, g2);
    goto L_000018d8;

    L_00001954:
        g4 = *(u32 *)(fp + 0x40);
        if (!((g4 >> 7) & 1))
            goto L_00001968;
        /* lift: setbit 7, 0, g0 @ 0x195c */
        irq_mask_helper(g0, g1, g2);
    goto L_000018d8;

    L_00001968:
        g4 = *(u32 *)(fp + 0x40);
        if (!((g4 >> 8) & 1))
            goto L_0000197c;
        /* lift: setbit 8, 0, g0 @ 0x1970 */
        irq_mask_helper(g0, g1, g2);
    goto L_000018d8;

    L_0000197c:
        g4 = *(u32 *)(fp + 0x40);
        if (!((g4 >> 9) & 1))
            goto L_000018d8;
        /* lift: setbit 9, 0, g0 @ 0x1984 */
        irq_mask_helper(g0, g1, g2);
    goto L_000018d8;
    g0 = r4;
    g1 = r5;
    g4 = r9;
    g5 = r10;
    g6 = r11;
    g7 = r12;
    g13 = r13;
    g14 = r14;
    return;
}
