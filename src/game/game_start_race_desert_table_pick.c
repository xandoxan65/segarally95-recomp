/* Desert race table base pick @ 0x41B78 — bal leaf.
 *
 * Sets g0 from 0x5e0b10 / 0x5e0b30 / 0x5e0b50 by practice flag (0x202230),
 * timer (0x20a560), and course index (0x214354). Returns via bx (saved g14).
 *
 * source: /tmp/dasm_41b78/maincpu_041b78_80.asm */
// @rom 0x41b78 +0x5c game_start_race_desert_table_pick

#include "i960_lift.h"
#include "i960_mem.h"
#include "i960_host.h"

u32 game_start_race_desert_table_pick(u32 arg0, u32 arg1, u32 arg2)
{
    u32 course;
    u32 base;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    g14 = 0;

    if (i960_ld_u32(I960_WORKRAM, 0x202230, 0) == 0u) {
        course = i960_host_race_course_index();
        base = i960_ld_u32(I960_WORKRAM, 0x5e0b10, course << 2);
    } else if ((i32)i960_ld_u32(I960_WORKRAM, 0x20a560, 0) >= 0) {
        course = i960_host_race_course_index();
        base = i960_ld_u32(I960_WORKRAM, 0x5e0b50, course << 2);
    } else {
        course = i960_host_race_course_index();
        base = i960_ld_u32(I960_WORKRAM, 0x5e0b30, course << 2);
    }

    g0 = base;
    return base;
}
