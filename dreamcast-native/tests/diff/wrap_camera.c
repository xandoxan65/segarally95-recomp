/* Camera ports (code/race/camera.c) against their lifted originals; render hooks for the diff build. */
#include "wrap_common.h"
#include "race/camera.h"
#include "race/render_hooks.h"
#include "i960_mem.h"

void sr_hook_geo_view_range(uint32_t reg, uint32_t a, uint32_t b) {
    i960_mmio_write_u32(0x800090, reg);
    i960_mmio_write_u32(0x804000, a);
    i960_mmio_write_u32(0x804000, b);
}

void sr_hook_geo_light_dir(uint32_t reg, uint32_t x, uint32_t y, uint32_t z) {
    i960_mmio_write_u32(0x8000a0, reg);
    i960_mmio_write_u32(0x804000, x);
    i960_mmio_write_u32(0x804000, y);
    i960_mmio_write_u32(0x804000, z);
}

void lifted_game_start_race_cam_bind(u32 a0, u32 a1, u32 a2);
void game_start_race_cam_bind(u32 a0, u32 a1, u32 a2) {
    const u32 link = (u32)g14;
    SR_DIFF_BEGIN();
    lifted_game_start_race_cam_bind(a0, a1, a2);
    SR_DIFF_NATIVE();
    sr_cam_bind(a0, link);
    SR_DIFF_END("game_start_race_cam_bind");
}

SR_DIFF_WRAP3(game_start_race_cam_rebind, sr_cam_rebind())
SR_DIFF_WRAP3(game_start_race_cam_chase, sr_cam_chase())

void lifted_geo_view_matrix_dir_emit(u32 a0, u32 a1, u32 a2);
void geo_view_matrix_dir_emit(u32 a0, u32 a1, u32 a2) {
    const u32 link = (u32)g14;
    SR_DIFF_BEGIN();
    lifted_geo_view_matrix_dir_emit(a0, a1, a2);
    SR_DIFF_NATIVE();
    sr_cam_light_dir(a0, a1, a2, link);
    SR_DIFF_END("geo_view_matrix_dir_emit");
}

SR_DIFF_WRAP3(game_start_race_cam_desert, sr_cam_desert())
SR_DIFF_WRAP3(game_start_race_cam_chase_alt, sr_cam_chase_alt())

void lifted_geo_attract_fp_series(u32 a0, u32 a1, u32 a2);
void geo_attract_fp_series(u32 a0, u32 a1, u32 a2) {
    u32 out[3], l0, l1, l2;
    SR_DIFF_BEGIN();
    lifted_geo_attract_fp_series(a0, a1, a2);
    l0 = (u32)g0;
    l1 = (u32)g1;
    l2 = (u32)g2;
    SR_DIFF_NATIVE();
    sr_spline_point(a0, a1, out);
    SR_DIFF_RET(l0 ^ (l1 * 3u) ^ (l2 * 7u), out[0] ^ (out[1] * 3u) ^ (out[2] * 7u));
    SR_DIFF_END("geo_attract_fp_series");
    g0 = l0;
    g1 = l1;
    g2 = l2;
}

void lifted_geo_view_matrix_row_swap_upload(u32 a0, u32 a1, u32 a2);
void geo_view_matrix_row_swap_upload(u32 a0, u32 a1, u32 a2) {
    uint8_t *m = sr_wrap_host_ptr((void *)(uintptr_t)g0);
    SR_DIFF_BEGIN();
    if(sr_wrap_is_host((void *)(uintptr_t)g0)) SR_DIFF_EXTRA(m, 0x30);
    lifted_geo_view_matrix_row_swap_upload(a0, a1, a2);
    SR_DIFF_NATIVE();
    sr_matrix_row_swap(m);
    SR_DIFF_END("geo_view_matrix_row_swap_upload");
}
