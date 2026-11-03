/* The race's sub-states other than the race body (code/race/race_phases.c): setup and the bonus-time blend before
 * the countdown (slots 0 and 1), the countdown 3-2-1-GO (slot 2), the course complete (slot 4) and time-up
 * (slot 5) fanfares, and the race GEO PRG slot that runs the countdown and fanfare handlers once a tick.
 * Lifted from game_start_race_slot0..5 (table 0x1CA10, dispatched by 0x2020a8 >> 1 at 0x1CA30) and their callees.
 * All state stays in RAM at the original's addresses; HUD and 2D drawing go through race/phase_hooks.h, GEO
 * register and FIFO writes through race/course_hooks.h, sound through race/sound_cmd.h. */
#ifndef SR_RACE_PHASES_H
#define SR_RACE_PHASES_H
#include <stdint.h>

#define RP_PRG_SLOT      0x20ab50u /* race GEO PRG handler (staged ROM address), run once a tick */
#define RP_COUNTDOWN_T   0x20ab54u /* countdown clock (float): +1 per tick to 166 in slot 2 phase 2, then +0.5 */
#define RP_FANFARE_T     0x20ab60u /* fanfare ticks */
#define RP_SCENE         0x2020acu /* scene table index: the fanfares leave the race by advancing it */

/* Staged PRG handlers (ROM address + 0x59f000). */
#define RP_PRG_HUD       0x5c0330u /* 0x21330: HUD frame */
#define RP_PRG_HUD_GO    0x5c0340u /* 0x21340: HUD frame after GO */
#define RP_PRG_COUNTDOWN 0x5c01f0u /* 0x211F0 */
#define RP_PRG_FINISH    0x5bf790u /* 0x20790: not lifted, a no-op in the reference */

void sr_race_slot0(void);              /* 0x1CA70: race setup, one tick */
void sr_race_slot1(void);              /* 0x1CD20: bonus time blend before the countdown */
void sr_race_slot2(void);              /* 0x1CE30: countdown phases, GO */
void sr_race_slot4(void);              /* 0x1D680: course complete */
void sr_race_slot5(void);              /* 0x1D8D0: time up */
/* 0x1C9C0: sub-state 0 before the first race frame (from the attract logo path, not the race tick). `link` is
 * the caller's g14, which the colorbase seed (0x40790) stores at 0x215c20. */
void sr_race_sub_reset(uint32_t link);

/* 0x21370: handler != 0 installs it; 0 runs the installed one, then pushes the 0x202270 pair to the GEO. */
void sr_race_prg_slot(uint32_t handler);
void sr_countdown_prg(void);           /* 0x211F0 */

/* The countdown's 3D objects of the last tick (the digits 3-2-1 and GO: GEO quads 0x82c6fc..0x82c72f) with the TGP
 * matrix each was drawn by, in the start lights' GEO window: focus 512 on the 496 x 384 screen, centre (248, 192),
 * view +y up. Not RAM state: for the renderer. */
typedef struct {
    uint32_t oba;
    float m[12];
} sr_countdown_draw_t;
uint32_t sr_countdown_draws(const sr_countdown_draw_t **out);
/* 0x3F9A0 (start lights, 5 records from 0x5c0180, keyframes at 0x205ba20): the new countdown clock. */
uint32_t sr_countdown_object_step(uint32_t table, uint32_t t, uint32_t step, uint32_t limit, uint32_t mesh_base,
                                  uint32_t mode, uint32_t count);
void sr_fanfare_practice_win(void);    /* 0x20CF0 */
void sr_fanfare_practice_dnf(void);    /* 0x20E70 */
void sr_fanfare_champ_dnf(void);       /* 0x20FF0 */
void sr_fanfare_champ_win_0(uint32_t slot); /* 0x20880; slot: the PRG slot address (0x20ab50) */
void sr_fanfare_champ_win_0_cont(void);     /* 0x207C0 */
void sr_fanfare_champ_win_2(uint32_t slot); /* 0x20A50 */
void sr_fanfare_champ_win_2_cont(void);     /* 0x20990 */
void sr_fanfare_champ_win_3(void);          /* 0x20BF0 */

/* Callees. */
void sr_rank_list_seed(uint32_t dest); /* 0x47330: checkpoint pairs at dest, 0x2020c4 rows */
void sr_lap_bank_clear(uint32_t clock); /* 0x1C830: lap tables, then 0x20b0b0 = clock */
void sr_lap_bank_seed(void);           /* 0x18248 */
void sr_pose_ring_init(void);          /* 0x460E8 */
void sr_gate_bind(void);               /* 0x22768 */
void sr_hud_row_seed(void);            /* 0x1E498 */
void sr_race_sound_mute(void);         /* 0x1C7A0 */

/* Host reference workaround (lift_skip_practice.c, i960_host_skip_practice_idle_wheel): in the scripted practice,
 * hold the wheel idle until GO. The race frame runs it before the camera. */
void sr_race_idle_wheel(void);

#endif
