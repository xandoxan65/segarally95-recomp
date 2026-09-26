/* PNG export for sys24 framebuffer via libpng. */

#include "sys24_png_write.h"

#include <stdio.h>
#include <stdlib.h>

#ifndef I960_HOST_HAVE_PNG

int sys24_write_png_rgb32(const char *path, const u32 *bitmap, int w, int h)
{
    (void)path;
    (void)bitmap;
    (void)w;
    (void)h;
    fprintf(stderr, "lift: PNG support not built (install libpng and reconfigure)\n");
    return -1;
}

#else

#include <png.h>

int sys24_write_png_rgb32(const char *path, const u32 *bitmap, int w, int h)
{
    FILE *fp = NULL;
    png_structp png = NULL;
    png_infop info = NULL;
    u8 *row = NULL;
    int y;
    int rc = -1;

    if (!path || !bitmap || w <= 0 || h <= 0)
        return -1;

    fp = fopen(path, "wb");
    if (!fp)
        return -1;

    png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png)
        goto out;

    info = png_create_info_struct(png);
    if (!info)
        goto out;

    if (setjmp(png_jmpbuf(png)))
        goto out;

    png_init_io(png, fp);
    png_set_IHDR(png, info, (png_uint_32)w, (png_uint_32)h, 8, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    row = (u8 *)malloc((size_t)w * 3u);
    if (!row)
        goto out;

    for (y = 0; y < h; y++) {
        int x;
        const u32 *src = bitmap + (size_t)y * (size_t)w;

        for (x = 0; x < w; x++) {
            u32 c = src[x];

            row[(size_t)x * 3u + 0u] = (u8)((c >> 16) & 0xffu);
            row[(size_t)x * 3u + 1u] = (u8)((c >> 8) & 0xffu);
            row[(size_t)x * 3u + 2u] = (u8)(c & 0xffu);
        }
        png_write_row(png, row);
    }

    png_write_end(png, info);
    rc = 0;

out:
    free(row);
    if (png) {
        if (info)
            png_destroy_write_struct(&png, &info);
        else
            png_destroy_write_struct(&png, NULL);
    }
    if (fp)
        fclose(fp);
    return rc;
}

#endif /* I960_HOST_HAVE_PNG */
