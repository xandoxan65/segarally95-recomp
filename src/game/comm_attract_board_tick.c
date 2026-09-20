/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: decomp/disasm/maincpu/maincpu_016540_600.asm */
// @rom 0x16540 +0x5d8 comm_attract_board_tick

#include "i960_lift.h"
#include "i960_fp.h"
#include "model2_rom.h"
#include "i960_mem.h"
#include "lift_syms.h"

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret */
/* abi: u32 arg0=g0, u32 arg1=g1, u32 arg2=g2 → u32 g0 */
/* call site: comm_attract_inner_9 @ 0x13810 */

u32 comm_attract_board_tick(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    g4 = i960_ld_u32(I960_WORKRAM, 0x20a530, 0);
    if ((unsigned char)g4 != 3)
        goto L_00016584;
    /* board == 3: slot @ 0x20a970, g0 = 4 */
    g4 = i960_ld_u32(I960_WORKRAM, 0x202008, 0);
    g4 = g4 & 0x3fu;
    r5 = 47; /* addo 31,16 */
    g2 = i960_ld_u32(I960_WORKRAM, 0x20a970, 0);
    g0 = 4;
    if (g4 > (u32)r5)
        goto L_000165c8;
    goto L_000165a8;

    L_00016584:
        /* board != 3: slot @ 0x20a974, g0 = 45 */
        g4 = i960_ld_u32(I960_WORKRAM, 0x202008, 0);
        g4 = g4 & 0x3fu;
        r5 = 47;
        g2 = i960_ld_u32(I960_WORKRAM, 0x20a974, 0);
        g0 = 45; /* addo 31,14 */
        if (g4 > (u32)r5)
            goto L_000165c8;

    L_000165a8:
        g1 = 2;
        g3 = 0;
        g4 = 0;
        draw_scene_dispatch((u32)g0, (u32)g1, (u32)g2);
        goto L_000165d8;

    L_000165c8:
        g1 = 2;
        g3 = 0;
        g4 = 0;
        erase_scene_dispatch((u32)g0, (u32)g1, (u32)g2);

    L_000165d8:
        g4 = i960_ld_u32(I960_WORKRAM, 0x20a530, 0);
        if ((unsigned char)g4 == 3)
            goto L_0001661c;
        g4 = i960_ld_u8(I960_WORKRAM, 0x202018, 0);
        if ((unsigned char)g4 == 0)
            goto L_0001661c;
        g4 = i960_ld_u32(I960_WORKRAM, 0x20a97c, 0);
        g4 = g4 + 1;
        r5 = 0xbb8; /* lda 0xbb8 */
        i960_st_u32(I960_WORKRAM, 0x20a97c, 0, (u32)g4);
        if (g4 != (u32)r5)
            goto L_0001661c;
        g0 = 0x9c; /* lda 0x9c */
        comm_palette_index_call((u32)g0);
        i960_st_u32(I960_WORKRAM, 0x20a97c, 0, (u32)g14);

    L_0001661c:
        g5 = i960_ld_u32(I960_WORKRAM, 0x20a95c, 0);
        g4 = g5 << 3;
        g4 = g4 - g5;
        g4 = g4 << 6;
        g5 = i960_ld_u16(I960_ABS, 0x1a1223cu + (u32)g4, 0);
        g4 = i960_ld_u32(I960_ABS, 0x1a121d8u + (u32)g4, 0);
        g5 = g5 << 16;
        r4 = (uintptr_t)((i32)g5 >> 16);
        if ((unsigned char)g4 != 6)
            goto L_00016704;
        g4 = i960_ld_u8(I960_WORKRAM, 0x20a980, 0);
        if ((unsigned char)g4 == 0)
            goto L_00016704;
        g4 = i960_ld_u32(I960_WORKRAM, 0x20a960, 0);
        if ((signed char)r4 >= (signed char)g4)
            goto L_00016704;
        g4 = i960_ld_u32(I960_WORKRAM, 0x20a968, 0);
        r5 = 0x3e7;
        if ((signed char)g4 > (signed char)r5)
            goto L_00016704;
        g5 = i960_ld_u32(I960_WORKRAM, 0x20a958, 0);
        g4 = i960_ld_u32(I960_ABS, (u32)g5, 0x10);
        g0 = 0;
        if ((signed char)g4 >= (signed char)r4)
            goto L_000166a0;
        do {
            g5 = g5 + 0x20;
            g4 = i960_ld_u32(I960_ABS, (u32)g5, 0x10);
            g0 = g0 + 1;
        } while ((signed char)g4 < (signed char)r4);

    L_000166a0:
        i960_call_rom(0x17010);
    g4 = i960_ld_u32(I960_WORKRAM, 0x20a968, 0);
    g5 = i960_ld_u32(I960_WORKRAM, 0x20a964, 0);
    g4 = g4 + 1;
    i960_st_u32(I960_WORKRAM, 0x20a968, 0, (u32)g4);
    if ((unsigned char)g5 != 1)
        goto L_00016ad4;
    g4 = i960_ld_u32(I960_WORKRAM, 0x20227c, 0);
    if ((unsigned char)g4 != 0)
        goto L_00016ad4;
    i960_st_u32(I960_WORKRAM, 0x213844, 0, (u32)g14);
    r5 = 4;
    i960_st_u32(I960_WORKRAM, 0x21384c, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x213840, 0, (u32)r5);
    i960_st_u32(I960_WORKRAM, 0x20a964, 0, (u32)g14);
    g0 = 0xe166;
    tile_attract_palram_gate((u32)g0);
    goto L_00016ad4;

    L_00016704:
        g4 = i960_ld_u32(I960_WORKRAM, 0x20a530, 0);
        i960_st_u8(I960_WORKRAM, 0x20a980, 0, (u8)g14);
        if ((unsigned char)g4 != 3)
            goto L_00016774;
        g6 = i960_ld_u8(I960_WORKRAM, 0x20a581, 0);
        g0 = 0;
        if ((signed char)g6 <= 0)
            goto L_00016848;
        g5 = 0x1a121d8u;
        g7 = i960_ld_u32(I960_WORKRAM, 0x20a584, 0);

    L_00016738:
        if ((unsigned char)g7 == g0)
            goto L_00016744;
        g4 = i960_ld_u32(I960_ABS, (u32)g5, 0);
        if ((unsigned char)g4 == 6)
            goto L_00016758;

    L_00016744:
        g0 = g0 + 1;
        g5 = g5 + 0x1c0;
        if (g0 < g6)
            goto L_00016738;
        goto L_00016848;

    L_00016758:
        r5 = 1;
        i960_st_u8(I960_WORKRAM, 0x20a980, 0, (u8)r5);
        goto L_00016848;

    L_00016768:
        i960_st_u32(I960_WORKRAM, 0x20a96c, 0, (u32)g0);
        goto L_00016990;

    L_00016774:
        g2 = i960_ld_u8(I960_WORKRAM, 0x20a581, 0);
        g0 = 0;
        if ((signed char)g2 <= 0)
            goto L_00016848;
        g3 = 0x1a121dcu;
        r4 = 0;
        g13 = g3 - 4;
        r5 = 1;
        r7 = 7 << 6;

    L_0001679c:
        g4 = i960_ld_u32(I960_WORKRAM, 0x20a584, 0);
        if ((unsigned char)g4 == g0)
            goto L_00016830;
        g4 = i960_ld_u32(I960_ABS, (u32)g13, 0);
        if ((unsigned char)g4 != 6)
            goto L_00016830;
        g4 = i960_ld_u32(I960_ABS, 0x1a121dcu + (u32)r4, 0);
        if (g4 <= 0)
            goto L_00016830;
        g5 = g0 + 1;
        /* cmpibge g5,g2 → skip inner match loop when g5 >= count */
        if ((signed char)g5 >= (signed char)g2)
            goto L_00016824;
        g4 = g5 << 3;
        g4 = g4 - g5;
        g6 = g4 << 6;
        g4 = g2 << 3;
        g4 = g4 - g2;
        g1 = g4 << 6;
        g7 = 0x1a121d8u + (u32)g6;
        r6 = 7 << 6;

    L_000167ec:
        g4 = i960_ld_u32(I960_ABS, (u32)g7, 0);
        if ((unsigned char)g4 != 6)
            goto L_00016814;
        g5 = i960_ld_u32(I960_ABS, 0x1a121dcu + (u32)g6, 0);
        g4 = i960_ld_u32(I960_ABS, (u32)g3, 0);
        if ((unsigned char)g4 != g5)
            goto L_00016814;
        i960_st_u8(I960_WORKRAM, 0x20a980, 0, (u8)r5);
        goto L_00016824;

    L_00016814:
        g6 = g6 + r6;
        g7 = g7 + r6;
        if (g6 < g1)
            goto L_000167ec;

    L_00016824:
        g4 = i960_ld_u8(I960_WORKRAM, 0x20a980, 0);
        if ((unsigned char)g4 == 1)
            goto L_00016848;

    L_00016830:
        g0 = g0 + 1;
        g3 = g3 + r7;
        r4 = r4 + r7;
        g13 = g13 + r7;
        if (g0 < g2)
            goto L_0001679c;

    L_00016848:
        g4 = i960_ld_u8(I960_WORKRAM, 0x20a980, 0);
        if ((unsigned char)g4 == 0)
            goto L_00016ad4;
        g4 = i960_ld_u32(I960_WORKRAM, 0x20a96c, 0);
        g5 = i960_ld_u8(I960_WORKRAM, 0x20a581, 0);
        g4 = g4 + 1;
        i960_st_u32(I960_WORKRAM, 0x20a95c, 0, (u32)g4);
        if ((u32)g5 == (u32)g4)
            i960_st_u32(I960_WORKRAM, 0x20a95c, 0, (u32)g14);
        g13 = i960_ld_u32(I960_WORKRAM, 0x20a584, 0);
        g3 = i960_ld_u8(I960_WORKRAM, 0x20a581, 0);
        goto L_00016954;

    L_00016890:
        g5 = i960_ld_u32(I960_WORKRAM, 0x20a95c, 0);
        g4 = i960_ld_u32(I960_WORKRAM, 0x20a96c, 0);
        if ((unsigned char)g4 != g5)
            goto L_00016934;
        i960_st_u8(I960_WORKRAM, 0x20a980, 0, (u8)g14);
        goto L_00016990;

    L_000168b0:
        g0 = i960_ld_u32(I960_WORKRAM, 0x20a95c, 0);
        if ((unsigned char)g13 == g0)
            goto L_00016890;
        g5 = g0 << 3;
        g5 = g5 - g0;
        g5 = g5 << 6;
        g4 = i960_ld_u32(I960_ABS, 0x1a121d8u + (u32)g5, 0);
        if ((unsigned char)g4 != 6)
            goto L_00016890;
        g2 = 0x1a121dcu + (u32)g5;
        g4 = i960_ld_u32(I960_ABS, (u32)g2, 0);
        if (g4 <= 0)
            goto L_00016890;
        /* cmpibge 0,g3 → skip when count <= 0 */
        if ((signed char)g3 <= 0)
            goto L_00016934;
        g4 = g3 << 3;
        g4 = g4 - g3;
        g1 = g4 << 6;
        g6 = 0;
        g7 = 0x1a121d8u;

    L_00016904:
        g4 = i960_ld_u32(I960_ABS, (u32)g7, 0);
        if ((unsigned char)g4 != 6)
            goto L_00016920;
        g5 = i960_ld_u32(I960_ABS, 0x1a121dcu + (u32)g6, 0);
        g4 = i960_ld_u32(I960_ABS, (u32)g2, 0);
        if ((unsigned char)g4 == g5)
            goto L_00016768;

    L_00016920:
        g4 = 7 << 6;
        g6 = g6 + g4;
        g7 = g7 + g4;
        if (g6 < g1)
            goto L_00016904;

    L_00016934:
        g4 = i960_ld_u32(I960_WORKRAM, 0x20a95c, 0);
        g4 = g4 + 1;
        i960_st_u32(I960_WORKRAM, 0x20a95c, 0, (u32)g4);
        if ((unsigned char)g3 == g4) {
            i960_st_u32(I960_WORKRAM, 0x20a95c, 0, (u32)g14);
        }

    L_00016954:
        g4 = i960_ld_u32(I960_WORKRAM, 0x20a530, 0);
        if ((unsigned char)g4 != 3)
            goto L_000168b0;
        g5 = i960_ld_u32(I960_WORKRAM, 0x20a95c, 0);
        if ((unsigned char)g13 == g5)
            goto L_00016890;
        g4 = g5 << 3;
        g4 = g4 - g5;
        g4 = g4 << 6;
        g4 = i960_ld_u32(I960_ABS, 0x1a121d8u + (u32)g4, 0);
        if ((unsigned char)g4 != 6)
            goto L_00016890;
        i960_st_u32(I960_WORKRAM, 0x20a96c, 0, (u32)g5);

    L_00016990:
        g4 = i960_ld_u8(I960_WORKRAM, 0x20a980, 0);
        if ((unsigned char)g4 == 0)
            goto L_00016ad4;
        g5 = i960_ld_u32(I960_WORKRAM, 0x20a95c, 0);
        i960_st_u32(I960_WORKRAM, 0x20a968, 0, (u32)g14);
        g4 = g5 << 3;
        g4 = g4 - g5;
        g4 = g4 << 6;
        g5 = i960_ld_u16(I960_ABS, 0x1a1223cu + (u32)g4, 0);
        g6 = i960_ld_u32(I960_WORKRAM, 0x214354, 0);
        g7 = 0x1a121e0u + (u32)g4;
        g4 = i960_ld_u32(I960_ABS, (u32)g7, 0);
        g5 = g5 << 16;
        r4 = (uintptr_t)((i32)g5 >> 16);
        if ((unsigned char)g4 == g6)
            goto L_00016a40;
        r5 = 0 | (1u << 8);
        i960_st_u32(I960_WORKRAM, 0x213844, 0, (u32)r5);
        r5 = 4;
        i960_st_u32(I960_WORKRAM, 0x21384c, 0, (u32)g14);
        i960_st_u32(I960_WORKRAM, 0x213840, 0, (u32)r5);
        g4 = i960_ld_u32(I960_ABS, (u32)g7, 0);
        i960_st_u32(I960_WORKRAM, 0x214354, 0, (u32)g4);
        g4 = i960_ld_u32(I960_WORKRAM, 0x214354, 0);
        g0 = model2_workram_mirror_u32(0x5dccf0u + (g4 << 2));
        g1 = 1;
        i960_st_u32(I960_WORKRAM, 0x214350, 0, (u32)g0);
        texture_bank_select((u32)g0, (u32)g1, (u32)g2);
        r5 = 1;
        i960_st_u32(I960_WORKRAM, 0x20a964, 0, (u32)r5);
        comm_attract_course_init((u32)g0, (u32)g1, (u32)g2);

    L_00016a40:
        g4 = i960_ld_u32(I960_WORKRAM, 0x214354, 0);
        g5 = model2_workram_mirror_u32(0x5dccd0u + (g4 << 2));
        i960_st_u32(I960_WORKRAM, 0x20a958, 0, (u32)g5);
        g4 = i960_ld_u32(I960_ABS, (u32)g5, 0x10);
        g0 = 0;
        if ((signed char)g4 >= (signed char)r4)
            goto L_00016a74;
        do {
            g5 = g5 + 0x20;
            g4 = i960_ld_u32(I960_ABS, (u32)g5, 0x10);
            g0 = g0 + 1;
        } while ((signed char)g4 < (signed char)r4);

    L_00016a74:
        g4 = i960_ld_u32(I960_WORKRAM, 0x214354, 0);
        g5 = model2_workram_mirror_u32(0x5b54b0u + (g4 << 2));
        g4 = g0 + 4;
        if ((signed char)g5 > (signed char)g4)
            goto L_00016ab4;
        g4 = i960_ld_u32(I960_WORKRAM, 0x214354, 0);
        g4 = model2_workram_mirror_u32(0x5b54b0u + (g4 << 2));
        g5 = i960_ld_u32(I960_WORKRAM, 0x20a958, 0);
        g4 = g4 << 5;
        g4 = i960_ld_u32(I960_ABS, (u32)g5 + (u32)g4, 0x10);
        goto L_00016ac8;

    L_00016ab4:
        g4 = i960_ld_u32(I960_WORKRAM, 0x20a958, 0);
        g5 = g0 << 5;
        g4 = i960_ld_u32(I960_ABS, (u32)g4 + (u32)g5, 0x90);

    L_00016ac8:
        i960_st_u32(I960_WORKRAM, 0x20a960, 0, (u32)g4);
        i960_call_rom(0x17010);

    L_00016ad4:
        g4 = i960_ld_u8(I960_WORKRAM, 0x20a980, 0);
        if ((unsigned char)g4 != 0)
            goto L_00016b18;
        g0 = 0xffff;
        tile_attract_palram_gate((u32)g0);
        g4 = i960_ld_u32(I960_WORKRAM, 0x20a530, 0);
        if ((unsigned char)g4 == 3)
            goto L_00016b04;
        i960_st_u32(I960_WORKRAM, 0x20209c, 0, (u32)g14);
        return (u32)g0;

    L_00016b04:
        g4 = i960_ld_u32(I960_WORKRAM, 0x20a978, 0);
        g4 = g4 + 1;
        i960_st_u32(I960_WORKRAM, 0x20a978, 0, (u32)g4);

    L_00016b18:
        return (u32)g0;
}
