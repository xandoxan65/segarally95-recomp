/* Select confirm helper @ 0x15030 — car-select (and related) gate.
 * Returns g0=1 when confirm may advance the scene slot; g0=0 to stay.
 * source: disasm/maincpu/maincpu_015030_1d0.asm */
// @rom 0x15030 +0x1c8 game_start_select_confirm

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_mem.h"

#include <stdio.h>
#include <string.h>

/*
 * Private host frame — ROM addo 16,sp with locals at 0x40(fp)[i].
 *
 * Game-start phase_0 seeds 0x20a560 = −1, so the early success path
 * (@ 0x151C4) is the normal single-player select-expiry case. The
 * multi-cabinet vote path is implemented enough to not false-fail when
 * 0x20a581 == 0 (no peers).
 */
u32 game_start_select_confirm(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    u8 frame[0x50];
    u32 board;
    u32 timer;
    u32 flag;
    u32 need;
    u32 matched;
    u32 i;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;

    board = i960_ld_u32(I960_WORKRAM, 0x20a530, 0);
    timer = i960_ld_u32(I960_WORKRAM, 0x20a560, 0);
    flag = i960_ld_u32(I960_WORKRAM, 0x20a758, 0);

    /* @0x1503C–0x15054: board==0 | timer<=0 | flag!=0 → success. */
    if (board == 0u || (i32)timer <= 0 || flag != 0u)
        goto success;

    need = (u32)i960_ld_u8(I960_WORKRAM, 0x20a581, 0);
    i960_st_u32(I960_WORKRAM, 0x20a564, 0, 5u);
    matched = 0u;

    /* @0x15070: need<=0 skips the peer-match loop (matched stays 0). */
    if ((i32)need > 0) {
        u32 base = 0x01a121dcu;
        u32 stride = 0u;

        matched = 0u;
        while ((i32)matched < (i32)need) {
            u32 slot_timer = i960_ld_u32(I960_ABS, base, 0);

            if (slot_timer == timer) {
                u32 kind_ea = 0x01a121d8u + stride;
                u32 kind_ptr = i960_ld_u32(I960_ABS, kind_ea, 0);
                u32 kind = i960_ld_u32(I960_ABS, kind_ptr, 0);

                if (kind != 5u)
                    break;
            }
            matched++;
            stride += 64u;
            base += 64u;
        }
    }

    need = (u32)i960_ld_u8(I960_WORKRAM, 0x20a581, 0);
    /* @0x150B8: matched count must equal need (both 0 is OK). */
    if (matched != need) {
        fp = fp_save;
        g0 = 0;
        return 0u;
    }

    for (i = 0u; i < 2u; i++)
        *(u32 *)(fp + 0x40u + i * 4u) = 0u;

    i960_st_u32(I960_WORKRAM, 0x2139c0, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x2139c4, 0, 0);

    /* Vote scan omitted when need==0 — falls through with g0=1. */
    if ((i32)need > 0) {
        u32 jp = (u32)i960_ld_u8(I960_WORKRAM, 0x20201b, 0);
        u32 span = ((need << 3) - need) << 6;
        u32 off;
        u32 best = 0u;

        for (off = 0u; off < span; off += 64u) {
            u32 slot_timer = i960_ld_u32(I960_ABS, 0x01a121dcu + off, 0);
            u16 score;
            u32 vote_ix;
            u32 *slot;
            u32 c;

            if (slot_timer != timer)
                continue;
            score = (u16)i960_ld_u16(I960_ABS, 0x01a121c2u + off, 0);
            if ((i32)(u32)score < (i32)jp) {
                c = i960_ld_u32(I960_WORKRAM, 0x2139c4, 0);
                i960_st_u32(I960_WORKRAM, 0x2139c4, 0, c + 1u);
            }
            vote_ix = i960_ld_u32(I960_ABS, 0x01a121e0u + off, 0);
            if (vote_ix > 1u)
                vote_ix = 1u;
            slot = (u32 *)(fp + 0x40u + vote_ix * 4u);
            c = i960_ld_u32(I960_WORKRAM, 0x2139c0, 0);
            i960_st_u32(I960_WORKRAM, 0x2139c0, 0, c + 1u);
            (*slot)++;
        }
        for (i = 0u; i < 2u; i++) {
            u32 votes = *(u32 *)(fp + 0x40u + i * 4u);

            if ((i32)votes > (i32)best) {
                best = votes;
                i960_st_u32(I960_WORKRAM, 0x214354, 0, i);
            }
        }
    }
    goto done_ok;

success:
    i960_st_u32(I960_WORKRAM, 0x2139c0, 0, 1u);
    i960_st_u32(I960_WORKRAM, 0x20a560, 0, (u32)-1);
    i960_st_u32(I960_WORKRAM, 0x2139c4, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x2139c8, 0, 1u);

done_ok:
    fp = fp_save;
    g0 = 1;
    lift_log( "lift: select_confirm ok course=%u\n",
            (unsigned)i960_ld_u32(I960_WORKRAM, 0x214354, 0));
    return 1u;
}
