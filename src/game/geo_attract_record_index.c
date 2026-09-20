/* Course-event window walk @ 0x3F410 — callx each live node in [idx_a, idx_b).
 *
 * g0 = span index (0x2140c8), g1/g2 = signed window from 0x5db310 (desert
 * −5/+25). lda 0x1(g0)[g2] is LEA idx_b = g0+g2+1, not a load.
 * For each r5: 0x215390[r5] head, then +0xc chain while +0x14 == r5.
 *
 * source: disasm/maincpu/maincpu_03f410_a0.asm */
// @rom 0x3f410 +0xa0 geo_attract_record_index

#include "i960_lift.h"
#include "i960_mem.h"
#include "i960_host_staging.h"

#include <stdio.h>

void geo_attract_record_index(u32 arg0, u32 arg1, u32 arg2)
{
    u32 head;
    u32 idx_a;
    u32 idx_b;
    u32 rec_va;
    u32 handler;
    u32 tag;
    u32 guard;
    static int logged;

    /* @0x3F410: cmpibg 0,g0 → ret when g0 < 0. */
    if ((i32)arg0 < 0) {
        if (!logged) {
            fprintf(stderr, "lift: record_index skip span=%d (negative)\n",
                    (int)arg0);
            fflush(stderr);
            logged = 1;
        }
        return;
    }

    head = i960_ld_u32(I960_WORKRAM, 0x215b60, 0);
    /* @0x3F41C: cmpobl g0,head — ret if g0 >= head. */
    if (arg0 >= head) {
        if (!logged) {
            fprintf(stderr,
                    "lift: record_index skip span=%u >= head=%u\n",
                    (unsigned)arg0, (unsigned)head);
            fflush(stderr);
            logged = 1;
        }
        return;
    }

    idx_a = arg0 + arg1;
    if ((i32)idx_a < 0)
        idx_a += head;

    /* @0x3F438: lda 0x1(g0)[g2] → g0 + g2 + 1. */
    idx_b = arg0 + arg2 + 1u;
    if ((i32)idx_b < 0)
        idx_b += head;

    if (idx_a >= head)
        idx_a -= head;
    if (idx_b >= head)
        idx_b -= head;

    if (!logged) {
        u32 rec0 = i960_ld_u32(I960_WORKRAM, 0x215390, 0);

        fprintf(stderr,
                "lift: record_index window [%u,%u) head=%u span=%u "
                "215390[0]=%#x tag0=%#x h4=%#x\n",
                (unsigned)idx_a, (unsigned)idx_b, (unsigned)head,
                (unsigned)arg0, (unsigned)rec0,
                rec0 ? (unsigned)i960_ld_u32(I960_ABS, rec0, 0x14) : 0u,
                rec0 ? (unsigned)i960_ld_u32(I960_ABS, rec0, 4) : 0u);
        fflush(stderr);
        logged = 1;
    }

    /* @0x3F4A0: while r5 != r6. */
    {
        unsigned callx = 0;

        while (idx_a != idx_b) {
            rec_va = i960_ld_u32(I960_WORKRAM, 0x215390, idx_a << 2);
            if (rec_va != 0u) {
                guard = 0;
                /* First node: require +0x14 == r5 before the callx chain. */
                tag = i960_ld_u32(I960_ABS, rec_va, 0x14);
                while (tag == idx_a && guard++ < 32u) {
                    handler = i960_ld_u32(I960_ABS, rec_va, 4);
                    if (handler != 0u) {
                        g0 = rec_va;
                        if (!i960_host_staging_call_lifted(handler))
                            i960_call_indirect(handler);
                        callx++;
                    }
                    rec_va = i960_ld_u32(I960_ABS, rec_va, 0xc);
                    if (rec_va == 0u)
                        break;
                    tag = i960_ld_u32(I960_ABS, rec_va, 0x14);
                }
            }
            idx_a += 1u;
            if (idx_a >= head)
                idx_a -= head;
        }
        if (logged == 1) {
            fprintf(stderr, "lift: record_index callx=%u\n", callx);
            fflush(stderr);
            logged = 2;
        }
    }
}
