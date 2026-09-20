/* Race HUD ranking-list seed @ 0x47330 — slot0 passes g0 = &0x20aca0.
 *
 * Picks course/mode row-count + table bases from ROM mirror @ 0x5e55xx,
 * writes 0x2020c4 = row count, then fills (ptr, next) pairs at g0 into a
 * list terminated by (0, −1). Without this, leftover 0x2020c4 draws dozens
 * of bogus leaderboard rows.
 *
 * source: disasm/maincpu/maincpu_047330_200.asm */
// @rom 0x47330 +0x150 game_start_race_rank_list_seed

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_mem.h"
#include "i960_host.h"
#include "model2_rom.h"

#include <stdio.h>

/* Copy (a,b) pairs from src_a/src_b until peek of next *src_a is < 0. */
static void copy_pair_list(u32 *dst_a, u32 *dst_b, u32 src_a, u32 src_b)
{
    u32 a = *dst_a;
    u32 b = *dst_b;
    u32 peek;

    for (;;) {
        i960_st_u32(I960_WORKRAM, a, 0, model2_workram_mirror_u32(src_a));
        i960_st_u32(I960_WORKRAM, b, 0, model2_workram_mirror_u32(src_b));
        src_a += 4u;
        peek = model2_workram_mirror_u32(src_a);
        src_b += 4u;
        a += 8u;
        b += 8u;
        if ((i32)peek < 0)
            break;
    }
    *dst_a = a;
    *dst_b = b;
}

void game_start_race_rank_list_seed(u32 arg0, u32 arg1, u32 arg2)
{
    u32 dest = arg0 != 0u ? arg0 : (u32)g0;
    u32 mode;
    u32 course;
    u32 view;
    u32 rows;
    u32 tab_b;
    u32 tab_a;
    u32 idx;
    u32 off;
    u32 cursor_a;
    u32 cursor_b;
    u32 i;
    static int logged;

    (void)arg1;
    (void)arg2;

    if (!logged) {
        lift_log( "lift: race_rank_list_seed dest=%#x\n", dest);
        fflush(stderr);
        logged = 1;
    }

    if (dest == 0u)
        return;

    mode = i960_ld_u32(I960_WORKRAM, 0x202230, 0);
    course = i960_host_race_course_index();
    view = i960_ld_u8(I960_WORKRAM, 0x20201d, 0);
    view = (view << 5) & (3u << 5);

    if (mode == 0u) {
        rows = model2_workram_mirror_u32(0x005e5570u + view + course * 4u);
        tab_b = model2_workram_mirror_u32(0x005e5cf0u + course * 4u);
    } else {
        /* Practice / non-champ. */
        rows = model2_workram_mirror_u32(0x005e55f0u + view + course * 4u);
        tab_b = model2_workram_mirror_u32(0x005e6310u + course * 4u);
    }

    i960_st_u32(I960_WORKRAM, 0x2020c4, 0, rows);

    /* g1 = dest; g0 = dest+4 (interleaved a/b pair slots). */
    cursor_a = dest;
    cursor_b = dest + 4u;

    course = course & 3u;
    off = i960_ld_u8(I960_WORKRAM, 0x20201c, 0);
    off = (off << 2) & 12u;
    tab_a = 0x005e5670u + (course << 5);
    /* lda (g5)[g7*16] then (g2)[g4*4] → tab_b + (off + rows*16)*4. */
    idx = off + rows * 16u;
    copy_pair_list(&cursor_a, &cursor_b, tab_a, tab_b + idx * 4u);

    /* Extra passes for rows 1..rows-1 with 20201c-scaled offset into tab_b. */
    if ((i32)rows > 1) {
        for (i = 1u; (i32)i < (i32)rows; i++) {
            u32 v = i960_ld_u8(I960_WORKRAM, 0x20201c, 0);

            v = (v << 4) & 0x30u;
            copy_pair_list(&cursor_a, &cursor_b, tab_a, tab_b + v);
        }
    }

    /* Sentinel: st g14,(g1) / st −1,4(g1) — g14 is 0 on this call path. */
    i960_st_u32(I960_WORKRAM, cursor_a, 0, 0);
    i960_st_u32(I960_WORKRAM, cursor_a, 4, (u32)-1);

    if (logged == 1) {
        lift_log( "lift: race_rank_list rows=%u\n", (unsigned)rows);
        fflush(stderr);
        logged = 2;
    }
}
