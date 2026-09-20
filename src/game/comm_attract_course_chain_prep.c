/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_03f1b8_30.asm */
// @rom 0x3f1b8 +0x30 comm_attract_course_chain_prep

#include "i960_lift.h"
#include "i960_mem.h"

/* convention: kind=leaf_bx  args g0,g2,g1  link g14→g1 bx */
/* abi: void * arg0=g0, u32 arg1=g2 → void * g0 */

/*
 * Build the free event pool as a contiguous 0x300-slot chain of 0x7c-byte
 * records starting at arg0 (course_init passes 0x505000).
 *
 * Disasm uses `lda 0x7c(g0),g4` — LEA of the next slot, not a memory load.
 * Treating it as `ld` walked a null/garbage link and wrote through
 * `0xffffff90(g4)` into ROM-mirror course lists → Event Over Flow.
 */

void * comm_attract_course_chain_prep(void * arg0, u32 arg1)
{
    u32 chain_va;
    u32 next_va;
    i32 remain;

    (void)arg1;

    g1 = g14;
    g14 = 0;
    chain_va = (u32)(uintptr_t)arg0;
    remain = 0x2ff;
    do {
        next_va = chain_va + 0x7cu; /* lda 0x7c(g0) */
        remain--;
        i960_st_u32(I960_WORKRAM, chain_va, 0xc, next_va);
        chain_va = next_va;
    } while (remain >= 0);
    /* Null-terminate last node's +0xc (== 0xffffff90(next) after final bump). */
    i960_st_u32(I960_WORKRAM, next_va, 0xffffff90u, (u32)g14);
    g0 = chain_va;
    (void)g1;
    return (void *)(uintptr_t)g0;
}
