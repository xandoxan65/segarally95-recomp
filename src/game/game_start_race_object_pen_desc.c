/* Secondary pen draw @ 0x248B0 — descriptor at g0.
 *
 * +4 mode: 0 → ret; 1 → body pen via 0x5c2780 → glyph_emit; else →
 * 0x5c2798 path (0x24560 / 0x240c0).
 *
 * source: disasm/maincpu/maincpu_0248b0_c0.asm */
// @rom 0x248b0 +0xc0 game_start_race_object_pen_desc

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "lift_syms.h"
#include "model2_rom.h"

void game_start_race_object_pen_desc(u32 arg0, u32 arg1, u32 arg2)
{
    u32 desc_va = arg0;
    u32 mode;
    u32 obj_va;
    u8 *obj;
    u8 flags;
    u32 pen_ix;
    u32 pen_addr;
    u32 scale_bits;

    (void)arg1;
    (void)arg2;

    if (desc_va == 0u)
        return;

    /* @0x248B0–0x248B4: mode @ +4, object VA @ +8. */
    mode = i960_ld_u32(I960_WORKRAM, desc_va, 4);
    obj_va = i960_ld_u32(I960_WORKRAM, desc_va, 8);
    if (mode == 0u)
        return;

    obj = model2_ram_mut(obj_va);
    if (!obj)
        return;

    if (mode == 1u) {
        /* @0x248C8–0x248F8: clear high pen bits, emit 0x5c2780[pen*3]. */
        flags = obj[0x50];
        pen_ix = flags & 15u;
        obj[0x50] = (u8)(flags & 0x9fu);
        pen_addr = 0x5c2780u + ((pen_ix + (pen_ix << 1)) << 4);
        g0 = pen_addr;
        g1 = (uintptr_t)obj;
        g2 = 0x270fu;
        geo_attract_glyph_emit((void *)(uintptr_t)pen_addr, obj, 0x270fu);
        return;
    }

    /*
     * @0x24900+: set bit5 on flags, pick 0x5c2798[pen*3], compare scale @ +0xc
     * vs 100.0 — 0x24560 (>) or 0x240c0 (<=).
     */
    flags = obj[0x50];
    obj[0x50] = (u8)((flags & 0x9fu) | 0x20u);
    pen_ix = flags & 15u;
    pen_addr = 0x5c2798u + ((pen_ix + (pen_ix << 1)) << 4);
    scale_bits = i960_ld_u32(I960_WORKRAM, desc_va, 0xc);
    g0 = pen_addr;
    g1 = (uintptr_t)obj;
    g2 = 0x270fu;
    fp0 = i960_u32_to_f64(scale_bits);
    fp1 = i960_rifl_read(0, 0x40590000u); /* 100.0 */
    if (fp0 > fp1)
        game_start_race_object_pen_far(pen_addr, obj_va, 0x270fu);
    else
        game_start_race_object_pen_mid(pen_addr, obj_va, 0x270fu);
}
