/* Each ported routine under its lifted name: run the original, then the port from the same state, compare.
 * The lifted definitions are renamed lifted_<name> for this build only (ported/car.list). */
#include "wrap_common.h"
#include "race/car.h"
#include "race/road.h"

SR_DIFF_WRAP3(game_start_race_obj_car_frame, sr_car_frame(a0))
SR_DIFF_WRAP3(game_start_race_obj_pose_publish, sr_pose_publish(a0))
SR_DIFF_WRAP3(game_start_race_obj_car_peer, sr_car_peer(a0))
SR_DIFF_WRAP3(game_start_race_obj_car_yaw_step, sr_car_yaw_step(a0))
SR_DIFF_WRAP3(game_start_race_obj_car_motion, sr_car_motion(a0, a1))
SR_DIFF_WRAP3(game_start_race_obj_car_velocity_step, sr_car_velocity_step(a0))
SR_DIFF_WRAP3(game_start_race_obj_car_integrate, sr_car_integrate(a0, a1))

/* road_span and road_span_basis write temporaries at the caller's fp+0x40..0x5c; basis takes its output in g3. */
void lifted_game_start_race_obj_road_span(u32 a0, u32 a1, u32 a2);
void game_start_race_obj_road_span(u32 a0, u32 a1, u32 a2) {
    uint8_t *frame = (uint8_t *)fp;
    SR_DIFF_BEGIN();
    SR_DIFF_EXTRA(frame, 0x60);
    lifted_game_start_race_obj_road_span(a0, a1, a2);
    SR_DIFF_NATIVE();
    sr_road_span(a0, a1, frame);
    SR_DIFF_END("game_start_race_obj_road_span");
}

void lifted_game_start_race_obj_road_span_basis(u32 a0, u32 a1, u32 a2);
void game_start_race_obj_road_span_basis(u32 a0, u32 a1, u32 a2) {
    uint8_t *frame = (uint8_t *)fp;
    u32 out = (u32)g3;
    SR_DIFF_BEGIN();
    SR_DIFF_EXTRA(frame, 0x60);
    lifted_game_start_race_obj_road_span_basis(a0, a1, a2);
    SR_DIFF_NATIVE();
    sr_road_span_basis(a0, a1, a2, out, frame);
    SR_DIFF_END("game_start_race_obj_road_span_basis");
}


u32 lifted_game_start_race_obj_car_contact_solve(u32 node, void *target, void *angles);
u32 game_start_race_obj_car_contact_solve(u32 node, void *target, void *angles) {
    u32 l, n;
    SR_DIFF_BEGIN();
    SR_DIFF_EXTRA(target, 12);
    l = lifted_game_start_race_obj_car_contact_solve(node, target, angles);
    SR_DIFF_NATIVE();
    n = sr_car_contact_solve(node, target, angles);
    SR_DIFF_RET(l, n);
    SR_DIFF_END("game_start_race_obj_car_contact_solve");
    return l;
}

void lifted_game_start_race_obj_car_index_refresh(u32 node, u32 pred[3]);
void game_start_race_obj_car_index_refresh(u32 node, u32 pred[3]) {
    SR_DIFF_BEGIN();
    SR_DIFF_EXTRA(pred, 12);
    lifted_game_start_race_obj_car_index_refresh(node, pred);
    SR_DIFF_NATIVE();
    sr_car_index_refresh(node, pred);
    SR_DIFF_END("game_start_race_obj_car_index_refresh");
}

void lifted_geo_view_table_index_a(void *a0, u32 a1, void *a2);
void geo_view_table_index_a(void *a0, u32 a1, void *a2) {
    const u32 flags = (u32)g3;
    SR_DIFF_BEGIN();
    lifted_geo_view_table_index_a(a0, a1, a2);
    SR_DIFF_NATIVE();
    sr_road_query_a((u32)(uintptr_t)a0, a1, sr_wrap_host_ptr(a2), flags);
    SR_DIFF_END("geo_view_table_index_a");
}

void lifted_geo_view_table_index_b(void *a0, void *a1, u32 a2);
void geo_view_table_index_b(void *a0, void *a1, u32 a2) {
    SR_DIFF_BEGIN();
    if(sr_wrap_is_host(a1)) SR_DIFF_EXTRA(a1, 0x20);
    lifted_geo_view_table_index_b(a0, a1, a2);
    SR_DIFF_NATIVE();
    sr_road_query_b(sr_wrap_host_ptr(a0), sr_wrap_host_ptr(a1), a2);
    SR_DIFF_END("geo_view_table_index_b");
}

u32 lifted_geo_view_slot_word_pack(u32 a0, u32 a1, u32 a2);
u32 geo_view_slot_word_pack(u32 a0, u32 a1, u32 a2) {
    u32 l, n;
    SR_DIFF_BEGIN();
    l = lifted_geo_view_slot_word_pack(a0, a1, a2);
    SR_DIFF_NATIVE();
    n = (u32)sr_road_cell_pack(sr_at(a0), sr_at(a1));
    SR_DIFF_RET(l, n);
    SR_DIFF_END("geo_view_slot_word_pack");
    return l;
}

u32 lifted_geo_view_float_clamp(u32 a0, u32 a1, u32 a2);
u32 geo_view_float_clamp(u32 a0, u32 a1, u32 a2) {
    u32 l, n;
    SR_DIFF_BEGIN();
    l = lifted_geo_view_float_clamp(a0, a1, a2);
    SR_DIFF_NATIVE();
    n = sr_angle_wrap(a0);
    SR_DIFF_RET(l, n);
    SR_DIFF_END("geo_view_float_clamp");
    return l;
}
