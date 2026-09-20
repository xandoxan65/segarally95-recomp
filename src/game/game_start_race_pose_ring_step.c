/* Pose ring step @ 0x46160 — called every race slot3 frame.
 *
 * Appends the live cam/car block @ 0x213b40 (0x58 bytes) into the ring at
 * 0x51c400 when under the 0xa8b cap, then optionally mirrors cam + nearest
 * peer into the per-course buffer @ 0x217280 once 0x217288 is armed.
 *
 * source: disasm/maincpu/maincpu_046160_280.asm
 */
// @rom 0x46160 +0x258 game_start_race_pose_ring_step

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "i960_host.h"

#include <math.h>
#include <stdint.h>

static void copy_pose58(u32 dst, u32 src)
{
    u32 i;

    for (i = 0u; i < 0x50u; i += 4u)
        i960_st_u32(I960_ABS, dst, i, i960_ld_u32(I960_ABS, src, i));
    i960_st_u32(I960_ABS, dst, 0x50u, i960_ld_u32(I960_ABS, src, 0x50u));
    i960_st_u32(I960_ABS, dst, 0x54u, i960_ld_u32(I960_ABS, src, 0x54u));
}

void game_start_race_pose_ring_step(u32 arg0, u32 arg1, u32 arg2)
{
    u32 ring_ix;
    u32 ring_words;
    u32 off;
    u32 armed;
    u32 course;
    u16 cam_s;
    u32 tab;
    u32 peer_count;
    u32 best_ix;
    float best_d;
    u32 i;
    float cx, cz;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    ring_ix = i960_ld_u32(I960_WORKRAM, 0x217250, 0);
    ring_words = (u32)((i32)ring_ix >> 2);
    /* @0x46170: cmpibg (ix>>2), 0xa8b → skip append. */
    if (!((i32)ring_words > 0xa8b)) {
        off = ring_words * 0x58u;
        copy_pose58(0x51c400u + off, 0x213b40u);
        i960_st_u32(I960_WORKRAM, 0x217250, 0, ring_ix + 1u);
    }

    armed = i960_ld_u32(I960_WORKRAM, 0x217288, 0);
    if (armed == 0u) {
        u32 gen = i960_ld_u32(I960_WORKRAM, 0x21728c, 0);

        course = i960_host_race_course_index();
        cam_s = i960_ld_u16(I960_WORKRAM, 0x213b96, 0);
        /* @0x46210: ld 0x5e5010(gen*4)[course*16]. */
        tab = i960_ld_u32(I960_WORKRAM,
                          0x5e5010u + (gen << 2),
                          course << 4);
        if ((i32)(int16_t)cam_s < (i32)tab) {
            peer_count = i960_ld_u32(I960_WORKRAM, 0x2139c8, 0);
            best_d = i960_u32_to_f64(0x7f7fffffu);
            best_ix = 0u;
            cx = (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x213b40, 0));
            cz = (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x213b48, 0));

            /* @0x4623c: walk peers when count >= 1. */
            if ((i32)peer_count >= 1) {
                u32 peer_z = 0x213ba0u;
                u32 peer_x = peer_z - 8u;

                for (i = 1u; (i32)i < (i32)peer_count; i++) {
                    float ox, oz, dx, dz, dist;

                    ox = (float)i960_u32_to_f64(i960_ld_u32(I960_ABS, peer_x, 0));
                    oz = (float)i960_u32_to_f64(i960_ld_u32(I960_ABS, peer_z, 0));
                    dx = cx - ox;
                    dz = cz - oz;
                    dist = sqrtf(dx * dx + dz * dz);
                    if (dist < best_d) {
                        best_d = dist;
                        best_ix = i;
                    }
                    peer_z += 0x58u;
                    peer_x += 0x58u;
                }
            }
            i960_st_u32(I960_WORKRAM, 0x217290, 0, best_ix);
            if (best_ix != 0u)
                armed = armed + 1u;
        }
        i960_st_u32(I960_WORKRAM, 0x217288, 0, armed);
    }

    if (armed == 0u)
        return;

    {
        u32 cursor = i960_ld_u32(I960_WORKRAM, 0x217284, 0);
        u32 gen = i960_ld_u32(I960_WORKRAM, 0x21728c, 0);
        u32 best = i960_ld_u32(I960_WORKRAM, 0x217290, 0);
        u32 written = i960_ld_u32(I960_ABS, cursor, 0);
        u32 limit = (gen + 1u) * 0xfau;
        u32 words = (u32)((i32)written >> 2);
        u32 peer_src = 0x213b40u + best * 0x58u;
        u32 dest_base;
        u32 dst;

        /* @0x462e4: cmpi written, limit — bge → rollover path. */
        if ((i32)written >= (i32)limit) {
            i960_st_u32(I960_WORKRAM, 0x217288, 0, 0u);
            i960_st_u32(I960_WORKRAM, 0x21728c, 0, gen + 1u);
            return;
        }

        dest_base = i960_ld_u32(I960_WORKRAM, 0x217280, 0);
        dst = dest_base + words * 0x58u;
        copy_pose58(dst, 0x213b40u);

        /* @0x46350–0x46388: second record — peer at 0x213b40[best]. */
        dst = dest_base + (words + 0xe1u) * 0x58u;
        copy_pose58(dst, peer_src);

        i960_st_u32(I960_ABS, cursor, 0, written + 1u);
    }
}
