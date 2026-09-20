/* Live System-24 framebuffer preview (SDL2). OpenGL compositing deferred for geo. */
#ifndef SYS24_VIEWER_H
#define SYS24_VIEWER_H

#include "i960_lift.h"

/* True when I960_HOST_LIVE_VIEW is set (or lift --live). */
int sys24_viewer_wanted(void);

/* Open window + texture. Returns 0 on success, -1 on error. No-op when not wanted. */
int sys24_viewer_open(const char *title);

/* Composite tile RAM → texture and present. Returns 1 if user closed the window. */
int sys24_viewer_present(const u8 *tile_map, const u8 *char_ram, const u8 *palram);

/* Poll SDL quit/escape; Space toggles pause (flip blocks until resume).
 * Returns 1 if user closed the window. */
int sys24_viewer_poll_events(void);

/* Composite + present once (call from geo_vsync_wait after frame sync). */
int sys24_viewer_flip(const u8 *tile_map, const u8 *char_ram, const u8 *palram);

void sys24_viewer_shutdown(void);

#endif
