/* Race sub-slot 1 @ 0x1CD20 — countdown / blend between slot0 and slot2.
 *
 * After slot0 advances 0x2020a8 by 2 (→2), race_frame keeps dispatching
 * table[1] until this leaf bumps the sub-slot to 4 (→ table[2]).
 *
 * Even 0x2020a8 (first hit): snapshot 0x20b0b0, clear frame counter
 * 0x2021f4, call 0x1E478(g0=0/1 from 0x20aea8), then 2020a8 += 1 (→odd).
 *
 * Odd 0x2020a8: while 0x2021f4 ≤ 60, lerp 0x20b0b0 toward the snapshot
 * over 60 frames (optionally call 0x26140); past 60, clear 0x20aea8 /
 * 0x20aeb4, bal 0x1E498, then 2020a8 += 1 (→4).
 *
 * source: disasm/maincpu/maincpu_01cd20_120.asm */
// @rom 0x1cd20 +0x110 game_start_race_slot1

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>

/* @0x1E478: st g0 → 0x20aeb4; return via saved link. */
static void race_slot1_store_aeb4(u32 value)
{
    i960_st_u32(I960_WORKRAM, 0x20aeb4, 0, value);
}

/* @0x1C7E8: clear 0x20aea8; return. */
static void race_slot1_clear_aea8(void)
{
    i960_st_u32(I960_WORKRAM, 0x20aea8, 0, 0);
}

void game_start_race_slot1(u32 arg0, u32 arg1, u32 arg2)
{
    u32 sub;
    u32 counter;
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    sub = i960_ld_u32(I960_WORKRAM, 0x2020a8, 0);

    if (!logged) {
        fprintf(stderr, "lift: race_slot1 sub=%u\n", (unsigned)sub);
        fflush(stderr);
        logged = 1;
    }

    /* @0x1CD20: bbs bit0 → odd (lerp / finish) path. */
    if ((sub & 1u) == 0u) {
        u32 span = i960_ld_u32(I960_WORKRAM, 0x20b0b0, 0);
        u32 gate = i960_ld_u32(I960_WORKRAM, 0x20aea8, 0);

        i960_st_u32(I960_WORKRAM, 0x2021f4, 0, 0);
        i960_st_u32(I960_WORKRAM, 0x20aad8, 0, span);
        race_slot1_store_aeb4(gate != 0u ? 1u : 0u);

        sub = i960_ld_u32(I960_WORKRAM, 0x2020a8, 0);
        i960_st_u32(I960_WORKRAM, 0x2020a8, 0, sub + 1u);
        return;
    }

    counter = i960_ld_u32(I960_WORKRAM, 0x2021f4, 0);

    /* @0x1CD7C: cmpible counter, 60 → blend path; else finish then fall through. */
    if ((i32)counter > 60) {
        race_slot1_clear_aea8();
        race_slot1_store_aeb4(0);
        game_start_race_hud_row_seed(0, 0, 0);

        sub = i960_ld_u32(I960_WORKRAM, 0x2020a8, 0);
        i960_st_u32(I960_WORKRAM, 0x2020a8, 0, sub + 1u);
        if (logged == 1) {
            fprintf(stderr, "lift: race_slot1 done sub→%u\n",
                    (unsigned)(sub + 1u));
            fflush(stderr);
            logged = 2;
        }
    } else if ((i32)counter <= 50) {
        /*
         * @0x1CDB4–0x1CE0C: blend only while counter ≤ 50 (addo 31,19).
         *   step = (0x20aea8 * counter) / 60
         *   half = 0x20b0b0 / 60
         *   0x20b0b0 = step + 0x20aad8
         *   q = (step + snap) / 60
         *   if q != half → call 0x26140(g0=0x94)
         */
        u32 aea8 = i960_ld_u32(I960_WORKRAM, 0x20aea8, 0);
        u32 snap = i960_ld_u32(I960_WORKRAM, 0x20aad8, 0);
        u32 cur = i960_ld_u32(I960_WORKRAM, 0x20b0b0, 0);
        u32 sixty = 60u;
        u32 step = (aea8 * counter) / sixty;
        i32 half = (i32)cur / (i32)sixty;
        u32 blended = step + snap;
        i32 q = (i32)blended / (i32)sixty;

        i960_st_u32(I960_WORKRAM, 0x20b0b0, 0, blended);
        if (q != half) {
            g0 = 0x94u;
            comm_palette_index_call(0x94u);
        }
    }

    /* @0x1CE10: always bump frame counter on the odd path. */
    counter = i960_ld_u32(I960_WORKRAM, 0x2021f4, 0);
    i960_st_u32(I960_WORKRAM, 0x2021f4, 0, counter + 1u);
}
