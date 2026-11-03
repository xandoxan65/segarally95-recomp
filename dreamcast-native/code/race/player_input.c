/* The player's controls: accelerator, brake and gear lever into engine revs, gear and drive force
 * (geo_view_matrix_prep @ 0x3A320 and callees), and the wheel into the steering feedback byte
 * (geo_view_scene_apply @ 0x39750). See include/race/player_input.h. */
#include "race/player_input.h"
#include "race/player_scene.h" /* sr_steer_amount, sr_rng_next */
#include "race/sound_cmd.h"
#include "race/fp.h"
#include "sr_tgp.h"

#define RPM_MAX_BITS 0x46098000u /* 8800.0f */

static double rpm_max(void) { return sr_d(0, 0x40c13000u); } /* 8800.0 */

/* Unsigned word to float as the i960 does it: signed convert, plus 2^32 when negative. */
static uint32_t u32_to_float(uint32_t v) {
    uint32_t f = sr_b((float)(int32_t)v);
    if((int32_t)v < 0) f = fadd(0x4f800000u, f);
    return f;
}

/* The lifted code compares a sign-extended word with a zero-extended local: words with bit 31 never match. */
static int same_word(uint32_t sext, uint32_t zext) { return sext == zext && !(sext & 0x80000000u); }

/* x clamped to [0, 8800], with the lifted tests' order (NaN gives 0). */
static uint32_t rpm_clamp(uint32_t x) {
    const double v = D(x);
    if((v >= 0.0 ? v : 0.0) >= rpm_max()) return RPM_MAX_BITS;
    return v >= 0.0 ? x : 0;
}


uint32_t sr_player_torque(uint32_t rpm, uint32_t throttle, uint32_t *scale_out) {
    const float x = sr_f(rpm);
    const float q = x / 100.f;
    const int32_t rem = (int32_t)x % 100;
    const int32_t idx = (int32_t)(q / 1.1f);
    const float frac = (float)rem / 100.f;
    const uint32_t base = sr_ld32(0x2142f8u) + (uint32_t)(idx << 2);
    const float lo = sr_f(sr_ld32(base)), hi = sr_f(sr_ld32(base + 4));
    float scale = sr_f(throttle) * sr_f(sr_ld32(0x5d930cu));
    scale *= sr_f(sr_ld32(0x214124u));
    if(scale_out) *scale_out = sr_b(scale);
    return sr_b((lo + frac * (hi - lo)) * scale);
}

uint32_t sr_player_brake(uint32_t cam) {
    const int32_t raw = ((int32_t)sr_ld8(0x202052u) - 0x30) >> 1;
    const float pedal = (float)raw / 64.f;
    uint32_t flags = sr_ld8(cam + PLAYER_FLAGS10), level = sr_b(pedal);
    float out;
    flags = pedal > 0.6f ? (flags | 0x10u) : (flags & ~0x10u);
    sr_st8(cam + PLAYER_FLAGS10, flags);
    if((int32_t)sr_ld32(0x214120u) > 1) level = 0x3f000000u; /* 0.5 */
    out = sr_f(sr_ld32(0x5d9310u)) * sr_f(sr_ld32(0x214124u)) * sr_f(level);
    if(out < 0.f) out = 0.f;
    return sr_b(out);
}

void sr_player_overrev(uint32_t torque) {
    const uint32_t mode = sr_ld32(0x214330u);
    const float x = sr_f(torque);
    if(mode == 1) {
        if(x < 0.01f) {
            sr_sound_cmd_fill(0x6c, 0x40);
            sr_st32(0x21432cu, 0);
            sr_st32(0x214330u, 2);
        }
    } else if(mode == 2) {
        sr_sound_cmd(sr_ld32(0x5d9e30u + ((sr_rng_next() & 7u) << 2)));
        sr_st32(0x214330u, 0);
    } else if(mode == 0) {
        uint32_t count = sr_ld32(0x21432cu);
        if(x > 0.15f) {
            sr_st32(0x21432cu, count + 1);
            if((int32_t)count > 60) sr_st32(0x214330u, 1);
        } else {
            sr_st32(0x21432cu, (int32_t)count <= 0 ? 0 : count - 1);
        }
    }
}

uint32_t sr_player_gear_auto(uint32_t cam, uint32_t throttle) {
    uint32_t counter, gear;
    float rpm, t;
    if(sr_ld32(0x214120u) == 0) return 1;
    counter = sr_ld32(0x2142e0u);
    rpm = sr_f(sr_ld32(cam + PLAYER_RPM));
    gear = sr_ld32(cam + PLAYER_GEAR);
    sr_st32(0x2142e0u, counter + 1);
    if((int32_t)counter <= 29) return gear;

    if(rpm > (float)sr_d(1, 0x40bef000u) && sr_f(sr_ld32(0x2142f0u)) < 0.1f) {
        gear++;
    } else {
        t = sr_f(throttle) * (float)sr_d(0, 0x40a57c00u);
        t = t + (float)sr_d(1, 0x40a9c800u);
        if(rpm < t) gear--;
    }
    if((int32_t)gear <= 0) gear = 1;
    else if((int32_t)gear > 4) gear = 4;
    if(sr_ld32(cam + PLAYER_GEAR) != gear) sr_st32(0x2142e0u, 0);
    return gear;
}

