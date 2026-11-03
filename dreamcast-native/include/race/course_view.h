/* The course section selection of the race frame (code/race/course_view.c): which stretch of course is drawn,
 * the in-front/behind flag 0x2020a4, the section origin 0x214360..0x214368 that the player's scene update reads,
 * and the GEO polygon budget. The drawing itself goes through race/course_hooks.h.
 *
 * Cells:
 *   0x2020a4  1 when the course direction at the car's section points away from the view (selects tables)
 *   0x214344  polygon budget for the course catalogs (prg_budget, then + 0x20b940 in the race frame)
 *   0x214348, 0x20aac4, 0x20aac8  prg_budget's other results
 *   0x214360/64/68  section origin (x, 0, z) of the car's road section (dir table of the course list)
 *   0x20b940  polygons queued this frame (geo_fifo_bootstrap clears it at the start of the race frame)
 *   0x20a290  TGP parameter list cursor (buffer RAM 0x918000..): each matrix bind (TGP 0x05) takes a slot */
#ifndef SR_RACE_COURSE_VIEW_H
#define SR_RACE_COURSE_VIEW_H
#include <stdint.h>

/* 0x1C450 game_start_race_prg_budget: the course polygon budget 0x214344 from the per-course section tables
 * (0x5dc9a0 / 0x5dc9c0 by 0x2020a4, + 0x5dc9e0 unless 0x2139d8), the depth list 0x2139f0 (whose +4 words it
 * sets to 0/1/2) and the mode 0x202230; also 0x214348, 0x20aac4, 0x20aac8. */
void sr_race_prg_budget(void);

/* The race frame's statements before the view bind (0x1BCxx, game_start_race_frame.c lines "acc = a + b" ..
 * the store): acc = [0x214344] + [0x20b940], capped at 0xbb8 when [0x2139d8] != 0, stored to 0x214344. */
uint32_t sr_course_budget_accumulate(void);

/* 0x3BE30 geo_attract_course_view_bind. course = the course index, pos = [0x2140c8] (the player's road cell),
 * budget = sr_course_budget_accumulate()'s value. Writes 0x214360..68 and 0x2020a4 (from the current TGP matrix:
 * the caller must have the camera's view loaded), binds the view matrix (TGP 0x05, a 0x20a290 slot), then the
 * section's catalogs (prg_mode, catalog_span). */
void sr_course_view_bind(uint32_t course, uint32_t pos, uint32_t budget);

/* The race frame's block from "acc = ..." to course_view_bind, in its order: sr_course_budget_accumulate then
 * sr_course_view_bind(course, [0x2140c8], acc). The reference restores the latched camera view on the TGP just
 * before (model2_hw_restore_latched_view_matrix); the caller does that. */
void sr_course_view_frame(uint32_t course);

/* Callees, checked on their own too. */
void sr_course_prg_mode(uint32_t course, uint32_t span);                                         /* 0x3B9B0 */
/* rec_va = 0x5dca00 + course*12; span_b = +1 / -1 (direction); budget, flag: the original's g3, g4. */
void sr_course_catalog_span(uint32_t rec_va, uint32_t span_a, uint32_t span_b, uint32_t budget, uint32_t flag); /* 0x3B7C0 */
void sr_course_marker_burst(void);                                                                /* 0x32410 */
/* start_ix, ix: the original's g3, g4 (catalog_span's first and end list index). */
void sr_course_span_table(uint32_t course, uint32_t span_b, uint32_t list_len, uint32_t start_ix, uint32_t ix); /* 0x47920 */

#endif
