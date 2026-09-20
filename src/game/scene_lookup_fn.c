/* Scene range lookup @ 0x14788 (bal from game_start_display_setup).
 * Walks 0x5B375C[g0] range table vs 0x202050; returns matching index in g0.
 * source: disasm/maincpu/maincpu_014788_80.asm */
// @rom 0x14788 +0x70 scene_lookup_fn

#include "i960_lift.h"
#include "i960_mem.h"

#include "model2_rom.h"

static u32 staging_u32(u32 ea)
{
    u32 v = i960_ld_u32(I960_WORKRAM, ea, 0);

    /* Range tables live in the ROM→workram staging window; mirror if unset. */
    if (ea >= 0x005a0000u && ea < 0x00600000u) {
        u32 mir = model2_workram_mirror_u32(ea);

        if (v == 0)
            v = mir;
    }
    return v;
}

static u32 range_word(u32 ea)
{
    /* Bounds may be 0 — always take ROM mirror for staged range rows. */
    if (ea >= 0x005a0000u && ea < 0x00600000u)
        return model2_workram_mirror_u32(ea);
    return i960_ld_u32(I960_WORKRAM, ea, 0);
}

u32 scene_lookup_fn(u32 table_index, u32 mask_src)
{
    u32 idx = table_index;
    u32 mask = mask_src;
    u32 probe;
    u32 table_ptr;
    u32 entry;
    u32 level;
    u32 cont;
    u32 lo;
    u32 hi;
    u32 test;

    /* @0x14790 cmpi g1,g0; @0x14794 testl g4 — true when mask < idx. */
    test = ((i32)mask < (i32)idx) ? 0xffffffffu : 0u;
    level = (u32)i960_ld_u8(I960_WORKRAM, 0x202050, 0) & 0xffu;
    /* @0x147A0 subo g4,0,g4 → negate; @0x147AC and with g1. */
    test = (u32)(0u - test);
    probe = test & mask;

    table_ptr = staging_u32(0x005b375cu + (idx << 2));
    entry = table_ptr + (probe << 3);
    idx = probe;

    do {
        cont = 0;
        lo = range_word(entry);
        hi = range_word(entry + 4u);

        if (level < lo) {
            entry -= 8u;
            idx -= 1u;
            cont = 1;
        } else if (level > hi) {
            entry += 8u;
            idx += 1u;
            cont = 1;
        }
    } while (cont != 0);

    return idx;
}
