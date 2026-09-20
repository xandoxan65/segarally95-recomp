/* Optional live-viewer recording → ffmpeg pipe (MJPEG AVI / H.264). */
#ifndef SYS24_VIEWER_RECORD_H
#define SYS24_VIEWER_RECORD_H

#include "i960_lift.h"

#include <stddef.h>

/* Start recording to path (".avi" → mjpeg, else libx264 mp4). Returns 0 ok, -1 fail. */
int sys24_viewer_record_start(const char *path, int width, int height, int fps);

/* True while a writer thread is live. */
int sys24_viewer_record_active(void);

/*
 * Queue one packed RGB24 frame (nbytes = width*height*3) for async write.
 * Never blocks on encode: if the prior frame is still flushing, this frame is
 * dropped (keeps attract vsync latency low).
 */
void sys24_viewer_record_submit_bgra(const void *pixels, size_t nbytes);

/* Stop writer, close ffmpeg (finalize container). Safe if never started. */
void sys24_viewer_record_stop(void);

#endif
