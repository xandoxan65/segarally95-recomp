/* Where the course section selection (code/race/course_view.c) hands drawing to the renderer: GEO register writes
 * and the GEO program FIFO (0x804000) words of the course catalogs. In the differential checks these write the
 * Model 2 ports as the lifted code does (tests/diff/wrap_course_view.c); on the Dreamcast the renderer takes them
 * (code/hooks/course_hooks.c, empty for now). */
#ifndef SR_RACE_COURSE_HOOKS_H
#define SR_RACE_COURSE_HOOKS_H
#include <stdint.h>

/* GEO bank poke: register 0x800010 + (bank << 10) <- 0, before each catalog (bank = [0x202278] & 3). */
void sr_hook_course_bank(uint32_t bank);
/* One word to the GEO program FIFO (the far-Z push 0x48c35000, the course word [0x5da1f0]). */
void sr_hook_course_prg(uint32_t w);
/* A catalog entry (4 words: model, texture, ... count) to the GEO program FIFO: one piece of course to draw. */
void sr_hook_course_catalog(uint32_t w0, uint32_t w1, uint32_t w2, uint32_t w3);
/* A GEO register write (0x800160 <- 0, 0x801008 <- next write pointer). */
void sr_hook_course_reg(uint32_t addr, uint32_t value);
/* GEO write pointer (register 0x802008). The original stores it into the TGP parameter list at [0x20a290] next to
 * each matrix bind (TGP 0x05). The Dreamcast stub returns 0. */
uint32_t sr_hook_course_write_ptr(void);

#endif
