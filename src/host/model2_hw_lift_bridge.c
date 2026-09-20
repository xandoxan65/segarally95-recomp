/* Bind lift board / viewer / geo uploads into stand-alone model2_hw. */

#include "model2_hw_lift.h"

#include "i960_host.h"
#include "i960_mem.h"
#include "lift_syms.h"
#include "model2_geo.h"
#include "model2_rom.h"
#include "sys24_viewer.h"

#include <string.h>

static u32 lift_workram_ld_u32(u32 vaddr)
{
    /* Same guest VA the i960 st'd — 0x52/0x07 blocks may sit in workram or
     * bufferram after GEO wr_slot advances 0x20a290. */
    return i960_ld_u32(I960_ABS, vaddr, 0);
}

static void lift_upload_texture(u32 address, u32 count, const u32 *data)
{
    model2_geo_upload_texture(address, count, data);
}

static void lift_upload_polygon(u32 address, u32 count, const u32 *data)
{
    model2_geo_upload_polygon(address, count, data);
}

static double lift_vsync_hz(void)
{
    return model2_tile_vsync_hz();
}

static int lift_display_wanted(void)
{
    return sys24_viewer_wanted();
}

static int lift_display_flip(void)
{
    return sys24_viewer_flip(model2_tile_map_ptr(), model2_tile_char_ptr(),
                             model2_palram_ptr());
}

static void lift_boot_vblank(void)
{
    if (i960_host_boot_screen())
        boot_tile_splash_frame(0, 0, 0);
}

void model2_hw_host_ops_from_lift(model2_hw_host_ops_t *out)
{
    if (!out)
        return;
    memset(out, 0, sizeof(*out));
    out->workram_ld_u32 = lift_workram_ld_u32;
    out->upload_texture = lift_upload_texture;
    out->upload_polygon = lift_upload_polygon;
    out->vsync_hz = lift_vsync_hz;
    out->display_wanted = lift_display_wanted;
    out->display_flip = lift_display_flip;
    out->boot_vblank = lift_boot_vblank;
    out->request_halt = i960_host_request_halt;
}

void model2_hw_init_from_lift(void)
{
    model2_hw_host_ops_t host;

    model2_hw_host_ops_from_lift(&host);
    model2_hw_bind_host(&host);
    model2_hw_reset();
}
