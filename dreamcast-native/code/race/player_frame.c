/* The player car's per-tick update (geo_view_frame_update @ 0x37870), called by geo_view_scene_frame for the
 * scene node at 0x500000 (race/player_update.h).
 *
 * Picks the dynamics row from the surface bits 0x214208, runs the engine and gearbox (matrix_prep), splits the
 * drive force over the wheel pairs (node+0x64..0x70), turns the slip 0x21420c and the lateral term 0x214214 into
 * the forces r5/r7 and r4/r8 limited by the grip circles (sqrt_mix A/B), drives the steering wheel force and the
 * skid sound, then pushes the forces through the TGP: a1 (caller frame +0x120) is rotated into the car's frame,
 * a2 (+0x150) accumulates the body forces.
 *
 * Float rules: single ops are fadd/fsub/fmul/fdiv; where the original keeps a product chain in double (the 1.1,
 * 0.3, 0.66, 0.635, 1.25, 0.2, 1000.0 and 2pi/3 constants are doubles) the port computes it in double and rounds
 * once (D()/F()). Comparisons the original makes between a float and a double constant use the float threshold
 * that gives the same answer (noted at each). Every comparison keeps the original's sense, so NaN takes the same
 * branch.
 *
 * Host reference details reproduced: the lifted version works on a private scratch frame (the hardware uses the
 * caller's fp+0x40..0x6c), so nothing is written to the caller's frame beyond a1/a2; g14 is 0 for the whole call,
 * so every "st g14" is a store of 0. */
#include "race/player_update.h"
#include "race/player_input.h"
#include "race/fp.h"
#include "sr_tgp.h"
#include <math.h>

#define PI_D    3.141592653589793
#define TWOPI_D (2.0 * 3.141592653589793)
#define HALFPI_D 1.5707963267948966
#define HALFPI_F 0x3fc90fdbu

/* Dynamics row copied from the table 0x5d67a0 (13 words). */
#define ROW_TABLE    0x5d67a0u
#define ROW_COPY     0x2141c0u /* words 0..11 */
#define ROW_W12      0x2141f0u
#define ROW_FRONT    0x2141c0u /* front pair scale */
#define ROW_REAR     0x2141c4u /* rear pair scale */
#define ROW_AIR_K    0x2141c8u
#define ROW_BAL_POS  0x2141e4u
#define ROW_BAL_NEG  0x2141e8u

#define SURFACE_BITS 0x214208u
#define SLIP         0x21420cu
#define SLIP_K       0x214210u
#define LAT          0x214214u
#define LAT_K        0x214218u
#define SKID_LEVEL_V 0x214254u
#define DRIVE_FORCE  0x214258u
#define BALANCE      0x21425cu
#define AIRBORNE     0x214260u
#define ROW_SEL      0x214264u

static float ldf(uint32_t va) { return sr_f(sr_ld32(va)); }

static uint32_t tgp_sin(uint32_t a) {
    uint32_t out;
    sr_tgp_cmd(0x0a801515u, &a, 1, &out, 1);
    return out;
}

/* |x| <= pi/2 ? (float)|x| : pi/2f, the angle both force paths send to the TGP sine. */
static uint32_t sin_arg(double x) {
    const double a = x >= 0.0 ? x : -x;
    return a <= HALFPI_D ? F(a) : HALFPI_F;
}

/* (1 + [0x214254]) * sqrt(a*a + b*b), the grip circle. */
static uint32_t grip_limit(uint32_t a, uint32_t b) {
    const uint32_t sum = fadd(fmul(a, a), fmul(b, b));
    return F((1.0 + D(sr_ld32(SKID_LEVEL_V))) * (double)sqrtf(sr_f(sum)));
}

/* Below a slip of 0.02 ([k] * |s|) the force fades out linearly. */
static uint32_t small_slip_fade(uint32_t s, uint32_t k_va, uint32_t force) {
    const uint32_t k = sr_ld32(k_va);
    const uint32_t p = sr_f(s) >= 0.f ? fmul(s, k) : fmul(k, fneg(s));
    if(sr_f(p) < sr_f(0x3ca3d70au)) return fmul(fdiv(p, 0x3ca3d70au), force);
    return force;
}

