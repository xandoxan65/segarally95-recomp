/* Cam control / HUD slot seed @ 0x3A280 — clears control words, bal to
 * index tables @ 0x3AB98 / 0x3AE08, then picks 0x5d8f20 vs 0x5d90c0 from
 * 0x2139d0.
 *
 * Callers: geo_view_table_seed @ 0x377E0.
 * source: disasm/maincpu/maincpu_03a280_80.asm
 *         disasm/maincpu/maincpu_03ab98_b0.asm
 *         disasm/maincpu/maincpu_03ae08_28.asm */
// @rom 0x3a280 +0x80 geo_view_control_seed

#include "i960_lift.h"
#include "i960_mem.h"

/* Inlined bal @ 0x3AB98: palette/index words from g0 nibble. */
static void control_index_bal(u32 nibble)
{
    u32 last;

    i960_st_u32(I960_WORKRAM, 0x21431c, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x214304, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x214308, 0, 1u);

    if (nibble == 0u) {
        i960_st_u32(I960_WORKRAM, 0x21430c, 0, 0xafu);
        i960_st_u32(I960_WORKRAM, 0x214310, 0, 0xb0u);
        i960_st_u32(I960_WORKRAM, 0x214320, 0, 0xbbu);
        i960_st_u32(I960_WORKRAM, 0x214324, 0, 0xbcu);
        last = 0x69u;
    } else {
        i960_st_u32(I960_WORKRAM, 0x21430c, 0, 0xadu);
        i960_st_u32(I960_WORKRAM, 0x214310, 0, 0xaeu);
        i960_st_u32(I960_WORKRAM, 0x214320, 0, 0xb9u);
        i960_st_u32(I960_WORKRAM, 0x214324, 0, 0xbau);
        last = 0x7fu;
    }
    i960_st_u32(I960_WORKRAM, 0x214314, 0, last);
    /* addo 31,1 → 32 */
    i960_st_u32(I960_WORKRAM, 0x214318, 0, 32u);
}

/* Inlined bal @ 0x3AE08: clear three control words. */
static void control_clear_bal(void)
{
    i960_st_u32(I960_WORKRAM, 0x214328, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x214330, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x21432c, 0, 0);
}

void geo_view_control_seed(u32 arg0, u32 arg1, u32 arg2)
{
    u32 cam = arg0 != 0u ? arg0 : (u32)g0;
    u32 nibble;
    u32 flag;

    (void)arg1;
    (void)arg2;

    if (cam == 0u)
        return;

    i960_st_u32(I960_WORKRAM, 0x214300, 0, (u32)g14);
    /* addo 31,29 → 60 */
    i960_st_u32(I960_WORKRAM, 0x2142fc, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x2142e0, 0, 60u);

    nibble = (u32)i960_ld_u8(I960_ABS, cam, 0x10u) & 15u;
    control_index_bal(nibble);
    control_clear_bal();

    i960_st_u32(I960_WORKRAM, 0x2142f0, 0, (u32)g14);
    /* lda 0x44480000 = 800.0f */
    i960_st_u32(I960_WORKRAM, 0x2142e8, 0, 0x44480000u);
    i960_st_u32(I960_WORKRAM, 0x2142f4, 0, (u32)g14);

    flag = i960_ld_u32(I960_WORKRAM, 0x2139d0, 0);
    if (flag == 0u)
        i960_st_u32(I960_WORKRAM, 0x2142f8, 0, 0x5d8f20u);
    else
        i960_st_u32(I960_WORKRAM, 0x2142f8, 0, 0x5d90c0u);
}
