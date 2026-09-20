/* Race object publish @ 0x22C70 — timer < 0 path from obj_bind @ 0x23110.
 *
 * cmpible 0,timer at 0x23110 branches to 0x22e20 when timer >= 0; fallthrough
 * calls here when timer < 0. Solo practice leaves timer = −1, so this path
 * must run — it stores 0x213b40 blocks into 0x213980[] (not m2comm).
 *
 * source: disasm/maincpu/maincpu_022c70_1b0.asm */
// @rom 0x22c70 +0x1a8 game_start_race_obj_publish

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>

void game_start_race_obj_sort_index(u32 arg0, u32 arg1, u32 arg2);
void game_start_race_obj_depth_pass(u32 arg0, u32 arg1, u32 arg2);

void game_start_race_obj_publish(u32 arg0, u32 arg1, u32 arg2)
{
    u32 count;
    u32 i;
    u32 slot_ea;
    u32 pose;
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    if (!logged) {
        lift_log( "lift: race_obj_publish (0x22c70 timer<0)\n");
        fflush(stderr);
        logged = 1;
    }

    /* @0x22C70: lda 0x30(sp),sp — private frame unused on host. */
    count = i960_ld_u32(I960_WORKRAM, 0x2139c8, 0);
    if (count != 0u) {
        slot_ea = 0x00213980u;
        pose = 0x00213b40u;
        for (i = 0; i < count; i++) {
            i960_st_u32(I960_WORKRAM, slot_ea, 0, pose);
            slot_ea += 4u;
            pose += 0x58u;
        }
    }

    /* @0x22CBC–0x22CD0: 213978=count; 214130/134 = 213b40, 0. */
    i960_st_u32(I960_WORKRAM, 0x213978, 0, count);
    i960_st_u32(I960_WORKRAM, 0x214130, 0, 0x00213b40u);
    i960_st_u32(I960_WORKRAM, 0x214134, 0, 0u);

    game_start_race_obj_sort_index(0, 0, 0);
    g0 = 1;
    game_start_race_obj_depth_pass(1, 0, 0);

    /* @0x22CE4: optional 0x213b40 → 0x20a5b8 mirror when 0x20a530 != 0. */
    if (i960_ld_u32(I960_WORKRAM, 0x20a530, 0) != 0u) {
        u32 src = 0x00213b40u;
        u32 dst = 0x0020a5b8u;
        u32 w;
        unsigned k;

        for (k = 0; k < 0x58u; k += 4u) {
            w = i960_ld_u32(I960_WORKRAM, src + k, 0);
            i960_st_u32(I960_WORKRAM, dst + k, 0, w);
        }

        /*
         * @0x22D5C–0x22E00: walk 0x2139f8 list copying 0x58-byte peers into
         * 0x20a610… when present. Solo practice leaves 0x2139f8 == 0.
         */
        if (i960_ld_u32(I960_WORKRAM, 0x2139f8, 0) != 0u) {
            u32 list = 0x002139f8u;
            u32 out = 0x0020a610u;
            u32 n = 1u;

            while (n <= 3u) {
                u32 peer = i960_ld_u32(I960_WORKRAM, list, 0);

                if (peer == 0u)
                    break;
                for (k = 0; k < 0x58u; k += 4u) {
                    w = i960_ld_u32(I960_ABS, peer + k, 0);
                    i960_st_u32(I960_WORKRAM, out + k, 0, w);
                }
                n++;
                list += 0x10u;
                out += 0x58u;
                if (n > 3u)
                    break;
                peer = i960_ld_u32(I960_WORKRAM, list, 0);
                if (peer == 0u)
                    break;
            }
            i960_st_u32(I960_WORKRAM, 0x20a5b4, 0, n);
        } else {
            i960_st_u32(I960_WORKRAM, 0x20a5b4, 0, 1u);
        }
    }
}