uint32_t sr_player_gear_manual(uint32_t gear) {
    const uint32_t lever = sr_ld32(0x202044u);
    const uint32_t counter = sr_ld32(0x2142e0u);
    if((int32_t)lever == (int32_t)gear) {
        sr_st32(0x2142e0u, 0);
        return gear;
    }
    if((int32_t)counter <= 6) {
        sr_st32(0x2142e0u, counter + 1);
        return gear;
    }
    if((int32_t)lever != 0) {
        if((int32_t)lever < (int32_t)gear) sr_st32(0x2142fcu, 6);
        else sr_st32(0x214300u, sr_ld32(0x2139ccu) == 1 ? (uint32_t)-6 : (uint32_t)-8);
    }
    sr_st32(0x2142e0u, counter + 1);
    return lever;
}

void sr_player_engine_sound(uint32_t rpm, uint32_t throttle) {
    const uint32_t mute = sr_ld32(0x214208u) & 0x300u;
    const uint32_t pitch_max = sr_ld32(0x214314u), vol_max = sr_ld32(0x214318u);
    const uint32_t vol_old = sr_ld32(0x214308u);
    uint32_t pitch, vol, on;

    pitch = (uint32_t)(int64_t)(D(fmul(rpm, u32_to_float(pitch_max))) / rpm_max());
    if(pitch > pitch_max) pitch = pitch_max;
    else if(pitch == 0) pitch = 1;

    /* Volume follows the throttle, falling a quarter of the way per tick. */
    vol = (uint32_t)(int64_t)D(fmul(throttle, u32_to_float(vol_max)));
    if(vol < vol_old) {
        const uint32_t step = (vol_old - vol) >> 2;
        vol = step ? vol_old - step : vol_old - 1;
    }
    if(sr_ld32(0x2142fcu) != 0) vol = vol_max;
    else if(sr_ld32(0x214300u) != 0) vol = 0;

    /* Muting and unmuting (0x214208 bits 8-9) is not reached in the drives: unchecked. */
    on = sr_ld32(0x21431cu);
    if(on != 0 && mute == 0) {
        sr_sound_cmd_fill(sr_ld32(0x214324u), 0);
        sr_st32(0x21431cu, 0);
    } else if(mute != 0 && on == 0) {
        sr_sound_cmd(sr_ld32(0x214320u));
        sr_st32(0x214304u, 0);
        sr_st32(0x21431cu, 1);
    }

    if(!same_word(sr_ld32(0x214304u), pitch) || !same_word(sr_ld32(0x214308u), vol)) {
        if(sr_ld32(0x21431cu) != 0) sr_sound_cmd_fill(sr_ld32(0x214324u), pitch);
        sr_sound_cmd_fill(sr_ld32(0x214310u), vol);
        sr_sound_cmd_fill(sr_ld32(0x21430cu), pitch);
        sr_st32(0x214308u, vol);
        sr_st32(0x214304u, pitch);
    }
}

