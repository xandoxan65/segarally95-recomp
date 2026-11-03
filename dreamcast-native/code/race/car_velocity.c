/* Car velocity step (0x30E00): steer the horizontal velocity towards the road point ahead and ease the speed
 * towards the target node+0xac, with the grip pair 0x20cae4/e8 (set from the speed by car_frame) as response. */
#include "race/car.h"
#include "race/fp.h"
#include "sr_tgp.h"

static void tgp0(uint32_t cmd) { sr_tgp_cmd(cmd, 0, 0, 0, 0); }
static void tgp1(uint32_t cmd, uint32_t a) { sr_tgp_cmd(cmd, &a, 1, 0, 0); }

void sr_car_velocity_step(uint32_t node) {
    uint32_t probe[3], unit[3], old_unit[3], speed, response, distance, blend;
    uint32_t old_velocity[3];
    if(node == 0) return;
    if(!(D(fadd(sr_ld32(node + 0x64), sr_ld32(node + 0x68))) > 0.0)) return;

    /* A point 1.25 ahead and 0.4 below the car, in world space. */
    tgp0(0x10002020u);
    tgp0(0x12802525u);
    {
        const uint32_t pos[3] = {sr_ld32(node + CAR_POS), sr_ld32(node + CAR_POS + 4), sr_ld32(node + CAR_POS + 8)};
        sr_tgp_cmd(0x13802727u, pos, 3, 0, 0);
    }
    tgp1(0x15002a2au, sr_ld32(node + CAR_ANG + 4));
    tgp1(0x14802929u, sr_ld32(node + CAR_ANG));
    tgp1(0x15802b2bu, sr_ld32(node + CAR_ANG + 8));
    {
        static const uint32_t local[3] = {0, 0xbecccccdu, 0x3fa00000u};
        sr_tgp_cmd(0x16002c2cu, local, 3, probe, 3);
    }
    tgp0(0x10802121u);

    /* Direction from there to the road point at the lateral offset. */
    {
        const uint32_t delta[3] = {fsub(sr_ld32(node + CAR_ROAD_XZ), probe[0]), 0,
                                   fsub(sr_ld32(node + CAR_ROAD_XZ + 8), probe[2])};
        sr_st32(node + 0x9c, delta[0]);
        sr_st32(node + 0xa0, 0);
        sr_st32(node + 0xa4, delta[2]);
        sr_tgp_cmd(0x17802f2fu, delta, 3, unit, 3);
        sr_st32(node + 0x9c, unit[0]);
        sr_st32(node + 0xa0, unit[1]);
        sr_st32(node + 0xa4, unit[2]);
    }

    speed = F(D(sr_ld32(node + 0xac)) * sr_d(0xd714cf45u, 0x3f72f684u));
    old_velocity[0] = sr_ld32(node + CAR_VEL);
    old_velocity[1] = 0;
    old_velocity[2] = sr_ld32(node + CAR_VEL + 8);
    if(D(speed) <= D(sr_ld32(node + 0xb0))) {
        sr_st8(node + CAR_FLAGS10, sr_ld8(node + CAR_FLAGS10) & ~0x10u);
        response = sr_ld32(0x20cae4u);
    } else {
        sr_st8(node + CAR_FLAGS10, sr_ld8(node + CAR_FLAGS10) | 0x10u);
        response = sr_ld32(0x20cae8u);
    }
    {
        const uint32_t in[6] = {0, 0, 0, old_velocity[0], old_velocity[1], old_velocity[2]};
        sr_tgp_cmd(0x2c005858u, in, 6, &distance, 1);
    }
    speed = fsub(speed, fmul(response, fsub(speed, distance)));
    sr_st32(node + 0xb0, speed);

    sr_tgp_cmd(0x17802f2fu, old_velocity, 3, old_unit, 3);
    {
        const double dot = D(sr_ld32(node + 0x9c)) * D(old_unit[0]) + D(sr_ld32(node + 0xa4)) * D(old_unit[2]);
        double amount = 1.0 - dot;
        if(amount > 1.0) amount = 1.0;
        blend = F(amount);
        old_unit[0] = fadd(old_unit[0], fmul(fsub(sr_ld32(node + 0x9c), old_unit[0]), blend));
        old_unit[2] = fadd(old_unit[2], fmul(fsub(sr_ld32(node + 0xa4), old_unit[2]), blend));
    }
    sr_tgp_cmd(0x17802f2fu, old_unit, 3, unit, 3);
    sr_st32(node + CAR_VEL, fmul(unit[0], speed));
    sr_st32(node + CAR_VEL + 8, fmul(unit[2], speed));
}
