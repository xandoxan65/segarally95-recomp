/* Pool list unlink @ 0x2AAF0 — called from list_walk when node+4 == 0.
 *
 * g0 = node to unlink (walk passes next node's +8 / prev link). Relinks
 * neighbors, then bal free-list insert by low 2 bits of node byte0:
 *   0 → 0x2A928 (list A @ 0x20cab0)
 *   1 → 0x2A968 (list B @ 0x20cab4)
 *   else → return 0
 *
 * source: disasm/maincpu/maincpu_02aa90_200.asm @ 0x2AAF0 */
// @rom 0x2aaf0 +0x44 game_start_race_list_unlink
// @rom 0x2a928 +0x30 game_start_race_list_free_a
// @rom 0x2a968 +0x30 game_start_race_list_free_b

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

static void list_free_push(u32 head_ea, u32 node)
{
    u32 old;

    if (node == 0u) {
        g0 = 0;
        return;
    }
    old = i960_ld_u32(I960_WORKRAM, head_ea, 0);
    i960_st_u32(I960_ABS, node, 0xc, old);
    i960_st_u32(I960_WORKRAM, head_ea, 0, node);
    g0 = 1;
}

void game_start_race_list_free_a(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg1;
    (void)arg2;
    list_free_push(0x20cab0u, arg0 != 0u ? arg0 : (u32)g0);
}

void game_start_race_list_free_b(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg1;
    (void)arg2;
    list_free_push(0x20cab4u, arg0 != 0u ? arg0 : (u32)g0);
}

void game_start_race_list_unlink(u32 arg0, u32 arg1, u32 arg2)
{
    u32 node = arg0 != 0u ? arg0 : (u32)g0;
    u32 prev;
    u32 next;
    u32 kind;

    (void)arg1;
    (void)arg2;

    /* @0x2AAF0: cmpibne 0,g0 → body; else return 0. */
    if (node == 0u) {
        g0 = 0;
        return;
    }

    /* @0x2AAFC–0x2AB08: splice prev/next. */
    prev = i960_ld_u32(I960_ABS, node, 0x8);
    next = i960_ld_u32(I960_ABS, node, 0xc);
    i960_st_u32(I960_ABS, prev, 0xc, next);
    i960_st_u32(I960_ABS, next, 0x8, prev);

    kind = (u32)i960_ld_u8(I960_ABS, node, 0) & 3u;
    if (kind == 0u) {
        g0 = node;
        game_start_race_list_free_a(node, 0, 0);
        return;
    }
    if (kind == 1u) {
        g0 = node;
        game_start_race_list_free_b(node, 0, 0);
        return;
    }
    g0 = 0;
}