uint32_t sr_player_matrix_prep(uint32_t cam, uint32_t scratch) {
    uint32_t throttle, pedal, gear, timer, dist_scale, heading, rpm, rpm_target, slip, r7, r11, net;
    uint32_t xf[3], torque, brake, pull, shift_up;
    double v;

    /* Accelerator 0x30..0xbf as 0..1; 0.3 while a downshift blips (0x2142fc), 0 from the second race tick. */
    v = (double)((int32_t)sr_ld8(0x202051u) - 0x30);
    pedal = F(v);
    if((v >= 0.0 ? v : 0.0) >= 143.0) pedal = 0x430f0000u;
    else if(!(v >= 0.0)) pedal = 0;
    throttle = fdiv(pedal, 0x430f0000u);
    timer = sr_ld32(0x2142fcu);
    if(timer != 0) {
        timer--;
        throttle = 0x3e99999au;
    }
    sr_st32(0x2142fcu, timer);
    shift_up = sr_ld32(0x214300u);
    if((int32_t)shift_up < 0) shift_up++;
    sr_st32(0x214300u, shift_up);
    if((int32_t)sr_ld32(0x214120u) > 1) throttle = 0;

    if(sr_ld32(0x2139d0u) == 0) gear = sr_player_gear_auto(cam, throttle);
    else gear = sr_player_gear_manual(sr_ld32(cam + PLAYER_GEAR));
    sr_st32(cam + PLAYER_GEAR, gear);
    /* Neutral before the race and during a downshift. */
    gear = (sr_ld32(0x214120u) == 0 || sr_ld32(0x2142fcu) != 0) ? 0 : sr_ld32(cam + PLAYER_GEAR);

    if(!(sr_ld32(0x202008u) & 1)) sr_player_engine_sound(sr_ld32(cam + PLAYER_RPM), throttle);

    /* Gear ratio, and the car's speed along its heading from the TGP (velocity . rotated forward axis). */
    dist_scale = fmul(sr_ld32(0x5d9254u), sr_ld32(0x5d9264u + (gear << 2)));
    sr_tgp_cmd(0x10002020u, 0, 0, 0, 0);
    sr_tgp_cmd(0x12802525u, 0, 0, 0, 0);
    sr_st32(0x2142ecu, dist_scale);
    heading = fadd(sr_ld32(cam + 0x74), sr_ld32(cam + 0x3c));
    sr_tgp_cmd(0x15002a2au, &heading, 1, 0, 0);
    {
        const uint32_t axis[3] = {0, 0, 0x3f800000u};
        sr_tgp_cmd(0x16002c2cu, axis, 3, xf, 3);
    }
    {
        const uint32_t in[6] = {sr_ld32(cam + 0x2c), xf[0], sr_ld32(cam + 0x30), xf[1], sr_ld32(cam + 0x34), xf[2]};
        sr_tgp_cmd(0x2c805959u, in, 6, &pull, 1); /* forward speed, r10 */
    }
    sr_tgp_cmd(0x10802121u, 0, 0, 0, 0);
    r11 = fadd(sr_ld32(0x5d9300u), fdiv(sr_ld32(0x5d9304u), dist_scale));

    /* Revs the wheels turn the engine at. */
    if(D(pull) >= 0.0) {
        const uint32_t g4 = fmul(0x42700000u, fmul(0x42700000u, sr_ld32(0x20b0b4u)));
        r7 = F(D(sr_ld32(0x2142ecu)) * (D(g4) / 1.8849555921538759));
    } else {
        r7 = 0;
    }

    if(gear != 0) {
        /* In gear: revs follow the wheels at most 200 per tick; the slip is how far they are apart. */
        uint32_t delta, ref;
        rpm_target = rpm_clamp(fadd(sr_ld32(0x2142e4u), r7));
        sr_st32(0x2142e8u, rpm_target);
        rpm = sr_ld32(cam + PLAYER_RPM);
        delta = fsub(rpm_target, rpm);
        if(!(D(fabs_bits(delta)) <= 200.0)) delta = D(delta) >= 0.0 ? 0x43480000u : 0xc3480000u;
        rpm = fadd(rpm, delta);
        sr_st32(cam + PLAYER_RPM, rpm);
        if(sr_ld32(0x2020b4u) != 0) { /* not reached in the drives: unchecked */
            const uint32_t k = fmul(sr_ld32(0x5d9318u), sr_ld32(scratch));
            ref = F((1.0 + D(k)) * D(rpm));
        } else {
            ref = F(D(rpm));
        }
        slip = fdiv(fsub(ref, r7), ref);
        if(D(slip) < 0.0) slip ^= 0x80000000u;
        sr_st32(scratch, D(slip) <= 1.0 ? slip : 0x3f800000u);
        sr_st32(cam + PLAYER_ROT_STEP, F(2.0 * 3.141592653589793 * D(sr_ld32(cam + PLAYER_RPM)) /
                                         D(sr_ld32(0x2142ecu)) / sr_d(0, 0x40ac2000u)));
    } else {
        sr_st32(cam + PLAYER_RPM, sr_ld32(0x2142e8u));
        sr_st32(cam + PLAYER_ROT_STEP, F(D(sr_ld32(0x20b0b4u)) / 0.3));
        sr_st32(scratch, 0); /* g14 */
    }

    {
        const uint32_t step = F(D(sr_ld32(0x20b0b4u)) / 0.3);
        const uint32_t acc = fadd(step, sr_ld32(cam + PLAYER_ROT));
        const double r = D(sr_ld32(cam + PLAYER_RPM));
        sr_st32(cam + PLAYER_ROT_STEP, step);
        sr_st32(cam + PLAYER_ROT, acc);
        if(r <= 1000.0 || !(D(pull) > 0.0)) sr_st32(scratch, 0); /* g14 */
    }

    {
        const double r = D(sr_ld32(cam + PLAYER_RPM));
        const double lim = sr_d(0, 0x40b77000u); /* 6000 */
        if(!(r >= lim) && !(D(throttle) <= 0.8)) sr_st32(0x2142f4u, F(1.0 - (lim - r) / lim));
        else sr_st32(0x2142f4u, 0x3f800000u);
    }
    sr_st32(0x2142f0u, sr_ld32(scratch));

    torque = sr_player_torque(sr_ld32(cam + PLAYER_RPM), throttle, 0);
    sr_player_overrev(torque);

    /* Next tick's free revs, then the drive force from the gap to the wheel revs. */
    rpm_target = fadd(sr_ld32(cam + PLAYER_RPM), fdiv(fsub(torque, sr_ld32(0x5d9308u)), r11));
    rpm_target = rpm_clamp(rpm_target);
    sr_st32(0x2142e8u, rpm_target);
    net = fmul(r11, fsub(rpm_target, r7));
    if(gear != 0 && (int32_t)sr_ld32(0x214120u) <= 1) {
        net = fmul(sr_ld32(0x2142ecu), net);
        net = F(D(net) / 0.3);
    } else {
        net = 0;
    }

    brake = sr_player_brake(cam);
    {
        double b = D(brake);
        if(D(pull) < 0.0) b = -b;
        return F(D(net) - b);
    }
}

