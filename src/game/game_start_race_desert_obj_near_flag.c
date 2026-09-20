/* Desert race object near-flag @ 0x42190 — TGP 0x50 vs cam object @ 0x213980.
 *
 * ABI: g0 = node. Compare node+0x18/1c/20 to cam XYZ; if dist < 3 set bit0
 * on node+0x58 (else clear). tile_texture_descriptor_apply on transitions.
 *
 * source: /tmp/dasm_42190b/maincpu_042190_120.asm */
// @rom 0x42190 +0xdc game_start_race_desert_obj_near_flag

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "lift_syms.h"

void game_start_race_desert_obj_near_flag(u32 node, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    u32 cam;
    u32 ax, ay, az;
    u32 cx, cy, cz;
    u32 dist;
    u32 flags;
    double lim = i960_rifl_read(0, 0x40418000u); /* 3.0 */

    (void)arg1;
    (void)arg2;

    sp = sp + 0x10;
    g14 = 0;

    cam = i960_ld_u32(I960_WORKRAM, 0x213980, 0);
    ax = i960_ld_u32(I960_ABS, node, 0x18);
    ay = i960_ld_u32(I960_ABS, node, 0x1c);
    az = i960_ld_u32(I960_ABS, node, 0x20);
    cx = i960_ld_u32(I960_ABS, cam, 0);
    cy = i960_ld_u32(I960_ABS, cam, 4);
    cz = i960_ld_u32(I960_ABS, cam, 8);

    i960_mmio_write_u32(0x884000, 0x28005050u);
    i960_mmio_write_u32(0x884000, ax);
    i960_mmio_write_u32(0x884000, ay);
    i960_mmio_write_u32(0x884000, az);
    i960_mmio_write_u32(0x884000, cx);
    i960_mmio_write_u32(0x884000, cy);
    i960_mmio_write_u32(0x884000, cz);
    dist = i960_mmio_read_u32(0x884000);

    flags = i960_ld_u8(I960_ABS, node, 0x58);
    /* cmprl dist,3.0 then ldob (no CC change) then bge. */
    if (i960_u32_to_f64(dist) < lim) {
        if ((flags & 1u) == 0u)
            tile_texture_descriptor_apply(0x97u, 0x7fu, 0);
        flags = i960_ld_u8(I960_ABS, node, 0x58);
        i960_st_u8(I960_ABS, node, 0x58, (u8)(flags | 1u));
    } else {
        if (flags & 1u)
            tile_texture_descriptor_apply(0x97u, 0, 0);
        flags = i960_ld_u8(I960_ABS, node, 0x58);
        i960_st_u8(I960_ABS, node, 0x58, (u8)(flags & ~1u));
    }

    fp = fp_save;
    sp = sp_save;
}
