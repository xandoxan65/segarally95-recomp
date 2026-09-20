/* Lift → model2_geo FIFO + hardware window bindings. */

#include "model2_geo_lift.h"
#include "model2_hw.h"
#include "model2_memory.h"
#include "model2_rom.h"

#include <string.h>

void model2_geo_fifo_ops_from_lift(model2_geo_fifo_ops_t *out)
{
    if (!out)
        return;
    memset(out, 0, sizeof(*out));
    out->prg_total = model2_hw_prg_total;
    out->copro_total = model2_hw_copro_total;
    out->display_gen = model2_hw_prg_display_gen;
    out->display_range = model2_hw_prg_display_range;
    out->copy_prg_range = model2_hw_prg_copy_range;
    out->copy_mtx_range = model2_hw_prg_matrix_copy_range;
    out->copy_prg = model2_hw_prg_copy;
    out->copy_mtx = model2_hw_prg_matrix_copy;
    out->copy_copro_range = model2_hw_copro_copy_range;
    out->projection = model2_hw_geo_projection;
    out->shading = model2_hw_geo_shading;
    out->set_notify = model2_hw_set_fifo_notify;
}

static u8 *lift_textureram0(void)
{
    return model2_ram_mut(TEXTURERAM0_BASE);
}

static u8 *lift_textureram1(void)
{
    return model2_ram_mut(TEXTURERAM1_BASE);
}

static const u8 *lift_texture_sheet_rom(unsigned bank)
{
    return model2_rom_at(bank ? TEXTURE_SHEET_BANK1_VADDR : TEXTURE_SHEET_BANK0_VADDR);
}

void model2_geo_hw_ops_from_lift(model2_geo_hw_ops_t *out)
{
    if (!out)
        return;
    memset(out, 0, sizeof(*out));
    out->palram = model2_palram_ptr;
    out->colorxlat = model2_colorxlat_ptr;
    out->lumaram = model2_lumaram_ptr;
    out->textureram0 = lift_textureram0;
    out->textureram1 = lift_textureram1;
    out->texture_sheet_rom = lift_texture_sheet_rom;
}

int model2_geo_init_from_lift(void)
{
    model2_geo_fifo_ops_t fifo;
    model2_geo_hw_ops_t hw;

    model2_geo_fifo_ops_from_lift(&fifo);
    model2_geo_hw_ops_from_lift(&hw);
    model2_geo_bind_hw(&hw);
    return model2_geo_init(&fifo);
}
