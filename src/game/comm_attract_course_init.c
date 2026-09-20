/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_03f2f0_110.asm */
// @rom 0x3f2f0 +0x110 comm_attract_course_init

#include "i960_lift.h"
#include "i960_mem.h"
#include "i960_host.h"
#include "lift_syms.h"
#include "model2_rom.h"

#include <stdio.h>

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret */
/* abi: u32 arg0=g0, u32 arg1=g1, u32 arg2=g2 → u32 g0 */

u32 comm_attract_course_init(u32 arg0, u32 arg1, u32 arg2)
{
    u32 course_idx;
    u32 list_ptr;
    u32 walk_va;
    u32 sentinel;
    i32 stack_i;

    (void)arg0;

    course_idx = i960_host_race_course_index();
    g0 = 0x00505000u;
    i960_st_u32(I960_WORKRAM, 0x215380, 0, (u32)g0);
    walk_va = model2_workram_mirror_u32(0x5de190u + (course_idx << 2));

    comm_attract_course_chain_prep((void *)(uintptr_t)g0, arg2);

    course_idx = i960_host_race_course_index();
    /*
     * @0x3F31C–0x3F33C:
     *   lda (g4)[g4*2] → course*3 (LEA, not a load)
     *   ld 0x5dcac8[that*4] → list head pointer
     *   lda 0x215b5c → stack walk EA (LEA)
     *   ld (g4) → list_ptr value
     */
    list_ptr = course_idx + (course_idx << 1);
    list_ptr = model2_workram_mirror_u32(0x5dcac8u + (list_ptr << 2));
    r7 = 0xfffe7961u;
    g5 = 0x215b5cu;
    list_ptr = i960_ld_u32(I960_ABS, list_ptr, 0);

    for (stack_i = 0x1f3; stack_i >= 0; stack_i--) {
        /* ROM stores g14 after chain_prep (g14=0). Host g14 may be live. */
        i960_st_u32(I960_WORKRAM, (u32)g5, 0, 0);
        g5 -= 4u;
    }

    sentinel = 0x1f4u;
    i960_st_u32(I960_WORKRAM, 0x215b60, 0, list_ptr);
    if (list_ptr <= sentinel)
        goto L_0003f380;

    fprintf(stderr,
            "lift: Generator Error — list_ptr=0x%x course=%u walk=0x%x\n",
            (unsigned)list_ptr, (unsigned)course_idx, (unsigned)walk_va);
    g0 = 0;
    g1 = 31u + 16u;
    tile_cursor_seed(0, (u32)g1);
    /* @0x3F370: lda 0x5de2b0 — printf format EA, not a load. */
    g0 = 0x005de2b0u;
    libc_printf((const char *)(uintptr_t)g0, (u32)g1, arg2);
    return (u32)g0;

    L_0003f380:
        r5 = 0;
        r8 = 0x2ffu;
        r9 = 0x1869fu;

    L_0003f390:
        r4 = i960_ld_u32(I960_ABS, walk_va, 0);
        if (r4 == r9)
            goto L_0003f3d0;
        if (r4 == r7)
            goto L_0003f3bc;
        comm_attract_course_record_copy((void *)(uintptr_t)walk_va, r5, arg2);
        r7 = r4;
        walk_va += 0x20u;
        i960_st_u32(I960_WORKRAM, 0x215390, r4 << 2, (u32)g0);
        goto L_0003f3e0;

    L_0003f3bc:
        comm_attract_course_record_copy((void *)(uintptr_t)walk_va, r5, arg2);
        walk_va += 0x20u;
        goto L_0003f3e0;

    L_0003f3d0:
        comm_attract_course_record_copy((void *)(uintptr_t)walk_va, r5, arg2);
        {
            static int logged;

            if (!logged) {
                fprintf(stderr,
                        "lift: course_init course=%u count=%u head=%u "
                        "215390[0]=%#x\n",
                        (unsigned)course_idx, (unsigned)r5, (unsigned)list_ptr,
                        (unsigned)i960_ld_u32(I960_WORKRAM, 0x215390, 0));
                fflush(stderr);
                logged = 1;
            }
        }
        return walk_va;

    L_0003f3e0:
        r5++;
        if ((signed int)r5 <= (signed int)r8)
            goto L_0003f390;
        fprintf(stderr,
                "lift: Event Over Flow — course=%u walk_end=0x%x count=%u\n",
                (unsigned)i960_ld_u32(I960_WORKRAM, 0x214354, 0),
                (unsigned)walk_va, (unsigned)r5);
        g0 = 0;
        g1 = 31u + 16u;
        tile_cursor_seed(0, (u32)g1);
        /* @0x3F3F4: lda 0x5de2d0 — printf format EA. */
        g0 = 0x005de2d0u;
        libc_printf((const char *)(uintptr_t)g0, (u32)g1, arg2);
        return (u32)g0;
}
