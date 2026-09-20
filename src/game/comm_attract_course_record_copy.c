/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_03f220_8c.asm */
// @rom 0x3f220 +0x8c comm_attract_course_record_copy

#include "i960_lift.h"
#include "i960_mem.h"
#include "i960_host_staging.h"
#include "lift_syms.h"
#include "model2_rom.h"

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret  caller r5,r6  callee r4,r6 */
/* abi: void * arg0=g0, u32 arg1=g1, u32 arg2=g2 → void */

/*
 * Copy one ROM course-list record (src @ g0, 0x20-byte stride) into a freshly
 * allocated event slot (dst from chain_restore). Prior lift treated src as
 * dst and wrote +0x20/+0x5c into the next ROM-mirror records — that clobbered
 * ids and forced the course_init walk into Event Over Flow.
 */

void comm_attract_course_record_copy(void *arg0, u32 arg1, u32 arg2)
{
    u32 src_va;
    u32 dst_va;
    u32 tag;
    u32 handler;
    u32 w0;
    u32 w1;

    src_va = (u32)(uintptr_t)arg0;
    g1 = arg1;
    g2 = arg2;

    r4 = src_va;
    comm_attract_course_chain_restore(0, arg2);
    r6 = g0; /* cmpi/mov/be order @ 0x3F228 */
    if (g0 == 0)
        goto L_done;

    dst_va = (u32)g0;

    /* clrbit 2 on dst+2 */
    tag = i960_ld_u8(I960_WORKRAM, dst_va, 2);
    tag &= (u8)~(1u << 2);
    i960_st_u8(I960_WORKRAM, dst_va, 2, (u8)tag);

    /* src+4 low5 → scene handler table @ 0x5ddfc0 → dst+4; index → dst+0x10 */
    handler = i960_ld_u32(I960_ABS, src_va, 4) & 31u;
    handler = model2_workram_mirror_u32(0x5ddfc0u + (handler << 2));
    i960_st_u32(I960_WORKRAM, dst_va, 0x10, arg1);
    i960_st_u32(I960_WORKRAM, dst_va, 4, handler);

    /* id → dst+0x14 */
    i960_st_u32(I960_WORKRAM, dst_va, 0x14,
                i960_ld_u32(I960_ABS, src_va, 0));

    /* ldl 0x14(src) → stl 0x18(dst) */
    w0 = i960_ld_u32(I960_ABS, src_va, 0x14);
    w1 = i960_ld_u32(I960_ABS, src_va, 0x18);
    i960_st_u32(I960_WORKRAM, dst_va, 0x18, w0);
    i960_st_u32(I960_WORKRAM, dst_va, 0x1c, w1);

    i960_st_u32(I960_WORKRAM, dst_va, 0x20,
                i960_ld_u32(I960_ABS, src_va, 0x1c));

    /* g14 is 0 after restore; src+0x10 → dst+0x28 */
    i960_st_u32(I960_WORKRAM, dst_va, 0x2c, (u32)g14);
    i960_st_u32(I960_WORKRAM, dst_va, 0x24, (u32)g14);
    i960_st_u32(I960_WORKRAM, dst_va, 0x28,
                i960_ld_u32(I960_ABS, src_va, 0x10));

    i960_st_u32(I960_WORKRAM, dst_va, 0x5c,
                i960_ld_u32(I960_ABS, src_va, 8));
    i960_st_u32(I960_WORKRAM, dst_va, 0x60,
                i960_ld_u32(I960_ABS, src_va, 0xc));

    g0 = dst_va;
    if (handler != 0) {
        if (!i960_host_staging_call_lifted(handler))
            i960_call_indirect(handler);
    }

L_done:
    g0 = (u32)r6;
}
