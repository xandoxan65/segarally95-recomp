/* System-24 char gfx decode — MAME gfx_element::decode for char_layout.
 *
 * Applies layout_xormask from set_gfx(..., NATIVE_ENDIAN_VALUE_LE_BE(8,0), ...).
 */
#ifndef SYS24_GFX_H
#define SYS24_GFX_H

#include "i960_lift.h"

/* 8×8 4bpp planar tile; charincrement = 256 bits = 32 bytes (segas24_tile_device::char_layout). */
u8 sys24_gfx_pixel(const u8 *char_ram, u16 code, int px, int py);

#endif
