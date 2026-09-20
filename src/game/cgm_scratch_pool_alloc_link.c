/* CGM scratch pool: pop freelist-A + link into list-B @ 0x2AA90.
 *
 * logo_path (and 0x420F0) call this after cgm_scratch_pool_reset; g0 becomes
 * the allocated 0x100-byte node stored at 0x2140cc as the race cam object.
 *
 * source: disasm/maincpu/maincpu_02aa90_200.asm,
 *         maincpu_02a888_180.asm, maincpu_02a9f8_80.asm */
// @rom 0x2aa90 +0xc cgm_scratch_pool_alloc_link

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_memory.h"

#define CGM_POOL_CFG_A      0x20ca10u
#define CGM_POOL_HEAD_B     0x20ca18u
#define CGM_POOL_CURSOR_A   0x20cab0u
#define CGM_POOL_LINK_OFF   0x0cu
#define CGM_POOL_PREV_OFF   0x08u

/* @0x2A888: pop freelist head @ 0x20cab0; clear low 2 flag bits. */
static u32 cgm_scratch_pool_pop_a(void)
{
    u32 node = i960_ld_u32(I960_WORKRAM, CGM_POOL_CURSOR_A, 0);
    u32 next;
    u8 flags;

    if (node == 0u)
        return 0u;

    next = i960_ld_u32(I960_WORKRAM, node, CGM_POOL_LINK_OFF);
    i960_st_u32(I960_WORKRAM, CGM_POOL_CURSOR_A, 0, next);
    flags = i960_ld_u8(I960_WORKRAM, node, 0);
    flags = (u8)(flags & (u8)~3u);
    i960_st_u8(I960_WORKRAM, node, 0, flags);
    return node;
}

/* @0x2A9F8: insert node at tail of list-B (sentinel @ 0x20ca10). */
static void cgm_scratch_pool_link_b(u32 node)
{
    u32 prev;
    u32 sent;

    if (node == 0u)
        return;

    prev = i960_ld_u32(I960_WORKRAM, CGM_POOL_HEAD_B, 0);
    sent = CGM_POOL_CFG_A;
    i960_st_u32(I960_WORKRAM, node, CGM_POOL_LINK_OFF, sent);
    i960_st_u32(I960_WORKRAM, node, CGM_POOL_PREV_OFF, prev);
    i960_st_u32(I960_WORKRAM, prev, CGM_POOL_LINK_OFF, node);
    i960_st_u32(I960_WORKRAM, CGM_POOL_HEAD_B, 0, node);
}

void cgm_scratch_pool_alloc_link(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    node = cgm_scratch_pool_pop_a();
    cgm_scratch_pool_link_b(node);
    g0 = node;
}
