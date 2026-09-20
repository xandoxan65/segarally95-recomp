/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_03f1f8_24.asm */
// @rom 0x3f1f8 +0x24 comm_attract_course_chain_restore

#include "i960_lift.h"
#include "i960_mem.h"

/* convention: kind=leaf_bx  args g0,g2,g1  link g14→g1 bx */
/* abi: u32 arg0=g0, u32 arg1=g2 → void (result in g0) */

/*
 * Allocate one event from the free pool head @ 0x215380.
 * `lda 0x7c(g0),g4` bumps the head by one record size; g0 stays as the
 * allocated slot. Prior lift `ld`/`g0=next` returned the bumped pointer
 * (or 0) and broke record_copy destinations.
 */

void comm_attract_course_chain_restore(u32 arg0, u32 arg1)
{
    u32 next;

    (void)arg0;
    (void)arg1;

    g1 = g14;
    g14 = 0;
    g0 = i960_ld_u32(I960_WORKRAM, 0x215380, 0);
    next = (u32)g0 + 0x7cu; /* lda 0x7c(g0) */
    i960_st_u32(I960_WORKRAM, 0x215380, 0, next);
    g4 = next;
    (void)g1;
}