void sr_player_frame_update(uint32_t node, uint32_t a1[3], uint32_t a2[3], uint32_t corr) {
    uint32_t row, p, i, r4, r5, r6, r7, r8, r13, r14, g0, g8, prev_bal, s, gp;

    /* Dynamics row: the highest set bit 9..1 of the surface bits (bit 0 is never tested). */
    row = 9;
    while(!((sr_ld32(SURFACE_BITS) >> row) & 1u) && --row > 0) {
    }
    p = sr_ld32(ROW_TABLE + row * 4u);
    for(i = 0; i < 12; i++) sr_st32(ROW_COPY + i * 4u, sr_ld32(p + i * 4u));
    sr_st32(ROW_W12, sr_ld32(p + 0x30u));

    sr_st32(DRIVE_FORCE, sr_player_matrix_prep(node, SKID_LEVEL_V));

    /* Load on the front (+0x64/+0x68) and rear (+0x6c/+0x70) wheel pairs, and their side-to-side shares. */
    r5 = fadd(sr_ld32(node + 0x68u), sr_ld32(node + 0x64u));
    r7 = fadd(sr_ld32(node + 0x70u), sr_ld32(node + 0x6cu));
    prev_bal = sr_ld32(BALANCE);
    r14 = sr_f(r5) != 0.f ? F(1.1 * D(fsub(sr_ld32(node + 0x68u), sr_ld32(node + 0x64u))) / D(r5)) : 0;
    r13 = sr_f(r7) != 0.f ? F(1.1 * D(fsub(sr_ld32(node + 0x70u), sr_ld32(node + 0x6cu))) / D(r7)) : 0;

    if(sr_f(fmul(r7, r5)) != 0.f) {
        /* On the ground: the drive force splits into r6 (scaled by 0.2, or the row's factor when 0x2020b4 is 0)
         * and r8; r4 is the airborne gate's kick against the slip. 0x2020b4 is 0 in all checked drives, so the 0.2
         * factor is not checked. */
        const uint32_t k = sr_ld32(0x2020b4u) != 0 ? 0x3e4ccccdu : sr_ld32(ROW_AIR_K);
        uint32_t gate, drive;
        sr_st32(AIRBORNE, 0);
        gate = sr_player_airborne_gate(1);
        drive = sr_ld32(DRIVE_FORCE);
        r6 = fmul(drive, k);
        r4 = fmul(gate, fneg(sr_ld32(SLIP)));
        r8 = fsub(drive, r6);
    } else {
        sr_st32(AIRBORNE, 1);
        r4 = sr_player_airborne_gate(0);
        r8 = 0;
        r6 = 0;
    }

    /* Front/rear balance 0x21425c from the load difference, eased by the row's gains and the sin curve. */
    g8 = F(1.0 + 0.3 * D(sr_ld32(0x20b0b4u)));
    {
        const uint32_t sum = fadd(r7, r5);
        if(sr_f(sum) != 0.f) {
            const uint32_t q = fdiv(fsub(r5, r7), sum);
            const uint32_t d = fsub(q, prev_bal);
            const uint32_t gain = !(sr_f(d) <= 0.f) ? sr_ld32(ROW_BAL_POS) : sr_ld32(ROW_BAL_NEG);
            uint32_t m;
            sr_st32(BALANCE, q);
            m = fsub(q, fmul(gain, d));
            sr_st32(BALANCE, m);
            m = sr_player_sin_mix(m);
            sr_st32(BALANCE, m);
            r5 = F((1.0 + D(m)) * D(sr_ld32(ROW_FRONT)));
            r7 = F((1.0 - D(m)) * D(sr_ld32(ROW_REAR)));
        } else {
            sr_st32(BALANCE, 0);
        }
    }

    /* Slip dead zone [0x5d6690]: below it the slip is 0, above it shrinks towards 0 by the dead zone. */
    {
        const uint32_t slip = sr_ld32(SLIP), dz = sr_ld32(0x5d6690u);
        const uint32_t mag = sr_f(slip) < 0.f ? fneg(slip) : slip;
        if(!(sr_f(mag) >= sr_f(dz)))
            sr_st32(SLIP, 0);
        else
            sr_st32(SLIP, sr_f(slip) >= 0.f ? fsub(slip, dz) : fsub(slip, fneg(dz)));
    }

    /* Front force from the slip: sin(2pi/3 * slip) of the front load. */
    s = sr_ld32(SLIP);
    gp = sr_ld32(0x214124u);
    g0 = F(D(g8) * (D(r5) * (1000.0 * D(gp))));
    {
        const uint32_t sn = tgp_sin(sin_arg(TWOPI_D * D(s) / 3.0));
        corr = fadd(fmul(g0, fmul(sn, r4)), corr);
        r5 = fmul(g0, sr_f(s) >= 0.f ? fneg(sn) : sn);
    }
    {
        const uint32_t lim = grip_limit(r6, r5);
        if(!(sr_f(lim) <= sr_f(g0))) {
            g0 = sr_player_sqrt_mix(fdiv(g0, lim), SR_PLAYER_SQRT_MIX_A);
            r5 = F((1.1 * D(g0)) * D(r5));
            r6 = fmul(g0, r6);
        }
    }
    r5 = small_slip_fade(sr_ld32(SLIP), SLIP_K, r5);

    /* [0x20b0b4] >= 0.023148148148148147 (double); 0x3cbda12f is the largest float below it. */
    if(!(sr_f(sr_ld32(0x20b0b4u)) > sr_f(0x3cbda12fu)))
        sr_player_steer_feedback(g0);
    else
        sr_player_force_feedback(sr_ld32(node + 0x74u), fadd(r5, corr), sr_ld32(SLIP));
    if((int32_t)sr_ld32(0x214120u) == 2) sr_st8(0x202049u, 0);

    /* Rear force from the lateral term: sin(pi * lat) of the rear load. */
    if(sr_f(r6) < 0.f) r14 = 0;
    g0 = F(D(g8) * (D(r7) * (1000.0 * D(gp))));
    r4 = fmul(tgp_sin(sin_arg(PI_D * D(sr_ld32(LAT)))), g0);
    {
        const uint32_t lim = grip_limit(r8, r4);
        if(!(sr_f(lim) <= sr_f(g0))) {
            g0 = sr_player_sqrt_mix(fdiv(g0, lim), SR_PLAYER_SQRT_MIX_B);
            r4 = F((1.1 * D(g0)) * D(r4));
            r8 = fmul(g0, r8);
        }
    }
    if(sr_f(sr_ld32(LAT)) >= 0.f) r4 = fneg(r4);
    r4 = small_slip_fade(sr_ld32(LAT), LAT_K, r4);
    if(sr_f(r8) < 0.f) r13 = 0;

    sr_player_pitch_blend(fadd(r8, r6), sr_ld32(DRIVE_FORCE));
    if(sr_f(fmul(r8, r6)) == 0.f) sr_st32(SKID_LEVEL_V, 0);

    /* Skid level 0x214254. The double thresholds 0.15, 0.2 and 0.1 round up to float, so "v >= 0.15" is
     * "v >= 0.15f", "v <= 0.2" is "v < 0.2f" and "v > 0.1" is "v >= 0.1f". */
    {
        const float t15 = sr_f(0x3e19999au), t2 = sr_f(0x3e4ccccdu), t1 = sr_f(0x3dcccccdu);
        const uint32_t side = sr_ld32(0x214200u);
        const float side_mag = sr_f(sr_f(side) >= 0.f ? side : fneg(side));
        const int sliding = !(side_mag < t2) && ldf(LAT_K) >= t1;
        if(sr_ld32(0x2020b4u) != 0) { /* not reached in the checked drives (0x2020b4 stays 0) */
            if(!(ldf(SKID_LEVEL_V) >= t15) && sliding) sr_st32(SKID_LEVEL_V, 0x3e19999au);
            if(!(ldf(SKID_LEVEL_V) >= t15)) sr_st32(SKID_LEVEL_V, 0);
        } else if(ldf(SKID_LEVEL_V) == 0.f && sliding) {
            sr_st32(SKID_LEVEL_V, 0x3c23d70au);
        }
    }
    sr_player_skid_sound(sr_ld32(SKID_LEVEL_V), row);

    /* Forces through the TGP: rotate a1 into the car frame and add the body forces to a2. */
    sr_tgp_cmd(0x10002020u, 0, 0, 0, 0);
    if(sr_f(r6) < 0.f) {
        r8 = fadd(r6, r8);
        r6 = 0;
    }
    {
        const double q13 = 0.66 * D(r13), q14 = 0.635 * D(r14);
        const uint32_t x = F(D(r8) * q13);
        uint32_t z, u, o[3];
        {
            const uint32_t in[4] = {0, r5, 0, r6};
            sr_tgp_cmd(0x21004242u, in, 4, 0, 0);
        }
        sr_tgp_cmd(0x12802525u, 0, 0, 0, 0);
        {
            const uint32_t ang = sr_ld32(node + 0x74u);
            sr_tgp_cmd(0x15002a2au, &ang, 1, 0, 0);
        }
        {
            const uint32_t zero = 0;
            sr_tgp_cmd(0x24004848u, &zero, 1, 0, 0);
            sr_tgp_cmd(0x21804343u, &zero, 1, o, 3);
        }
        z = F(D(x) - D(r6) * q14);
        u = F(1.25 * (D(r4) - D(o[0])) + D(z));
        sr_tgp_cmd(0x12802525u, 0, 0, 0, 0);
        {
            const uint32_t ang = sr_ld32(node + 0x3cu);
            sr_tgp_cmd(0x15002a2au, &ang, 1, 0, 0);
        }
        r4 = fadd(o[0], r4);
        r8 = fadd(o[2], r8);
        {
            const uint32_t in[4] = {0, r4, o[1], r8};
            const uint32_t zero = 0;
            const uint32_t v[4] = {0, a1[0], a1[1], a1[2]};
            sr_tgp_cmd(0x21004242u, in, 4, 0, 0);
            sr_tgp_cmd(0x24004848u, &zero, 1, 0, 0);
            sr_tgp_cmd(0x22004444u, v, 4, 0, 0);
            a2[0] = fadd(F(0.2 * D(r8)), a2[0]);
            a2[1] = fadd(u, a2[1]);
            a2[2] = fadd(F(-0.2 * D(r4)), a2[2]);
            sr_tgp_cmd(0x21804343u, &zero, 1, a1, 3);
        }
    }
    sr_st32(ROW_SEL, row);
    sr_tgp_cmd(0x10802121u, 0, 0, 0, 0);
}
