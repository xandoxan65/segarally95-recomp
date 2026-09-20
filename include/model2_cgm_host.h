#ifndef MODEL2_CGM_HOST_H
#define MODEL2_CGM_HOST_H

/*
 * GEO colorbase table @ 0x01802000 — lifted only:
 *   boot_palette_splash_upload @ 0x26758 (maincpu 0x5FB89E)
 *   palram_geo_colorbase_refresh @ 0x333C8 / mode5 @ 0x33744
 *
 * Race/attract CGM catalogs use catalog_draw_setup → cgm_leading_colorbase
 * (scratch @ 0x01800000+(slot<<5)); not a host-script path.
 */

#endif
