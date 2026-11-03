/* The player-input routines under their lifted names: run the original, then the port from the same state,
 * compare (ported/player_input.list).
 * geo_view_matrix_slot_init (automatic gearbox) is never called in the practice drives (manual gearbox). */
#include "wrap_common.h"
#include "race/player_input.h"
#include <stdio.h>

/* A routine returning g0: compare the returned word too. */
#define SR_DIFF_WRAP_RET(name, call)                                     \
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

SR_DIFF_WRAP_RET(geo_view_control_byte_apply, sr_player_brake(a0))
SR_DIFF_WRAP_RET(geo_view_matrix_slot_init, sr_player_gear_auto(a0, a1))
SR_DIFF_WRAP_RET(geo_view_matrix_slot_step, sr_player_gear_manual(a0))
SR_DIFF_WRAP3(geo_view_matrix_mode_gate, sr_player_overrev(a0))
SR_DIFF_WRAP3(geo_view_matrix_slot_fill, sr_player_engine_sound(a0, a1))
SR_DIFF_WRAP3(geo_view_scene_apply, sr_player_steer_feedback(a0))

/* angle_frac also leaves its scale in g1. */
u32 lifted_geo_view_angle_frac(u32 a0, u32 a1, u32 a2);
u32 geo_view_angle_frac(u32 a0, u32 a1, u32 a2) {
    u32 l, n, scale;
    SR_DIFF_BEGIN();
    l = lifted_geo_view_angle_frac(a0, a1, a2);
    SR_DIFF_NATIVE();
    n = sr_player_torque(a0, a1, &scale);
    if(scale != (u32)g1) fprintf(stderr, "sr_diff: geo_view_angle_frac g1 lifted %08x native %08x\n", (u32)g1, scale);
    SR_DIFF_RET(l, n);
    SR_DIFF_END("geo_view_angle_frac");
    return l;
}

/* The port assumes g14 = 0 (frame_update clears it before the call). */
u32 lifted_geo_view_matrix_prep(u32 a0, u32 a1, u32 a2);
u32 geo_view_matrix_prep(u32 a0, u32 a1, u32 a2) {
    const u32 cam = a0 ? a0 : (u32)g0, scratch = a1 ? a1 : (u32)g1;
    u32 l, n;
    if(g14 != 0) fprintf(stderr, "sr_diff: geo_view_matrix_prep entered with g14 %08x\n", (u32)g14);
    SR_DIFF_BEGIN();
    l = lifted_geo_view_matrix_prep(a0, a1, a2);
    SR_DIFF_NATIVE();
    n = sr_player_matrix_prep(cam, scratch);
    SR_DIFF_RET(l, n);
    SR_DIFF_END("geo_view_matrix_prep");
    return l;
}
