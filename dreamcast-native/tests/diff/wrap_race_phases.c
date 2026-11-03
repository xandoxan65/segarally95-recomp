/* Race phase routines under their lifted names (ported/race_phases.list): run the original, then the port from the
 * same state, compare. Also the diff build's phase_hooks.h: each hook calls the lifted presentation routine, so the
 * RAM it writes compares too.
 *
 * The lifted code passes values in g0..g15 and the hooks run lifted code, so each wrapper keeps the registers the
 * original left and puts them back after the port. */
#include "wrap_common.h"
#include "race/race_phases.h"
#include "race/phase_hooks.h"
#include "lift_syms.h"

#include <stdio.h>
#include <stdlib.h>

/* ---- hooks: the lifted presentation routines ---- */

uint32_t sr_hook_catalog_draw(uint32_t x, uint32_t y, uint32_t catalog, uint32_t glyph, uint32_t layer) {
    g0 = x;
    g1 = y;
    g2 = catalog;
    g3 = glyph;
    g4 = layer;
    return catalog_draw_setup(x, y, catalog);
}

void sr_hook_analog_idle(uint32_t all) {
    unsigned i;
    if(all)
        for(i = 0; i < 8; i++) model2_io_analog_set(i, 0);
    model2_io_analog_set(MODEL2_IO_AN_STEER, 0x80u);
    model2_io_analog_set(MODEL2_IO_AN_ACCEL, 0);
    model2_io_analog_set(MODEL2_IO_AN_BRAKE, 0);
}

/* ---- wrappers ---- */

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

/* Original, then `call` with the original's registers restored afterwards. `in` is evaluated before the original
 * runs (register arguments). */
#define RP_WRAP(name, in, call)                                           \
    void lifted_##name(u32 a0, u32 a1, u32 a2);                          \
    void name(u32 a0, u32 a1, u32 a2) {                                  \
        regs_t regs_;                                                    \
        in;                                                              \
        SR_DIFF_BEGIN();                                                 \
        lifted_##name(a0, a1, a2);                                       \
        regs_save(&regs_);                                               \
        SR_DIFF_NATIVE();                                                \
        call;                                                            \
        SR_DIFF_END(#name);                                              \
        regs_load(&regs_);                                               \
    }

RP_WRAP(game_start_race_slot0, (void)0, sr_race_slot0())
RP_WRAP(game_start_race_slot1, (void)0, sr_race_slot1())
RP_WRAP(game_start_race_slot2, (void)0, sr_race_slot2())
RP_WRAP(game_start_race_slot4, (void)0, sr_race_slot4())
RP_WRAP(game_start_race_slot5, (void)0, sr_race_slot5())
RP_WRAP(game_start_race_sub_reset, const u32 link = (u32)g14, sr_race_sub_reset(link))
/* SR_DIFF_FORCE_FINISH=N (diff build only): from race tick N, turn the race body into the course-complete state
 * (0x2020a8 = 8) once, before that tick's PRG slot, so slot4 and the practice win fanfare get checked although no
 * scripted drive finishes the three laps in time. A synthetic state: the lap and record bookkeeping of a real
 * finish (slot3 @ 0x1D31C) is skipped; slot4 does not read it. */
extern int i960_host_race_tick_now(void);
static void force_finish(void) {
    static long at = -2;
    if(at == -2) {
        const char *s = getenv("SR_DIFF_FORCE_FINISH");
        at = s && *s ? strtol(s, NULL, 0) : -1;
    }
    if(at >= 0 && i960_host_race_tick_now() >= at && (sr_ld32(0x2020a8u) >> 1) == 3) {
        sr_st32(0x2020a8u, 8);
        fprintf(stderr, "sr_diff: forced the finish at race tick %d\n", i960_host_race_tick_now());
        at = -1;
    }
}

RP_WRAP(game_start_race_geo_prg_slot, if(a0 == 0) force_finish(), sr_race_prg_slot(a0))
RP_WRAP(game_start_race_countdown_prg, (void)0, sr_countdown_prg())
RP_WRAP(game_start_race_fanfare_practice_win, (void)0, sr_fanfare_practice_win())
RP_WRAP(game_start_race_fanfare_practice_dnf, (void)0, sr_fanfare_practice_dnf())
RP_WRAP(game_start_race_fanfare_champ_dnf, (void)0, sr_fanfare_champ_dnf())
RP_WRAP(game_start_race_fanfare_champ_win_0, (void)0, sr_fanfare_champ_win_0(a0))
RP_WRAP(game_start_race_fanfare_champ_win_0_cont, (void)0, sr_fanfare_champ_win_0_cont())
RP_WRAP(game_start_race_fanfare_champ_win_2, (void)0, sr_fanfare_champ_win_2(a0))
RP_WRAP(game_start_race_fanfare_champ_win_2_cont, (void)0, sr_fanfare_champ_win_2_cont())
RP_WRAP(game_start_race_fanfare_champ_win_3, (void)0, sr_fanfare_champ_win_3())
/* rank_list_seed and lap_bank_clear take their value from g0 when the argument is 0. */
RP_WRAP(game_start_race_rank_list_seed, const u32 dest = a0 ? a0 : (u32)g0, sr_rank_list_seed(dest))
RP_WRAP(game_start_race_lap_bank_clear, const u32 clock = a0 ? a0 : (u32)g0, sr_lap_bank_clear(clock))
RP_WRAP(game_start_race_lap_bank_seed, (void)0, sr_lap_bank_seed())
RP_WRAP(geo_workram_pose_ring_init, (void)0, sr_pose_ring_init())
RP_WRAP(game_start_race_gate_bind, (void)0, sr_gate_bind())
RP_WRAP(game_start_race_hud_row_seed, (void)0, sr_hud_row_seed())
RP_WRAP(game_start_race_sound_mute, (void)0, sr_race_sound_mute())

/* object_step: time step limit in g1..g3, keyframe base g4, mode g5, record count g6; the new time in g0. */
void lifted_geo_countdown_object_step(u32 a0, u32 a1, u32 a2);
void geo_countdown_object_step(u32 a0, u32 a1, u32 a2) {
    const u32 limit = (u32)g3, base = (u32)g4, mode = (u32)g5, count = (u32)g6;
    regs_t regs;
    u32 t;
    SR_DIFF_BEGIN();
    lifted_geo_countdown_object_step(a0, a1, a2);
    regs_save(&regs);
    SR_DIFF_NATIVE();
    t = sr_countdown_object_step(a0, a1, a2, limit, base, mode, count);
    SR_DIFF_RET((u32)regs.g[0], t);
    SR_DIFF_END("geo_countdown_object_step");
    regs_load(&regs);
}
