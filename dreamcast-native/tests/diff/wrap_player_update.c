/* The player car's frame-update helpers (race/player_update.h) under their lifted names: run the original, then
 * the port from the same state, compare RAM, TGP and the returned g0 (ported/player_update.list). */
#include "wrap_common.h"
#include "race/player_update.h"

/* A routine returning g0: compare it too. */
#define SR_DIFF_WRAP_RET(name, call)                                      \
    u32 lifted_##name(u32 a0, u32 a1, u32 a2);                           \
    u32 name(u32 a0, u32 a1, u32 a2) {                                   \
        u32 l, n;                                                        \
        SR_DIFF_BEGIN();                                                 \
        l = lifted_##name(a0, a1, a2);                                   \
        SR_DIFF_NATIVE();                                                \
        n = (call);                                                      \
        SR_DIFF_RET(l, n);                                               \
        SR_DIFF_END(#name);                                              \
        return l;                                                        \
    }

SR_DIFF_WRAP_RET(geo_view_input_gate, sr_player_airborne_gate((int32_t)a0))
SR_DIFF_WRAP3(geo_view_latch_apply, sr_player_skid_sound(a0, a1))
SR_DIFF_WRAP3(geo_view_mode_apply, sr_player_force_feedback(a0, a1, a2))
SR_DIFF_WRAP_RET(geo_view_trig_scale, sr_player_trig_scale(a0))
SR_DIFF_WRAP3(geo_view_pitch_blend, sr_player_pitch_blend(a0, a1))
SR_DIFF_WRAP_RET(geo_view_sin_mix, sr_player_sin_mix(a0))
SR_DIFF_WRAP_RET(geo_view_sqrt_mix_a, sr_player_sqrt_mix(a0, SR_PLAYER_SQRT_MIX_A))
SR_DIFF_WRAP_RET(geo_view_sqrt_mix_b, sr_player_sqrt_mix(a0, SR_PLAYER_SQRT_MIX_B))

/* frame_update: a1/a2 are host words in scene_frame's frame (+0x120 / +0x150), g3 the correction seed. Both
 * vectors are compared: one SR_DIFF_EXTRA span when they lie within 1 KiB of each other (scene_frame's case), else
 * a2 by hand. */
#include <stdio.h>
#include <string.h>
void lifted_geo_view_frame_update(void *a0, void *a1, void *a2);
void geo_view_frame_update(void *a0, void *a1, void *a2) {
    static unsigned long s_a2_mismatch;
    uint8_t *p1 = (uint8_t *)a1, *p2 = (uint8_t *)a2;
    uint8_t *lo = p1 < p2 ? p1 : p2, *hi = p1 < p2 ? p2 : p1;
    const int span = hi + 12 - lo <= 1024;
    const u32 node = (u32)(uintptr_t)a0, corr = (u32)g3;
    uint32_t v1[3], v2[3];
    uint8_t a2_before[12], a2_after[12];
    SR_DIFF_BEGIN();
    if(span) SR_DIFF_EXTRA(lo, (unsigned)(hi + 12 - lo));
    else SR_DIFF_EXTRA(p1, 12);
    memcpy(a2_before, p2, 12);
    lifted_geo_view_frame_update(a0, a1, a2);
    SR_DIFF_NATIVE();
    memcpy(a2_after, p2, 12);
    memcpy(p2, a2_before, 12);
    memcpy(v1, p1, 12);
    memcpy(v2, p2, 12);
    sr_player_frame_update(node, v1, v2, corr);
    memcpy(p1, v1, 12);
    memcpy(p2, v2, 12);
    if(!span && memcmp(p2, a2_after, 12) != 0)
        fprintf(stderr, "sr_diff: geo_view_frame_update a2 differs (%lu)\n", ++s_a2_mismatch);
    SR_DIFF_END("geo_view_frame_update");
    if(!span) memcpy(p2, a2_after, 12);
}
