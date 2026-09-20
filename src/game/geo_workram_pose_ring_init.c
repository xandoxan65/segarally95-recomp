/* Pose-ring / replay buffer init @ 0x460E8 — BAL from race slot0.
 *
 * Clears ring counters, binds per-course cursor @ 0x217260[course] into
 * 0x217284, and sets the destination base 0x217280 = 0x556420 + course×0x9ab0.
 * Slot3's pose_ring_step @ 0x46160 writes 0x58-byte records through that base.
 *
 * ROM uses bal/bx(g0); host calls return normally.
 *
 * source: disasm/maincpu/maincpu_046098_150.asm (entry 0x460e8)
 */
// @rom 0x460e8 +0x70 geo_workram_pose_ring_init

#include "i960_lift.h"
#include "i960_mem.h"
#include "i960_host.h"

void geo_workram_pose_ring_init(u32 arg0, u32 arg1, u32 arg2)
{
    u32 course;
    u32 cursor;
    u32 dest;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    course = i960_host_race_course_index();
    i960_st_u32(I960_WORKRAM, 0x217250, 0, 0u);
    i960_st_u32(I960_WORKRAM, 0x217288, 0, 0u);
    i960_st_u32(I960_WORKRAM, 0x21728c, 0, 0u);
    i960_st_u32(I960_WORKRAM, 0x217254, 0, course);

    cursor = 0x217260u + (course << 2);
    i960_st_u32(I960_WORKRAM, 0x217284, 0, cursor);
    i960_st_u32(I960_ABS, cursor, 0, 0u);

    /* @0x46128–0x46150: dest = 0x556420 + course * 0x9ab0. */
    dest = 0x556420u + course * 0x9ab0u;
    i960_st_u32(I960_WORKRAM, 0x217280, 0, dest);
}