void sr_player_steer_feedback(uint32_t prev) {
    float angle, r12, sum_a, sum_b, g1f, g2f, g3f, g7f, y, acc;
    int32_t idx, code, lim, last;
    uint32_t slot, ptr;
    uint8_t mode;

    r12 = (sr_f(sr_ld32(0x214270u)) + 0.2f) * 2.f;
    sr_st32(0x214294u, sr_ld32(0x214290u));
    angle = sr_f(sr_steer_amount());

    sum_a = sr_f(sr_ld32(0x2142a0u)) + sr_f(sr_ld32(0x2142a4u)) + sr_f(sr_ld32(0x2142a8u)) + sr_f(sr_ld32(0x2142acu));
    sum_b = sr_f(sr_ld32(0x2142b0u)) + sr_f(sr_ld32(0x2142b4u)) + sr_f(sr_ld32(0x2142b8u)) + sr_f(sr_ld32(0x2142bcu));
    g3f = sum_a * 0.5f;
    g2f = sum_b * 0.5f;
    g1f = sr_f(sr_ld32(0x214284u) ^ 0x80000000u) * r12;

    /* Centring force curve: 64 entries by |angle|, through pointers at 0x5d8230. */
    {
        const float mag = angle < 0.f ? -angle : angle;
        idx = (int32_t)(mag * 63.f);
        ptr = idx > 63 ? sr_ld32(0x5d832cu) : sr_ld32(0x5d8230u + ((uint32_t)idx << 2));
        y = sr_f(sr_ld32(ptr)) / 63.f;
        if(angle < 0.f) y = -y;
        g1f = g1f * y;
    }

    g7f = angle - sr_f(sr_ld32(0x214294u));
    {
        const float neg_d = sr_f(sr_b(g7f) ^ 0x80000000u);
        const float t0 = g2f * sr_f(sr_ld32(0x21428cu));
        const float t1 = neg_d * sr_f(sr_ld32(0x214288u));
        const float t2 = g3f - t0;
        g1f = g1f + t1 * r12 - t2 * r12;
    }
    acc = (sr_f(sr_ld32(0x214280u)) + g1f) * 0.5f;

    slot = sr_ld32(0x202008u) & 3u;
    sr_st32(0x2142a0u + (slot << 2), sr_b(g1f));
    lim = (int32_t)sr_ld32(0x214274u);
    code = (int32_t)((acc / 20.f) * (float)lim);
    sr_st32(0x214290u, prev);
    slot = sr_ld32(0x202008u) & 3u;
    sr_st32(0x2142b0u + (slot << 2), sr_b(g7f));
    sr_st32(0x214280u, sr_b(acc));

    /* Step one unit past a change so the motor overshoots slightly. */
    last = (int32_t)sr_ld32(0x21427cu);
    if(last == 0) last = code;
    if(last < code) {
        last = code;
        code = code + 1;
    } else if(last > code) {
        last = code;
        code = code - 1;
    }
    sr_st32(0x21427cu, (uint32_t)last);
    if(code > lim) code = lim;
    if(code < -lim) code = -lim;

    mode = (uint8_t)sr_ld8(0x20201au);
    if(code == 0) {
        if(mode == 1) {
            sr_st8(0x202049u, 0);
            return;
        }
        code = (sr_ld32(0x202008u) & 3u) == 0 ? 1 : -1; /* not reached in the drives: unchecked */
    }
    code = code >= 0 ? code - 0x41 : code + 0x7f;
    sr_st8(0x202049u, (uint32_t)code);
    if(mode == 1 && sr_ld32(0x202098u) != 4) sr_st8(0x202049u, 0);
}
