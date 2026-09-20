/* Race course centroid table fill @ 0x41E70 — from logo_extra desert/country5.
 *
 * source: disasm/maincpu/maincpu_041e70_200.asm */
// @rom 0x41e70 +0x100 game_start_race_course_centroid

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "i960_host.h"
#include "model2_rom.h"

static u32 point_at(u32 base, u32 idx)
{
    /* @0x41EC4–0x41ED0: lda (idx)[idx*2]; lda (base)[that*4] → triple EA. */
    u32 scaled = idx + (idx << 1);

    return base + (scaled << 2);
}

void game_start_race_course_centroid(u32 arg0, u32 arg1, u32 arg2)
{
    u32 course;
    u32 list;
    i32 count;
    u32 base;
    u32 walk;
    u32 out_z;
    u32 out_x;
    u32 limit;
    double scale;
    double acc;
    u32 p0, p1, p2, p3;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    course = i960_host_race_course_index();
    list = course + (course << 1);
    list = model2_workram_mirror_u32(0x5dcac8u + (list << 2));
    /* @0x41E84 lda 0(g4),g4 — EA nop; structure is at list. */
    count = (i32)i960_ld_u32(I960_WORKRAM, list, 0);
    base = i960_ld_u32(I960_WORKRAM, list, 4);
    walk = i960_ld_u32(I960_WORKRAM, list, 8);

    if (count < 0)
        goto done;

    out_z = 0x215c48u;
    out_x = 0x215c40u; /* subo 8, g3 */
    scale = i960_rifl_read(0, 0x3fd00000u); /* 0.25 */
    limit = 0x215c48u + ((u32)count * 12u);

    while ((i32)out_z < (i32)limit) {
        u32 i0 = i960_ld_u16(I960_ABS, walk, 4);
        u32 i1 = i960_ld_u16(I960_ABS, walk, 6);
        u32 i2 = i960_ld_u16(I960_ABS, walk, 8);
        u32 i3 = i960_ld_u16(I960_ABS, walk, 0xa);

        p0 = point_at(base, i0);
        p1 = point_at(base, i1);
        p2 = point_at(base, i2);
        p3 = point_at(base, i3);

        acc = i960_u32_to_f64(i960_ld_u32(I960_ABS, p0, 0));
        acc += i960_u32_to_f64(i960_ld_u32(I960_ABS, p1, 0));
        acc += i960_u32_to_f64(i960_ld_u32(I960_ABS, p2, 0));
        acc += i960_u32_to_f64(i960_ld_u32(I960_ABS, p3, 0));
        i960_st_u32(I960_WORKRAM, out_x, 0, (u32)i960_f64_to_u32(acc * scale));

        acc = i960_u32_to_f64(i960_ld_u32(I960_ABS, p0, 8));
        acc += i960_u32_to_f64(i960_ld_u32(I960_ABS, p1, 8));
        acc += i960_u32_to_f64(i960_ld_u32(I960_ABS, p2, 8));
        acc += i960_u32_to_f64(i960_ld_u32(I960_ABS, p3, 8));
        i960_st_u32(I960_WORKRAM, out_z, 0, (u32)i960_f64_to_u32(acc * scale));

        out_z += 12u;
        out_x += 12u;
        walk += 12u;
    }

done:
    i960_st_u32(I960_WORKRAM, 0x216a50, 0, (u32)count);
}
