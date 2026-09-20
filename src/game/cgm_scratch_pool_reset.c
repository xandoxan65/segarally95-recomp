/* CGM scratch buffer-pool reset @ 0x2A7A0 (comm_attract_inner_2 animate path). */
/* source: decomp/disasm/maincpu/maincpu_02a5a0_2b0.asm, maincpu_02a800_100.asm */
// @rom 0x2a7a0 +0x58 cgm_scratch_pool_reset

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_memory.h"

#define CGM_POOL_CFG_A      0x20ca10u
#define CGM_POOL_CFG_B      0x20c970u
#define CGM_POOL_HEAD_A     0x20c978u
#define CGM_POOL_HEAD_B     0x20ca18u
#define CGM_POOL_CURSOR_A   0x20cab0u
#define CGM_POOL_CURSOR_B   0x20cab4u

#define CGM_POOL_NODE_COUNT 64u
#define CGM_POOL_LINK_OFF   0x0cu

static void cgm_scratch_pool_build_stride40(u32 pool_base)
{
    u32 cur;
    u32 next;
    u32 last;
    u32 count;

    cur = pool_base;
    last = pool_base;
    for (count = CGM_POOL_NODE_COUNT; count > 0u; count--) {
        /* lda 0x40(g0) — next node EA, not a load. */
        next = cur + 0x40u;
        i960_st_u32(I960_WORKRAM, cur, CGM_POOL_LINK_OFF, next);
        last = cur;
        cur = next;
    }
    /* st 0, 0xffffffcc(next) with next=last+0x40 → last+0xc null link. */
    i960_st_u32(I960_WORKRAM, last, CGM_POOL_LINK_OFF, 0);
}

static void cgm_scratch_pool_build_stride100(u32 pool_base)
{
    u32 cur;
    u32 next;
    u32 last;
    u32 count;

    cur = pool_base;
    last = pool_base;
    for (count = CGM_POOL_NODE_COUNT; count > 0u; count--) {
        /* lda 0x100(g0) — next node EA, not a load. */
        next = cur + 0x100u;
        i960_st_u32(I960_WORKRAM, cur, CGM_POOL_LINK_OFF, next);
        last = cur;
        cur = next;
    }
    /* st 0, 0xffffff0c(next) with next=last+0x100 → last+0xc null link. */
    i960_st_u32(I960_WORKRAM, last, CGM_POOL_LINK_OFF, 0);
}

void cgm_scratch_pool_reset(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x2A7A0–0x2A7E0: stl (0, 0x20ca10) @ 0x20c978; stl (0x20c970, 0) @ 0x20ca18. */
    i960_st_u32(I960_WORKRAM, CGM_POOL_HEAD_A, 0, 0);
    i960_st_u32(I960_WORKRAM, CGM_POOL_HEAD_A, 4, CGM_POOL_CFG_A);
    i960_st_u32(I960_WORKRAM, CGM_POOL_HEAD_B, 0, CGM_POOL_CFG_B);
    i960_st_u32(I960_WORKRAM, CGM_POOL_HEAD_B, 4, 0);
    i960_st_u32(I960_WORKRAM, CGM_POOL_CURSOR_A, 0, WORKRAM_BASE);
    i960_st_u32(I960_WORKRAM, CGM_POOL_CURSOR_B, 0, WORKRAM_BASE + 0x4000u);

    /* @0x2A7E8 bal 0x2A848 — stride-0x100 nodes @ WORKRAM_BASE */
    cgm_scratch_pool_build_stride100(WORKRAM_BASE);

    /* @0x2A7EC–0x2A7F4 bal 0x2A808 — stride-0x40 nodes @ WORKRAM_BASE+0x4000 */
    cgm_scratch_pool_build_stride40(WORKRAM_BASE + 0x4000u);
}
