/* Presentation the race phases (code/race/race_phases.c) trigger but the native port does not run itself: the CGM
 * catalogs of the 2D layer (glyph uploads) and the host's analog controls. The HUD frame, the scene text rows and
 * the tile map clears are ported (race/hud.h). The diff build
 * implements each hook by calling the lifted routine (tests/diff/wrap_race_phases.c), so the RAM it writes still
 * compares; the Dreamcast build has empty stubs (code/hooks/phase_hooks.c) until the HUD and 2D layer exist. */
#ifndef SR_RACE_PHASE_HOOKS_H
#define SR_RACE_PHASE_HOOKS_H
#include <stdint.h>

/* catalog_draw_setup @ 0x29EB0: CGM catalog at `catalog` (main data), entry `glyph` (g3), layer (g4); returns the
 * batch index it used (the colorbase seed stores it at 0x215c24). The Dreamcast stub returns 0. */
uint32_t sr_hook_catalog_draw(uint32_t x, uint32_t y, uint32_t catalog, uint32_t glyph, uint32_t layer);

/* The host's analog channels at rest: steer 0x80, accel and brake 0 (slot 0's recock, `all` = 0), or with all
 * other channels 0 as well (the practice idle wheel before GO, `all` = 1). Only the board: RAM is the port's. */
void sr_hook_analog_idle(uint32_t all);

#endif
