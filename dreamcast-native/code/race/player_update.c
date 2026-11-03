/* Leaf helpers of the player car's frame update (geo_view_frame_update @ 0x37870): airborne gate, skid sound,
 * steering wheel force, pitch blend and the sin/sqrt response curves (race/player_update.h).
 *
 * The lifted helpers compute in float (each op rounded, no double chains), so these do too: with
 * -ffp-contract=off a float expression here is the same sequence of IEEE single ops. sin goes to the TGP
 * (0x0a801515) as in the original. */
#include "race/player_update.h"
#include "race/sound_cmd.h"
#include "race/fp.h"
#include "sr_tgp.h"
#include <math.h>

#define PI_F 3.14159265358979323846f /* (float)M_PI, 0x40490fdb */

static float ldf(uint32_t va) { return sr_f(sr_ld32(va)); }

static float tgp_sin(float a) {
    const uint32_t in = sr_b(a);
    uint32_t out;
    sr_tgp_cmd(0x0a801515u, &in, 1, &out, 1);
    return sr_f(out);
}

/* --- Sound commands ------------------------------------------------------------------------------------------- */

#define SOUND_CMD_TABLE  0x5c3ce0u /* 3-byte packets */
#define SOUND_RING       0x20b100u /* 0x80 bytes */
#define SOUND_PENDING    0x20b180u
#define SOUND_CURSOR     0x20b184u
#define SOUND_BUSY       0x20b188u
#define SOUND_STAGING    0x20b18cu


/* --- Airborne gate (0x388D0) ---------------------------------------------------------------------------------- */

#define AIR_TICKS 0x214250u
#define FIVE_F    0x40a00000u /* 5.0f */

uint32_t sr_player_airborne_gate(int32_t state) {
    uint32_t n;
    if(state < 0) {
        sr_st32(AIR_TICKS, 0);
        return 0;
    }
    if(state > 0) {
        /* Landing: a thump after more than 10 ticks in the air. */
        n = sr_ld32(AIR_TICKS);
        sr_st32(AIR_TICKS, 0);
        if((int32_t)n <= 10) return 0;
        sr_sound_cmd_fill(0x47u, 0x7fu);
        return FIVE_F;
    }
    sr_sound_cmd(0x96u);
    n = sr_ld32(AIR_TICKS) + 1u;
    sr_st32(AIR_TICKS, n);
    return (int32_t)n > 59 ? 0 : FIVE_F;
}

/* --- Skid sound (0x38680) ------------------------------------------------------------------------------------- */

#define SKID_PLAYING  0x214230u /* 1 while the skid sound runs */
#define SKID_LEVEL    0x214234u /* level last sent with sound 0xb5 (-1: none yet) */
#define SKID_START    0x214238u /* level at the last (re)start */
#define SKID_BASE_ON  0x21423cu /* row word 3: level offset while playing */
#define SKID_BASE     0x214240u /* row word 2: level offset at the start */
#define SKID_SOUND    0x214244u /* row word 0: start sound */
#define SKID_SOUND_ON 0x214248u /* row word 1: restart sound */
#define SKID_ROW      0x21424cu /* row the words were taken from (15: none) */
#define SKID_TABLE    0x5dcd10u /* 16 bytes per dynamics row */

static uint32_t skid_volume(float level, uint32_t base_va) {
    const float v = level * 100.f * 0.4f;
    const uint32_t r = (uint32_t)((int32_t)v + (int32_t)sr_ld32(base_va));
    return (int32_t)r > 0x7f ? 0x7fu : r;
}

static void skid_play(uint32_t sound_va) {
    const uint32_t s = sr_ld32(sound_va);
    sr_sound_cmd_fill(s, 0);
    if((uint8_t)s == 0xb3u) sr_sound_cmd_fill(0x95u, 0x7fu);
}

