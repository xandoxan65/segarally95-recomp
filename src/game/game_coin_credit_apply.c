/* Credit / price apply @ 0xAC50 — spend 0x01D00020 when >= price @ 0x20202A.
 * Free-play bit3 @ 0x202024 → ret. Dual-chute bit0 → AD38 path.
 * source: disasm/maincpu/maincpu_00ac50_1c8.asm */
// @rom 0xac50 +0x1c0 game_coin_credit_apply

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_rom.h"

static void credit_apply_dual(void)
{
    u32 i;
    u32 credits_ea;
    u32 chute_ea;
    u16 price;
    u16 credits;
    u16 chute;
    u32 limit;
    u32 ix;

    credits_ea = 0x01d00020u;
    chute_ea = 0x01d00022u;

    for (i = 0; i < 2u; i++) {
        ix = ((u32)i960_ld_u8(I960_WORKRAM, 0x202019, 0) << 2) & 12u;
        limit = model2_workram_mirror_u32(0x005a9410u + ix);
        chute = (u16)i960_ld_u16(I960_ABS, chute_ea, 0);
        if (chute >= limit)
            i960_st_u32(I960_WORKRAM, 0x20a518, 0, 0);

        price = (u16)i960_ld_u16(I960_WORKRAM, 0x20202a, 0);
        if (price == 0)
            goto next;

        for (;;) {
            credits = (u16)i960_ld_u16(I960_ABS, credits_ea, 0);
            if (credits < price)
                break;
            i960_st_u16(I960_ABS, credits_ea, 0, (u16)(credits - price));
            chute = (u16)i960_ld_u16(I960_ABS, chute_ea, 0);
            ix = ((u32)i960_ld_u8(I960_WORKRAM, 0x202019, 0) << 2) & 12u;
            limit = model2_workram_mirror_u32(0x005a9410u + ix);
            if (chute < limit) {
                u32 meter = i960_ld_u32(I960_ABS, 0x01d00004u, 0);

                i960_st_u16(I960_ABS, chute_ea, 0, (u16)(chute + 1u));
                if (meter <= 0xfffffffeu)
                    meter += 1u;
                i960_st_u32(I960_ABS, 0x01d00004u, 0, meter);
                i960_st_u32(I960_WORKRAM, 0x20a518, 0, 2);
            }
            price = (u16)i960_ld_u16(I960_WORKRAM, 0x20202a, 0);
            if (price == 0)
                break;
            if ((u16)i960_ld_u16(I960_ABS, credits_ea, 0) < price)
                break;
        }
    next:
        credits_ea += 16u;
        chute_ea += 16u;
    }
}

void game_coin_credit_apply(u32 arg0, u32 arg1, u32 arg2)
{
    u8 flags;
    u16 price;
    u16 credits;
    u16 chute;
    u32 limit;
    u32 ix;
    u32 table;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    flags = (u8)i960_ld_u8(I960_WORKRAM, 0x202024, 0);
    if ((flags & 8u) != 0)
        return;
    if ((flags & 1u) != 0) {
        credit_apply_dual();
        return;
    }

    ix = ((u32)i960_ld_u8(I960_WORKRAM, 0x202019, 0) << 2) & 12u;
    limit = model2_workram_mirror_u32(0x005a9410u + ix);
    chute = (u16)i960_ld_u16(I960_ABS, 0x01d00022u, 0);
    if (chute >= limit)
        i960_st_u32(I960_WORKRAM, 0x20a518, 0, 0);

    price = (u16)i960_ld_u16(I960_WORKRAM, 0x20202a, 0);
    if (price == 0)
        return;
    credits = (u16)i960_ld_u16(I960_ABS, 0x01d00020u, 0);
    if (credits < price)
        return;

    table = 0x005a9410u + ix;
    for (;;) {
        credits = (u16)i960_ld_u16(I960_ABS, 0x01d00020u, 0);
        chute = (u16)i960_ld_u16(I960_ABS, 0x01d00022u, 0);
        limit = model2_workram_mirror_u32(table);
        i960_st_u16(I960_ABS, 0x01d00020u, 0, (u16)(credits - price));
        if (chute < limit) {
            u32 meter = i960_ld_u32(I960_ABS, 0x01d00004u, 0);

            i960_st_u16(I960_ABS, 0x01d00022u, 0, (u16)(chute + 1u));
            if (meter <= 0xfffffffeu)
                meter += 1u;
            i960_st_u32(I960_ABS, 0x01d00004u, 0, meter);
            i960_st_u32(I960_WORKRAM, 0x20a518, 0, 2);
        }
        price = (u16)i960_ld_u16(I960_WORKRAM, 0x20202a, 0);
        if (price == 0)
            return;
        if ((u16)i960_ld_u16(I960_ABS, 0x01d00020u, 0) < price)
            return;
    }
}
