/* Race flow routines under their lifted names (ported/race_flow.list): run the original, then the port from the
 * same state, compare.
 *
 * The lifted code passes values in g0..g15 and the hooks run lifted code, so each wrapper keeps the registers the
 * original left and puts them back after the port. */
#include "wrap_common.h"
#include "race/race_flow.h"

#include <string.h>

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

/* Original, then `call` with the original's registers restored afterwards. */
#define RF_WRAP(name, call)                                               \
    void lifted_##name(u32 a0, u32 a1, u32 a2);                          \
    void name(u32 a0, u32 a1, u32 a2) {                                  \
        regs_t regs_;                                                    \
        SR_DIFF_BEGIN();                                                 \
        lifted_##name(a0, a1, a2);                                       \
        regs_save(&regs_);                                               \
        SR_DIFF_NATIVE();                                                \
        call;                                                            \
        SR_DIFF_END(#name);                                              \
        regs_load(&regs_);                                               \
    }

RF_WRAP(game_start_race_slot3, sr_race_slot3())
RF_WRAP(game_start_race_pose_ring_step, sr_pose_ring_step())
RF_WRAP(game_start_race_gate_walk, sr_gate_walk())
RF_WRAP(game_start_race_lap_tick, sr_lap_tick())
RF_WRAP(game_start_race_aea8_set, sr_race_finish_clock(a0))
RF_WRAP(game_start_race_lap_best, sr_lap_best())
RF_WRAP(game_start_race_obj_bind, sr_obj_bind(a0))
RF_WRAP(game_start_race_obj_publish, sr_obj_publish())
RF_WRAP(game_start_race_obj_sort_index, sr_obj_sort_index())
RF_WRAP(game_start_race_obj_rank_fill, sr_obj_rank_fill())
RF_WRAP(game_start_race_obj_depth_pass, sr_obj_depth_pass(a0))

/* lap_record and list_flag_prep take their value from g0 when the argument is 0; list_flag_prep also stores the
 * caller's g14 (its return address) as a list mode. */
void lifted_game_start_race_lap_record(u32 a0, u32 a1, u32 a2);
void game_start_race_lap_record(u32 a0, u32 a1, u32 a2) {
    const u32 time = a0 ? a0 : (u32)g0;
    regs_t regs;
    SR_DIFF_BEGIN();
    lifted_game_start_race_lap_record(a0, a1, a2);
    regs_save(&regs);
    SR_DIFF_NATIVE();
    sr_lap_record(time);
    SR_DIFF_END("game_start_race_lap_record");
    regs_load(&regs);
}

void lifted_game_start_race_list_flag_prep(u32 a0, u32 a1, u32 a2);
void game_start_race_list_flag_prep(u32 a0, u32 a1, u32 a2) {
    const u32 budget = a0 ? a0 : (u32)g0, link = (u32)g14;
    regs_t regs;
    SR_DIFF_BEGIN();
    lifted_game_start_race_list_flag_prep(a0, a1, a2);
    regs_save(&regs);
    SR_DIFF_NATIVE();
    sr_list_flag_prep(budget, link);
    SR_DIFF_END("game_start_race_list_flag_prep");
    regs_load(&regs);
}

/* list_unlink takes its node from g0 when the argument is 0 (the list walk passes it in g0). */
void lifted_game_start_race_list_unlink(u32 a0, u32 a1, u32 a2);
void game_start_race_list_unlink(u32 a0, u32 a1, u32 a2) {
    const u32 node = a0 ? a0 : (u32)g0;
    regs_t regs;
    SR_DIFF_BEGIN();
    lifted_game_start_race_list_unlink(a0, a1, a2);
    regs_save(&regs);
    SR_DIFF_NATIVE();
    sr_race_list_unlink(node);
    SR_DIFF_END("game_start_race_list_unlink");
    regs_load(&regs);
}

/* obj_scan returns its count in g0 (slot3 stores it at 0x2020c8). */
void lifted_game_start_race_obj_scan(u32 a0, u32 a1, u32 a2);
void game_start_race_obj_scan(u32 a0, u32 a1, u32 a2) {
    regs_t regs;
    u32 n;
    SR_DIFF_BEGIN();
    lifted_game_start_race_obj_scan(a0, a1, a2);
    regs_save(&regs);
    SR_DIFF_NATIVE();
    n = sr_obj_scan(a0);
    SR_DIFF_RET((u32)regs.g[0], n);
    SR_DIFF_END("game_start_race_obj_scan");
    regs_load(&regs);
}

/* span_test: self xyz in g1, other xyz in g3 (host pointers into obj_scan's frame), result in g0. */
void lifted_game_start_race_obj_span_test(u32 self_z, void *self_xyz, u32 other_z);
void game_start_race_obj_span_test(u32 self_z, void *self_xyz, u32 other_z) {
    const uint32_t *other = (const uint32_t *)g3;
    regs_t regs;
    u32 n;
    SR_DIFF_BEGIN();
    lifted_game_start_race_obj_span_test(self_z, self_xyz, other_z);
    regs_save(&regs);
    SR_DIFF_NATIVE();
    n = sr_obj_span_test(self_z, (const uint32_t *)self_xyz, other_z, other);
    SR_DIFF_RET((u32)regs.g[0], n);
    SR_DIFF_END("game_start_race_obj_span_test");
    regs_load(&regs);
}