void sr_player_skid_sound(uint32_t level_bits, uint32_t row) {
    const float level = sr_f(level_bits);
    uint32_t vol;
    if(!(level >= 0.f)) {
        sr_st32(SKID_PLAYING, 0);
        sr_st32(SKID_ROW, 15u);
        return;
    }
    if(((sr_ld32(0x202008u) + 8u) & 7u) != 0) return;
    if((uint8_t)row != (uint8_t)sr_ld32(SKID_ROW)) {
        const uint32_t base = SKID_TABLE + (row << 4);
        const uint32_t w0 = sr_ld32(base), w1 = sr_ld32(base + 4), w2 = sr_ld32(base + 8), w3 = sr_ld32(base + 12);
        sr_st32(SKID_PLAYING, 0);
        sr_st32(SKID_LEVEL, 0);
        sr_st32(SKID_START, 0);
        sr_st32(SKID_SOUND, w0);
        sr_st32(SKID_ROW, row);
        sr_st32(SKID_SOUND_ON, w1);
        sr_st32(SKID_BASE, w2);
        sr_st32(SKID_BASE_ON, w3);
        sr_sound_cmd_fill(0xb5u, 0);
    }
    if(level == 0.f) {
        sr_st32(SKID_PLAYING, 0);
        vol = 0;
    } else if((uint8_t)sr_ld32(SKID_PLAYING) == 0) {
        vol = skid_volume(level, SKID_BASE);
        skid_play(SKID_SOUND);
        sr_st32(SKID_LEVEL, 0xffffffffu);
        sr_st32(SKID_START, vol);
        sr_st32(SKID_PLAYING, 1);
    } else {
        const uint32_t start = sr_ld32(SKID_START);
        vol = skid_volume(level, SKID_BASE_ON);
        /* Restart only when the volume moved by more than 3 (unsigned compare, signed test as the original). */
        if((int32_t)(vol >= start ? vol - start : start - vol) > 3) {
            sr_st32(SKID_START, vol);
            sr_sound_cmd_fill(0xb5u, 0);
            skid_play(SKID_SOUND_ON);
        }
    }
    if((uint8_t)sr_ld32(SKID_LEVEL) == (uint8_t)vol) return;
    sr_sound_cmd_fill(0xb5u, vol);
    sr_st32(SKID_LEVEL, vol);
}

/* --- Steering wheel force (0x39C40, table lerp 0x39B80) ------------------------------------------------------- */

#define WHEEL_OUT       0x202049u /* byte to the drive board */
#define WHEEL_PREV_IN   0x2142c4u
#define WHEEL_MODE      0x2142c8u
#define WHEEL_COUNT     0x2142ccu
#define WHEEL_ARG       0x2142d0u /* countdown strength (word) / latch value (byte) */
#define WHEEL_LATCHED   0x2142d4u
#define VIEW_FOV_MIX    0x214270u

uint32_t sr_player_trig_scale(uint32_t angle_bits) {
    const float angle = sr_f(angle_bits);
    const float mag = angle < 0.f ? -angle : angle;
    const float clamped = mag >= 63.f ? 63.f : mag;
    const int32_t idx = (int32_t)clamped;
    const float frac = clamped - (float)idx;
    const uint32_t off = (uint32_t)idx << 2;
    float a = ldf(0x5d8330u + off), b = ldf(0x5d8430u + off), out;
    if(idx != 63) {
        const float a1 = ldf(0x5d8334u + off), b1 = ldf(0x5d8434u + off);
        a = a + (a1 - a) * frac;
        b = b + (b1 - b) * frac;
    }
    out = a + (b - a) * ldf(VIEW_FOV_MIX);
    if(angle < 0.f) out = -out;
    return sr_b(out);
}

