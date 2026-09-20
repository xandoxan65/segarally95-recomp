/* System-24 char gfx decode — MAME gfx_element / segas24_tile_device::char_layout.
 *
 * license: BSD-3-Clause (MAME drawgfx.cpp / segaic24.cpp, Olivier Galibert)
 *
 * char_layout @ segas24_tile_device:
 *   planes {0,1,2,3}, x STEP8(0,4), y STEP8(0,32), charincrement 8*32 bits.
 *
 * set_gfx(..., NATIVE_ENDIAN_VALUE_LE_BE(8,0), ...) → layout_xormask 8 on LE hosts.
 * decode: readbit(src, (yoffs + xoffset[x]) ^ layout_xormask)
 */

#include "sys24_gfx.h"

#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) \
    && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define SYS24_GFX_XORMASK 0u
#else
#define SYS24_GFX_XORMASK 8u
#endif

/* drawgfx.cpp readbit */
static int sys24_gfx_readbit(const u8 *src, unsigned int bitnum)
{
    return src[bitnum / 8u] & (0x80u >> (bitnum % 8u));
}

u8 sys24_gfx_pixel(const u8 *char_ram, u16 code, int px, int py)
{
    static const u32 planeoffset[4] = { 0u, 1u, 2u, 3u };
    static const u32 xoffset[8] = { 0u, 4u, 8u, 12u, 16u, 20u, 24u, 28u };
    static const u32 yoffset[8] = { 0u, 32u, 64u, 96u, 128u, 160u, 192u, 224u };
    const u32 charincrement = 256u;
    u8 color = 0;
    int plane;
    u8 planebit;

    if (!char_ram || px < 0 || px > 7 || py < 0 || py > 7)
        return 0;

    for (plane = 0, planebit = (u8)(1u << 3); plane < 4; plane++, planebit >>= 1) {
        u32 bitnum = (u32)code * charincrement + planeoffset[plane] + yoffset[py] + xoffset[px];

        if (sys24_gfx_readbit(char_ram, bitnum ^ SYS24_GFX_XORMASK))
            color = (u8)(color | planebit);
    }
    return color;
}
