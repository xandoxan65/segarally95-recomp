/* The race HUD (code/race/hud.c) against the lifted originals (ported/hud.list). The tile map is compared with the
 * RAM images (sr_diff.c). draw_scene and erase_scene take the glyph and flags in g3 and g4, as the original. Hooks
 * and helpers change g0..g7, so each wrapper keeps the registers the original left. */
#include "wrap_common.h"
#include "race/hud.h"

typedef struct {
    uintptr_t g[16], fp, sp;
} regs_t;

static void regs_save(regs_t *r) {
    r->g[0] = g0; r->g[1] = g1; r->g[2] = g2; r->g[3] = g3;
    r->g[4] = g4; r->g[5] = g5; r->g[6] = g6; r->g[7] = g7;
    r->g[8] = g8; r->g[9] = g9; r->g[10] = g10; r->g[11] = g11;
    r->g[12] = g12; r->g[13] = g13; r->g[14] = g14; r->g[15] = g15;
    r->fp = fp;
    r->sp = sp;
}

static void regs_load(const regs_t *r) {
    g0 = r->g[0]; g1 = r->g[1]; g2 = r->g[2]; g3 = r->g[3];
    g4 = r->g[4]; g5 = r->g[5]; g6 = r->g[6]; g7 = r->g[7];
    g8 = r->g[8]; g9 = r->g[9]; g10 = r->g[10]; g11 = r->g[11];
    g12 = r->g[12]; g13 = r->g[13]; g14 = r->g[14]; g15 = r->g[15];
    fp = r->fp;
    sp = r->sp;
}

#define HUD_WRAP(name, call)                                              \
    void lifted_##name(u32 a0, u32 a1, u32 a2);                          \
    void name(u32 a0, u32 a1, u32 a2) {                                  \
        regs_t regs_;                                                    \
        SR_DIFF_BEGIN();                                                 \
        lifted_##name(a0, a1, a2);                                       \
        regs_save(&regs_);                                               \
        SR_DIFF_NATIVE();                                                \
        (void)a0, (void)a1, (void)a2;                                    \
        call;                                                            \
        SR_DIFF_END(#name);                                              \
        regs_load(&regs_);                                               \
    }

HUD_WRAP(game_start_race_hud_frame, sr_hud_frame())
HUD_WRAP(game_start_race_hud_layers, sr_hud_layers(a0))
HUD_WRAP(game_start_race_hud_tach, sr_hud_tach())
HUD_WRAP(game_start_race_cam_matrix_long, sr_hud_cam_matrix())
HUD_WRAP(game_start_race_hud_lap_time, sr_hud_lap_time(a0))
HUD_WRAP(game_start_race_hud_position, sr_hud_position(a0))
HUD_WRAP(game_start_race_hud_speed, sr_hud_speed(a0))
HUD_WRAP(game_start_race_hud_progress, sr_hud_progress(a0))
HUD_WRAP(tile_map_banks_clear, sr_tile_banks_clear())

#define SCENE_WRAP(name, native)                                          \
    u32 lifted_##name(u32 a0, u32 a1, u32 a2);                           \
    u32 name(u32 a0, u32 a1, u32 a2) {                                   \
        const uint32_t glyph = (uint32_t)g3, flags = (uint32_t)g4;       \
        regs_t regs_;                                                    \
        u32 r;                                                           \
        SR_DIFF_BEGIN();                                                 \
        r = lifted_##name(a0, a1, a2);                                   \
        regs_save(&regs_);                                               \
        SR_DIFF_NATIVE();                                                \
        SR_DIFF_RET(r, native(a0, a1, a2, glyph, flags));                \
        SR_DIFF_END(#name);                                              \
        regs_load(&regs_);                                               \
        return r;                                                        \
    }

SCENE_WRAP(draw_scene_dispatch, sr_draw_scene)
SCENE_WRAP(erase_scene_dispatch, sr_erase_scene)

void lifted_boot_tile_map_fill(u32 x, u32 y, u32 width, u32 height);
void boot_tile_map_fill(u32 x, u32 y, u32 width, u32 height) {
    regs_t regs_;
    SR_DIFF_BEGIN();
    lifted_boot_tile_map_fill(x, y, width, height);
    regs_save(&regs_);
    SR_DIFF_NATIVE();
    sr_tile_map_fill(x, y, width, height);
    SR_DIFF_END("boot_tile_map_fill");
    regs_load(&regs_);
}
