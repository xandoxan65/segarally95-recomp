/* RGB framebuffer PNG export (libpng). */
#ifndef SYS24_PNG_WRITE_H
#define SYS24_PNG_WRITE_H

#include "i960_lift.h"

/* Write bitmap[w*h] RGBA32 (0xAARRGGBB) as 24-bit PNG. Returns 0 on success. */
int sys24_write_png_rgb32(const char *path, const u32 *bitmap, int w, int h);

#endif
