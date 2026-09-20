/* Object depth / TGP span pass @ 0x23280 — called with g0=1 from publish.
 *
 * Clears the 0x213af8 scratch bank, then for each object in [start, count):
 *   Disasm @ 0x232E4: lda 0x213980[i*4] → slot EA; ld (slot) → g5 (pose VA).
 *   TGP 0x2c from (g5)/(g5+4)/(g5+8); desc+8 = g5. Float XYZ live at pose+0.
 * Epilogue writes +8=0 terminator and insertion-sorts by depth.
 *
 * source: disasm/maincpu/maincpu_023280_250.asm */
// @rom 0x23280 +0x250 game_start_race_obj_depth_pass

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_rom.h"

#include <stdio.h>
#include <string.h>

void game_start_race_obj_depth_pass(u32 arg0, u32 arg1, u32 arg2)
{
    u32 count;
    u32 start = arg0;
    u32 idx;
    u32 scratch;
    u32 obj;
    u32 xyz;
    u32 tx, ty, tz;
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    u8 frame[0x50];
    static int logged;

    (void)arg1;
    (void)arg2;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    sp = sp + 16;

    /* @0x23288–0x232A4: 18× st 0 walking back 16 from 0x213af8. */
    {
        u32 slot = 0x00213af8u;
        u32 n;

        for (n = 0; n < 18u; n++) {
            i960_st_u32(I960_WORKRAM, slot, 0, 0);
            slot -= 16u;
        }
    }

    count = i960_ld_u32(I960_WORKRAM, 0x213978, 0);
    scratch = 0x002139f0u;
    i960_st_u32(I960_WORKRAM, 0x2139ec, 0, 0);

    /* @0x232C4: cmpibge start,count → skip body. */
    if ((i32)start >= (i32)count) {
        if (!logged) {
            fprintf(stderr,
                    "lift: race_obj_depth_pass skip (g0=%u count=%u)\n",
                    (unsigned)start, (unsigned)count);
            fflush(stderr);
            logged = 1;
        }
        fp = fp_save;
        sp = sp_save;
        return;
    }

    if (!logged) {
        fprintf(stderr,
                "lift: race_obj_depth_pass body g0=%u count=%u\n",
                (unsigned)start, (unsigned)count);
        fflush(stderr);
        logged = 1;
    }

    for (idx = start; (i32)idx < (i32)count; idx++) {
        double depth_f;
        double y_f;
        u32 flag;

        obj = i960_ld_u32(I960_WORKRAM, 0x213980u + (idx << 2), 0);
        if (obj == 0u) {
            scratch += 16u;
            continue;
        }

        /*
         * @0x232E4–0x232F4: lda slot EA; ld (slot) → g5. XYZ and desc+8 both
         * use that pose VA (floats at +0/+4/+8) — no second pointer hop.
         */
        xyz = obj;

        i960_mmio_write_u32(0x884000, 0x16002c2cu);
        i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, xyz, 0));
        i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, xyz, 4));
        i960_mmio_write_u32(0x884000, i960_ld_u32(I960_ABS, xyz, 8));

        /* @0x23324–0x23348: FIFO readback → fp temps + scratch. */
        tx = i960_ld_u32(I960_MMIO, 0x884000, 0);
        ty = i960_ld_u32(I960_MMIO, 0x884000, 0);
        tz = i960_ld_u32(I960_MMIO, 0x884000, 0);

        i960_st_u32(I960_WORKRAM, scratch, 4, 0); /* mode */
        i960_st_u32(I960_WORKRAM, scratch, 8, xyz);
        i960_st_u32(I960_WORKRAM, scratch, 0, 0); /* flag */
        i960_st_u32(I960_WORKRAM, scratch, 12, tz);

        /*
         * @0x23394–0x23490: depth/y gates (cmprl vs 2.0 / 4.0 / 8.0 / −2.0).
         * Doubles: 0x40000000=2, 0x40100000=4, 0x40200000=8, 0xc0080000=−2,
         * 0x40080000=3 used as mul scale on the y<−2 branch.
         */
        depth_f = i960_u32_to_f64(tz);
        y_f = i960_u32_to_f64(ty);
        flag = 0;

        /* @0x233AC: depth > 2.0; @0x233D4: depth < 4.0 + 2139d8==0 → |x|<8 gate. */
        if (depth_f > 2.0) {
            if (depth_f < 4.0
                && i960_ld_u32(I960_WORKRAM, 0x2139d8, 0) == 0u) {
                double ax = i960_u32_to_f64(tx & 0x7fffffffu);

                flag = (ax < 8.0) ? 0u : 1u;
            } else {
                flag = 1u;
            }
            if (flag != 0u) {
                i960_st_u32(I960_WORKRAM, scratch, 12, tz);
                i960_st_u32(I960_WORKRAM, scratch, 0, 1u);
            }
        }

        /* @0x23440: y < 8.0; @0x23458: y > −2.0 → depth×3 into +12; always flag+=2. */
        if (y_f < 8.0) {
            if (y_f > -2.0) {
                i960_st_u32(I960_WORKRAM, scratch, 12,
                            (u32)i960_f64_to_u32(depth_f * 3.0));
            }
            {
                u32 f = i960_ld_u32(I960_WORKRAM, scratch, 0);
                i960_st_u32(I960_WORKRAM, scratch, 0, f + 2u);
            }
        }

        scratch += 16u;
    }

    /* @0x234B8: +8 = 0 terminator at the write cursor. */
    i960_st_u32(I960_WORKRAM, scratch, 8, 0);

    /*
     * @0x234C8–0x23530: insertion sort records in [0x2139f0, scratch) by +12.
     * Walk from 0x213a00 upward; skip when cursor == head (no body writes).
     */
    if (scratch != 0x002139f0u) {
        u32 cur = 0x00213a00u;
        u32 end = scratch;

        while (cur != end && cur < end) {
            u32 d_ins = i960_ld_u32(I960_WORKRAM, cur - 4u, 0);
            u32 w0 = i960_ld_u32(I960_WORKRAM, cur, 0);
            u32 w1 = i960_ld_u32(I960_WORKRAM, cur, 4);
            u32 w2 = i960_ld_u32(I960_WORKRAM, cur, 8);
            u32 w3 = i960_ld_u32(I960_WORKRAM, cur, 12);
            u32 dst = cur;
            u32 src = cur - 16u;

            while (src >= 0x002139f0u
                   && i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, src, 12))
                          > i960_u32_to_f64(d_ins)) {
                i960_st_u32(I960_WORKRAM, dst, 0,
                            i960_ld_u32(I960_WORKRAM, src, 0));
                i960_st_u32(I960_WORKRAM, dst, 4,
                            i960_ld_u32(I960_WORKRAM, src, 4));
                i960_st_u32(I960_WORKRAM, dst, 8,
                            i960_ld_u32(I960_WORKRAM, src, 8));
                i960_st_u32(I960_WORKRAM, dst, 12,
                            i960_ld_u32(I960_WORKRAM, src, 12));
                dst = src;
                if (src < 0x002139f0u + 16u)
                    break;
                src -= 16u;
            }
            i960_st_u32(I960_WORKRAM, dst, 0, w0);
            i960_st_u32(I960_WORKRAM, dst, 4, w1);
            i960_st_u32(I960_WORKRAM, dst, 8, w2);
            i960_st_u32(I960_WORKRAM, dst, 12, w3);
            cur += 16u;
        }
    }

    fp = fp_save;
    sp = sp_save;
}
