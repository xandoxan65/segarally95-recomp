/* Alt chase camera @ 0x1FF60 — staged callx 0x5BEF60.
 *
 * Installed by cam_rebind when 0x2140c0 != 0. Differs from chase @ 0x1FDC0:
 * builds an eye from blend/0x20ac74, emits yaw/pitch, then 0x26 →
 * geo_view_matrix_row_swap_upload @ 0x33A50, and sets 0x2139d8=1 so
 * object_pen runs again.
 *
 * source: disasm/maincpu/maincpu_01ff60_37c.asm */
// @rom 0x1ff60 +0x37c game_start_race_cam_chase_alt

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_hw.h"
#include "model2_rom.h"

#include "lift_syms.h"

#include <stdio.h>
#include <string.h>

static u32 f_neg(u32 bits)
{
    return bits ^ 0x80000000u;
}

void game_start_race_cam_chase_alt(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    u8 frame[0xb0];
    u32 obj;
    u32 px, py, pz;
    u32 yaw, pitch, roll;
    u32 blend;
    u32 half_yaw;
    u32 z_off;
    u32 pitch_cmd;
    unsigned i;
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    if (!logged) {
        lift_log( "lift: race_cam_chase_alt\n");
        fflush(stderr);
        logged = 1;
    }

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    sp = sp + 0x50u;

    obj = i960_ld_u32(I960_WORKRAM, 0x213980, 0);
    if (obj == 0u
        || (model2_ram_mut(obj) == NULL && model2_rom_at(obj) == NULL)) {
        sp = sp_save;
        fp = fp_save;
        return;
    }

    /* @0x1FF64–0x1FF98: pose → fp+0x40; angles → fp+0x50. */
    px = i960_ld_u32(I960_ABS, obj, 0);
    py = i960_ld_u32(I960_ABS, obj, 4);
    pz = i960_ld_u32(I960_ABS, obj, 8);
    *(u32 *)(fp + 0x40) = px;
    *(u32 *)(fp + 0x44) = py;
    *(u32 *)(fp + 0x48) = pz;

    yaw = i960_ld_u32(I960_ABS, obj, 0x18);
    pitch = i960_ld_u32(I960_ABS, obj, 0x1c);
    roll = i960_ld_u32(I960_ABS, obj, 0x20);
    *(u32 *)(fp + 0x50) = yaw;
    *(u32 *)(fp + 0x54) = pitch;
    *(u32 *)(fp + 0x58) = roll;

    /* blend = clamp(0x20ab5c + pitch); then 0x20ab5c − blend×0.05. */
    blend = (u32)i960_f64_to_u32(
        i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x20ab5c, 0))
        + i960_u32_to_f64(pitch));
    blend = geo_view_float_clamp(blend, 0, 0);
    blend = (u32)i960_f64_to_u32(
        i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x20ab5c, 0))
        - i960_u32_to_f64(blend)
              * i960_rifl_read(0x9999999au, 0x3fa99999u));
    i960_st_u32(I960_WORKRAM, 0x20ab5c, 0, blend);
    blend = geo_view_float_clamp(blend, 0, 0);

    /* half_yaw = yaw×0.5; z_off = 0x20ac74 + 5; pitch_cmd = 0.0698… − half_yaw. */
    half_yaw = (u32)i960_f64_to_u32(
        i960_u32_to_f64(yaw) * i960_rifl_read(0, 0x3fe00000u));
    z_off = (u32)i960_f64_to_u32(
        i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x20ac74, 0))
        + i960_rifl_read(0, 0x40140000u));
    pitch_cmd = (u32)i960_f64_to_u32(
        i960_rifl_read(0xa2525ff7u, 0x3fb1df46u)
        - i960_u32_to_f64(half_yaw));

    /* TGP 0x27(0, −1.3, z_off); 0x29(pitch_cmd); 0x2a(blend). */
    i960_mmio_write_u32(0x884000, 0x13802727u);
    i960_mmio_write_u32(0x884000, 0);
    i960_mmio_write_u32(0x884000, 0xbfa66666u);
    i960_mmio_write_u32(0x884000, z_off);
    i960_mmio_write_u32(0x884000, 0x14802929u);
    i960_mmio_write_u32(0x884000, pitch_cmd);
    i960_mmio_write_u32(0x884000, 0x15002a2au);
    i960_mmio_write_u32(0x884000, blend);

    i960_st_u32(I960_WORKRAM, 0x202270, 0, 0x43960000u);
    i960_st_u32(I960_WORKRAM, 0x202274, 0, 0x43960000u);
    i960_st_u32(I960_WORKRAM, 0x20ab5c, 0, blend);
    i960_mmio_write_u32(0x800090, 0);
    {
        u32 geo_a = 0x43960000u;
        u32 geo_b = 0x43960000u;

        i960_mmio_write_u32(0x804000, geo_a);
        i960_mmio_write_u32(0x804000, geo_b);
    }

    geo_view_matrix_dir_emit(0xbf34fdf4u, 0xbf34fdf4u, 0x3f34fdf4u);

    /* @0x20114: 0x27(−xyz); 0x26 → fp+0x60. */
    i960_mmio_write_u32(0x884000, 0x13802727u);
    i960_mmio_write_u32(0x884000, f_neg(px));
    i960_mmio_write_u32(0x884000, f_neg(py));
    i960_mmio_write_u32(0x884000, f_neg(pz));
    i960_mmio_write_u32(0x884000, 0x13002626u);
    for (i = 0; i < 12u; i++)
        *(u32 *)(fp + 0x60 + i * 4) = i960_mmio_read_u32(0x884000);

    g0 = fp + 0x60;
    geo_view_matrix_row_swap_upload((u32)g0, 0, 0);

    /* Reload R|T from ROM constants @ 0x5c7890 then 0x22 + 0x2c. */
    {
        u32 cx = i960_ld_u32(I960_WORKRAM, 0x5c7890, 0);
        u32 cy = i960_ld_u32(I960_WORKRAM, 0x5c7894, 0);
        u32 cz = i960_ld_u32(I960_WORKRAM, 0x5c7898, 0);

        i960_mmio_write_u32(0x884000, 0x10002020u);
        i960_mmio_write_u32(0x884000, 0x11002222u);
        for (i = 0; i < 4u; i++)
            i960_mmio_write_u32(0x884000, *(u32 *)(fp + 0x60 + i * 4));
        for (i = 0; i < 4u; i++)
            i960_mmio_write_u32(0x884000, *(u32 *)(fp + 0x70 + i * 4));
        for (i = 0; i < 4u; i++)
            i960_mmio_write_u32(0x884000, *(u32 *)(fp + 0x80 + i * 4));
        i960_mmio_write_u32(0x884000, 0x16002c2cu);
        i960_mmio_write_u32(0x884000, cx);
        i960_mmio_write_u32(0x884000, cy);
        i960_mmio_write_u32(0x884000, cz);
        px = i960_mmio_read_u32(0x884000);
        py = i960_mmio_read_u32(0x884000);
        pz = i960_mmio_read_u32(0x884000);
    }

    /*
     * Disasm @ 0x20290: 0x21 pop after the 0x0B row-swap upload. Latch the
     * composed view before pop — same host contract as chase @ 0x1FDC0 /
     * desert after 0x27 (GEO already received 0x0B from row_swap_upload).
     */
    model2_hw_latch_view_matrix();
    i960_mmio_write_u32(0x884000, 0x10802121u);
    /* Restore object_pen gate — chase clears it; alt sets it back. */
    i960_st_u32(I960_WORKRAM, 0x2139d8, 0, 1u);

    i960_st_u32(I960_WORKRAM, 0x20220c, 0, px);
    i960_st_u32(I960_WORKRAM, 0x202210, 0, py);
    i960_st_u32(I960_WORKRAM, 0x202214, 0, pz);
    i960_st_u32(I960_WORKRAM, 0x202200, 0, *(u32 *)(fp + 0x50));
    i960_st_u32(I960_WORKRAM, 0x202204, 0, *(u32 *)(fp + 0x54));
    i960_st_u32(I960_WORKRAM, 0x202208, 0, *(u32 *)(fp + 0x58));

    game_start_race_cam_rebind(0, 0, 0);

    sp = sp_save;
    fp = fp_save;
}