void sr_player_force_feedback(uint32_t in_bits, uint32_t target_bits, uint32_t slip_bits) {
    const uint32_t mode = sr_ld32(WHEEL_MODE);
    const uint32_t prev = sr_ld32(WHEEL_PREV_IN);
    const float in = sr_f(in_bits);
    const float d_in = in - sr_f(prev);
    sr_st32(WHEEL_PREV_IN, in_bits);

    if(mode == 0) {
        const float slip = sr_f(slip_bits);
        const float a = slip < 0.f ? -slip : slip;
        const float t = 0.9f * sqrtf(a) + 0.1f * (a * a);
        const float s = tgp_sin(t * PI_F);
        const float mag_in = in < 0.f ? -in : in;
        float f = sr_f(target_bits) * ldf(0x5d8534u);
        uint32_t out;
        float sign;
        int32_t code;
        f = f * s;
        if(mag_in > 0.05f) f = f - ldf(0x5d8530u) * d_in;
        out = sr_player_trig_scale(sr_b(ldf(0x214278u) * in + f));
        code = (int32_t)sr_f(out);
        sign = sr_f(out);
        if(code < 0) code = -code;
        if(code > 31) code = 31; /* never reached in the checked drives */
        if(code == 0) {
            /* No force: 0 when 0x20201a is 1, else a minimal push alternating every 4 ticks (the push is not
             * reached in the checked drives: 0x20201a is always 1 there). */
            if(sr_ld8(0x20201au) == 1u) {
                sr_st8(WHEEL_OUT, 0);
                return;
            }
            if((sr_ld32(0x202008u) & 3u) == 0) {
                code = -1;
                sign = -1.f;
            } else {
                code = 1;
                sign = 1.f;
            }
        }
        sr_st8(WHEEL_OUT, (uint32_t)(code + (sign < 0.f ? 0x7f : 0xbf)));
        return;
    }
    if(mode == 1) {
        const int32_t n = (int32_t)sr_ld32(WHEEL_ARG);
        const float v = (float)n * (ldf(VIEW_FOV_MIX) + 0.1f);
        int32_t code = (int32_t)v;
        uint32_t count;
        if(code < -31) code = -31;
        if(code > 31) code = 31;
        code = code < 0 ? -64 - code : code - 0x80;
        sr_st8(WHEEL_OUT, (uint32_t)code);
        count = sr_ld32(WHEEL_COUNT) - 1u;
        sr_st32(WHEEL_COUNT, count);
        if(count == 0xffffffffu) sr_st32(WHEEL_MODE, 0);
        return;
    }
    if(mode == 2) {
        const uint32_t b = sr_ld8(WHEEL_ARG);
        sr_st32(WHEEL_LATCHED, 1);
        sr_st32(WHEEL_MODE, 0);
        sr_st8(WHEEL_OUT, b + 16u);
        return;
    }
    if(mode == 3) {
        sr_st32(WHEEL_MODE, 0);
        sr_st32(WHEEL_LATCHED, 0);
        sr_st8(WHEEL_OUT, 16u);
        return;
    }
    sr_st32(WHEEL_MODE, 0); /* any other mode (not reached in the checked drives) */
}

/* --- Pitch blend (0x3AA38) and response curves (0x38A18, 0x38958, 0x389B8) ----------------------------------- */

void sr_player_pitch_blend(uint32_t sum, uint32_t base) {
    const float k = ldf(0x5d9314u) * 0.3f;
    const float a0 = sr_f(sum) * ldf(0x2142f4u);
    const float w = ldf(0x2142ecu);
    const float t = ldf(0x5d9304u) / w;
    const float a1 = sr_f(base) - a0;
    const float num = k * a1;
    const float den = t + ldf(0x5d9300u);
    sr_st32(0x2142e4u, sr_b((num / w) / den));
}

uint32_t sr_player_sin_mix(uint32_t x_bits) {
    const float x = sr_f(x_bits);
    const float s = tgp_sin(x * PI_F);
    const float scale = ldf(x > 0.f ? 0x5d6688u : 0x5d668cu);
    return sr_b(x + s * scale);
}

uint32_t sr_player_sqrt_mix(uint32_t x_bits, uint32_t scale_va) {
    const float x = sr_f(x_bits);
    const float s = ldf(scale_va);
    return sr_b(s * sqrtf(x) + (1.f - s) * (x * x));
}
