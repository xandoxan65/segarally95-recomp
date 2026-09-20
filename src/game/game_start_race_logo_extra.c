/* Race logo-path companion @ 0x420F0 — after obj_seed on START.
 *
 * Country==5: seed 0x216a58/5c/60 then course centroid @ 0x41E70.
 * Desert (0x214354==0): alloc_link node, install staged handler 0x5E2790
 * (ROM 0x43790), fill 0x216a88..0x21716c with −1.5f, then centroid.
 * Else: alloc_link with null handler at +4.
 *
 * source: disasm/maincpu/maincpu_0420f0_200.asm */
// @rom 0x420f0 +0x90 game_start_race_logo_extra

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_mem.h"
#include "i960_host.h"

#include "lift_syms.h"

#include <stdio.h>

void game_start_race_logo_extra(u32 arg0, u32 arg1, u32 arg2)
{
    u32 country;
    u32 course;
    u32 node;
    u32 cursor;
    u32 limit;
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x420F0: ldob 0x20201b; cmpibne 5 → course paths. */
    country = i960_ld_u8(I960_WORKRAM, 0x20201b, 0);
    if (country == 5u) {
        i960_st_u32(I960_WORKRAM, 0x216a58, 0, (u32)g14);
        i960_st_u32(I960_WORKRAM, 0x216a5c, 0, (u32)g14);
        i960_st_u32(I960_WORKRAM, 0x216a60, 0, 0xd5u);
        game_start_race_course_centroid(0, 0, 0);
        return;
    }

    course = i960_host_race_course_index();
    if (course != 0u) {
        /* @0x42170–0x42180: alloc_link; store node; +4 = g14 (0). */
        cgm_scratch_pool_alloc_link(0, 0, 0);
        node = (u32)g0;
        i960_st_u32(I960_WORKRAM, 0x217170, 0, node);
        i960_st_u32(I960_WORKRAM, node, 4, (u32)g14);
        return;
    }

    /* @0x4212C–0x42168: desert — handler 0x5E2790 + float fill + centroid. */
    cgm_scratch_pool_alloc_link(0, 0, 0);
    node = (u32)g0;
    i960_st_u32(I960_WORKRAM, 0x217170, 0, node);
    /* Staged callx target → ROM 0x43790 (desert node init). */
    i960_st_u32(I960_WORKRAM, node, 4, 0x005e2790u);

    cursor = 0x216a88u;
    limit = 0x21716cu;
    while ((i32)cursor <= (i32)limit) {
        i960_st_u32(I960_WORKRAM, cursor, 0, 0xbfc00000u); /* −1.5f */
        cursor += 28u;
    }

    game_start_race_course_centroid(0, 0, 0);

    if (!logged) {
        lift_log( "lift: race_logo_extra desert node=%#x\n", (unsigned)node);
        fflush(stderr);
        logged = 1;
    }
}
