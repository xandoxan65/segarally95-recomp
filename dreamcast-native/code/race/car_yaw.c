/* Car yaw step (0x2F3C0): the velocity turned into the car's own frame, its sideways part accumulated as slip.
 *
 * The TGP rotates (vel.x, 0, vel.z) by minus the yaw; the resulting z (along the car) is kept at node+0x80 and
 * summed into node+0x84. With the steering rate node+0xd0 above about 0.0346 rad and the speed scale node+0xb0
 * above about 0.37, node+0x74 counter-steers by -0.8 of it, else it is cleared. */
#include "race/car.h"
#include "race/fp.h"
#include "sr_tgp.h"

void sr_car_yaw_step(uint32_t node) {
    uint32_t in[3], t[3], d0;
    double ad;
    if(node == 0) return;

    sr_tgp_cmd(0x10002020u, 0, 0, 0, 0);
    sr_tgp_cmd(0x12802525u, 0, 0, 0, 0);
    in[0] = fneg(sr_ld32(node + CAR_ANG + 4));
    sr_tgp_cmd(0x15002a2au, in, 1, 0, 0);
    in[0] = sr_ld32(node + CAR_VEL);
    in[1] = 0;
    in[2] = sr_ld32(node + CAR_VEL + 8);
    sr_tgp_cmd(0x16002c2cu, in, 3, t, 3);
    sr_st32(node + 0x80, t[2]);
    sr_st32(node + 0x84, fadd(sr_ld32(node + 0x84), t[2]));
    sr_tgp_cmd(0x10802121u, 0, 0, 0, 0);

    d0 = sr_ld32(node + 0xd0);
    ad = D(d0);
    if(ad < 0.0) ad = -ad;
    if(ad > sr_d(0x5d0a56ecu, 0x3fa1dab3u) && D(sr_ld32(node + 0xb0)) > sr_d(0x0cda0316u, 0x3fd7b426u))
        sr_st32(node + 0x74, fneg(F(D(d0) * sr_d(0x9999999au, 0x3fe99999u))));
    else
        sr_st32(node + 0x74, 0);
}
