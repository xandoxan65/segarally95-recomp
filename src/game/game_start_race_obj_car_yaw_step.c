/* Car yaw / lateral step @ 0x2F3C0 — first e8==0/1 callee from car_frame.
 *
 * After road_attach → pose_publish, n17 high bits are cleared so e8==0.
 * Practice therefore takes the e8==0 path (not the inline e8==3 body).
 *
 * TGP: push → I → 0x2a(−yaw) → 0x2c(vel.xz) → accumulate into node+0x84;
 * stash transformed point at +0x80; pop; optional +0x74 from |d0| gates.
 *
 * Private host frame for 0x40(fp) temps (disasm only does addo 16,sp).
 *
 * source: disasm/maincpu/maincpu_02f3c0_160.asm */
// @rom 0x2f3c0 +0x15c game_start_race_obj_car_yaw_step

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_hw.h"

#include <stdio.h>
#include <string.h>

static u32 f_neg(u32 bits)
{
    return bits ^ 0x80000000u;
}

void game_start_race_obj_car_yaw_step(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    u8 frame[0x50];
    u32 node = arg0;
    u32 yaw;
    u32 vx, vz;
    u32 tx, ty, tz;
    u32 slot84;
    u32 d0;
    static int logged;

    (void)arg1;
    (void)arg2;

    if (node == 0u)
        return;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    /* @0x2F3C0: addo 16,sp — host needs 0x40(fp) temps. */
    sp = sp + 0x10u;

    if (!logged) {
        fprintf(stderr, "lift: car_yaw_step node=%#x\n", node);
        fflush(stderr);
        logged = 1;
    }

    /* @0x2F3C4–0x2F430: push → I → 0x2a(−yaw@+0x3c) → 0x2c(+0x2c,0,+0x34). */
    i960_mmio_write_u32(0x884000, 0x10002020u);
    i960_mmio_write_u32(0x884000, 0x12802525u);
    yaw = i960_ld_u32(I960_ABS, node, 0x3c);
    i960_mmio_write_u32(0x884000, 0x15002a2au);
    i960_mmio_write_u32(0x884000, f_neg(yaw));
    vx = i960_ld_u32(I960_ABS, node, 0x2c);
    vz = i960_ld_u32(I960_ABS, node, 0x34);
    i960_mmio_write_u32(0x884000, 0x16002c2cu);
    i960_mmio_write_u32(0x884000, vx);
    i960_mmio_write_u32(0x884000, 0);
    i960_mmio_write_u32(0x884000, vz);
    tx = i960_mmio_read_u32(0x884000);
    ty = i960_mmio_read_u32(0x884000);
    tz = i960_mmio_read_u32(0x884000);

    /* @0x2F43C–0x2F45C: +0x84 += tz; stash tz at +0x80. */
    slot84 = node + 0x84u;
    {
        u32 cur = i960_ld_u32(I960_ABS, slot84, 0);

        i960_st_u32(I960_ABS, node, 0x80, tz);
        i960_st_u32(I960_ABS, slot84, 0,
                    (u32)i960_f64_to_u32(i960_u32_to_f64(cur)
                                        + i960_u32_to_f64(tz)));
    }
    i960_mmio_write_u32(0x884000, 0x10802121u);

    /* Snapshot xform into 0x40(fp) (disasm keeps tx/ty/tz across gates). */
    memcpy(frame + 0x40, &tx, 4);
    memcpy(frame + 0x44, &ty, 4);
    memcpy(frame + 0x48, &tz, 4);

    /*
     * @0x2F470–0x2F51C: |d0@+0xd0| gates → +0x74 = −(d0×0.8) or 0.
     * Thresholds: rifl 0x5d0a56ec/0x3fa1dab3 and 0x0cda0316/0x3fd7b426;
     * scale rifl 0x9999999a/0x3fe99999. Disasm keeps signed d0 in fp1 across
     * the abs compare, then mulrl + notbit31 on the product high word.
     */
    d0 = i960_ld_u32(I960_ABS, node, 0xd0);
    {
        double ad = i960_u32_to_f64(d0);

        if (ad < 0.0)
            ad = -ad;
        if (ad > i960_rifl_read(0x5d0a56ecu, 0x3fa1dab3u)
            && i960_u32_to_f64(i960_ld_u32(I960_ABS, node, 0xb0))
                   > i960_rifl_read(0x0cda0316u, 0x3fd7b426u)) {
            u32 bits = (u32)i960_f64_to_u32(
                i960_u32_to_f64(d0)
                * i960_rifl_read(0x9999999au, 0x3fe99999u));

            i960_st_u32(I960_ABS, node, 0x74, f_neg(bits));
        } else {
            i960_st_u32(I960_ABS, node, 0x74, 0);
        }
    }

    fp = fp_save;
    sp = sp_save;
}
