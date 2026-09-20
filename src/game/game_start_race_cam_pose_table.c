/* Race cam pose table fill @ 0x21CD8 — bal from cam object init @ 0x34140.
 *
 * g0 = &cam+0x14, g1 = &cam+0x38. lda 0x5dcb20(idx*12)[course*3*16] /
 * lda 0x5dcc70[course*3*4] are float-triple EAs (not pointer tables).
 *
 * source: disasm/maincpu/maincpu_021cd8_80.asm */
// @rom 0x21cd8 +0x78 game_start_race_cam_pose_table

#include "i960_lift.h"
#include "i960_mem.h"
#include "i960_host.h"

void game_start_race_cam_pose_table(u32 arg0, u32 arg1, u32 arg2)
{
    u32 dst14 = arg0;
    u32 dst38 = arg1;
    u32 idx;
    u32 course;
    u32 course3;
    u32 ea;

    (void)arg2;

    idx = i960_ld_u32(I960_WORKRAM, 0x2139c0, 0);
    if (idx == 1u)
        idx = 0u;
    else
        idx = i960_ld_u32(I960_WORKRAM, 0x2139c4, 0);

    /* lda (g4)[g4*2] → *3; shlo 2 → word offset idx*12. */
    idx = (idx + (idx << 1)) << 2;

    course = i960_host_race_course_index();
    course3 = course + (course << 1);

    /* lda 0x5dcb20(idx)[course3*16] → float triple EA. */
    ea = 0x005dcb20u + idx + (course3 << 4);
    i960_st_u32(I960_ABS, dst14, 0, i960_ld_u32(I960_ABS, ea, 0));
    i960_st_u32(I960_ABS, dst14, 4, i960_ld_u32(I960_ABS, ea, 4));
    i960_st_u32(I960_ABS, dst14, 8, i960_ld_u32(I960_ABS, ea, 8));

    course = i960_host_race_course_index();
    course3 = course + (course << 1);
    /* lda 0x5dcc70[course3*4] → float triple EA. */
    ea = 0x005dcc70u + (course3 << 2);
    i960_st_u32(I960_ABS, dst38, 0, i960_ld_u32(I960_ABS, ea, 0));
    i960_st_u32(I960_ABS, dst38, 4, i960_ld_u32(I960_ABS, ea, 4));
    i960_st_u32(I960_ABS, dst38, 8, i960_ld_u32(I960_ABS, ea, 8));
}
