/* The lockstep run on the Dreamcast (code/lockstep.c): a race pack from sr-native-run, one native race tick per
 * frame, checkpoint hashes and tick times printed when it ends. */
#ifndef SR_LOCKSTEP_H
#define SR_LOCKSTEP_H
#include <stdint.h>

/* Load the pack; 0 on success. */
int sr_lockstep_load(const char *path);
/* Run one race tick; returns 0 once the pack's ticks are done. */
int sr_lockstep_tick(void);
/* Print the checkpoints and the tick time figures. */
void sr_lockstep_report(void);
uint32_t sr_lockstep_race_tick(void);

/* The pack's 2D layer graphics (NULL when the pack has none): characters (8 x 8, 4 bits a pixel, 32 bytes each,
 * SR_CHAR_SIZE) and the palette (u16 RGB555, SR_PALETTE_SIZE); the tile map itself is sr_tile_map (race/hud.h). */
#define SR_CHAR_SIZE    0x80000u
#define SR_PALETTE_SIZE 0x4000u
const uint8_t *sr_lockstep_chars(void);
const uint8_t *sr_lockstep_palette(void);

/* Practice: the same pack's start state, driven by live controls. Keeps a copy of the start for restarts. */
void sr_session_keep_start(void);
void sr_session_restart(void);
void sr_session_tick(uint32_t steer, uint32_t accel, uint32_t brake, uint32_t gear);

/* RECORD=1 practice: a drive on the pad, recorded so that a demo pack replays it exactly. The tick runs as an
 * absolute pack's replay does (idle controls until GO, the controls at GO taken again by the poll), with the
 * live controls in place of the pack's rows; each change becomes a row "race_tick steer accel brake gear view"
 * (tools/make_practice_pack.sh INPUTS=... turns the file into a pack). sr_session_record_reset starts over (a
 * restart); sr_session_record_save writes the rows so far. */
void sr_session_record_tick(uint32_t steer, uint32_t accel, uint32_t brake, uint32_t gear, int view);
void sr_session_record_reset(void);
int sr_session_record_save(const char *path);

#endif
