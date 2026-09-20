/* Attract CREDIT / free-play CGM HUD @ 0xC060.
 * mode==1: catalog seed into 0x20A4E0…0x20A4F0
 * else: draw CREDIT label + digits from 0x1D00020 / price 0x20202A
 * source: disasm/maincpu/maincpu_00c060_280.asm */
// @rom 0xc060 +0x300 comm_scene_catalog_init

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"

static u32 credit_catalog_setup(u32 x, u32 y, u32 catalog, u32 mode, u32 flags)
{
    g0 = x;
    g1 = y;
    g2 = catalog;
    g3 = mode;
    g4 = flags;
    return catalog_draw_setup(x, y, catalog);
}

static void credit_draw_glyph(u32 x, u32 y, u32 slot, u32 glyph, u32 flags)
{
    g0 = x;
    g1 = y;
    g2 = slot;
    g3 = glyph;
    g4 = flags;
    draw_scene_dispatch(x, y, slot);
}

static void credit_erase_glyph(u32 x, u32 y, u32 slot, u32 glyph, u32 flags)
{
    g0 = x;
    g1 = y;
    g2 = slot;
    g3 = glyph;
    g4 = flags;
    erase_scene_dispatch(x, y, slot);
}

void comm_scene_catalog_init(u32 mode, u32 arg1, u32 arg2)
{
    u32 slot;
    u32 credits;
    u32 price;
    u32 flags;
    u32 gate;

    (void)arg1;
    (void)arg2;

    /* @0xC060: cmpi g0,1 — keep mode in r4 for free-play branch later. */
    g0 = mode;
    if (mode == 1u) {
        slot = credit_catalog_setup(0, 0, 0x28ae89cu, 0, 4);
        i960_st_u32(I960_WORKRAM, 0x20a4e0, 0, slot);
        slot = credit_catalog_setup(1, 45, 0x288a114u, 0, 4);
        i960_st_u32(I960_WORKRAM, 0x20a4e4, 0, slot);
        slot = credit_catalog_setup(1, 45, 0x288ab90u, 0, 4);
        i960_st_u32(I960_WORKRAM, 0x20a4ec, 0, slot);
        slot = credit_catalog_setup(0, 0, 0x28898c4u, 0, 4);
        i960_st_u32(I960_WORKRAM, 0x20a4e8, 0, slot);
        slot = credit_catalog_setup(0, 0, 0x288b01cu, 0, 4);
        i960_st_u32(I960_WORKRAM, 0x20a4f0, 0, slot);
        return;
    }

    /* Update / draw path @ 0xC124 */
    flags = i960_ld_u8(I960_WORKRAM, 0x202024, 0);
    if ((flags & 8u) != 0)
        goto free_play;

    slot = i960_ld_u32(I960_WORKRAM, 0x20a4e4, 0);
    credit_draw_glyph(1, 45, slot, 0, 0);

    if (mode == 0u) {
        credits = i960_ld_u16(I960_ABS, 0x01d00020u, 0);
        g0 = i960_ld_u32(I960_WORKRAM, 0x20a4f0, 0);
        if (credits != 0)
            attract_credit_stamp_paid((u32)g0, 0, 0);
        else
            attract_credit_stamp_empty((u32)g0, 0, 0);
    }

    credits = i960_ld_u16(I960_ABS, 0x01d00020u, 0);
    slot = i960_ld_u32(I960_WORKRAM, 0x20a4e8, 0);
    /* @0xC180 cmpobge 9,g3 → tens only when credits > 9 */
    if (credits > 9u) {
        credit_draw_glyph(13, 45, slot, credits / 10u, 0);
    }
    credit_draw_glyph(15, 45, slot, credits % 10u, 0);

    /* @0xC1CC cmpobge 1,g4 → erase ratio when price <= 1; draw when price > 1. */
    price = i960_ld_u16(I960_WORKRAM, 0x20202a, 0);
    if (price <= 1u) {
        credit_erase_glyph(17, 45, slot, 10, 0);
        credit_erase_glyph(19, 45, slot, 0, 0);
        credit_erase_glyph(21, 45, slot, 0, 0);
        return;
    }

    /* "/" then price tens/ones */
    credit_draw_glyph(17, 45, slot, 10, 0);
    if (price > 9u) {
        credit_draw_glyph(19, 45, slot, price / 10u, 0);
        credit_draw_glyph(21, 45, slot, price % 10u, 0);
    } else {
        credit_draw_glyph(19, 45, slot, price, 0);
    }
    return;

free_play:
    slot = i960_ld_u32(I960_WORKRAM, 0x20a4ec, 0);
    credit_draw_glyph(1, 45, slot, 0, 0);

    gate = i960_ld_u32(I960_WORKRAM, 0x202008, 0) & 31u;
    if (gate != 0)
        return;

    if ((i960_ld_u32(I960_WORKRAM, 0x202008, 0) & (1u << 5)) != 0) {
        if (mode == 0u) {
            slot = i960_ld_u32(I960_WORKRAM, 0x20a4e0, 0);
            credit_draw_glyph(15, 30, slot, 0, 0);
        }
        {
            u8 b = i960_ld_u8(I960_WORKRAM, 0x20204c, 0);

            i960_st_u8(I960_WORKRAM, 0x20204c, 0, (u8)(b | 4u));
        }
        return;
    }

    if (mode == 0u) {
        slot = i960_ld_u32(I960_WORKRAM, 0x20a4e0, 0);
        credit_erase_glyph(15, 30, slot, 0, 0);
    }
    {
        u8 b = i960_ld_u8(I960_WORKRAM, 0x20204c, 0);

        i960_st_u8(I960_WORKRAM, 0x20204c, 0, (u8)(b & ~(1u << 2)));
    }
}
