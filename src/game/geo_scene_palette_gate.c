/* Scene layer-reg sync @ 0x58F0 (geo_scene_mode_dispatch submode 0).
 * ROM continues through ~0x5B64: set scroll bit15, init, then clear bit15.
 * Truncating before the clear left Sys24 layers disabled → black test menu. */
// @rom 0x58f0 +0x274 geo_scene_palette_gate

#include "i960_lift.h"
#include "i960_mem.h"

static u16 setbit15(u16 word)
{
    return (u16)(word | 0x8000u);
}

static u16 clrbit15(u16 word)
{
    return (u16)(word & 0x7fffu);
}

void geo_scene_palette_gate(u32 arg0, u32 arg1, u32 arg2)
{
    u16 hw;
    u16 wr_g5;
    u16 wr_g6;
    u16 wr_g7;
    u16 wr_g0;
    u8 flags;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x58F0 */
    i960_st_u8(I960_ABS, 0x181c000u, 0, (u8)g14);
    i960_st_u32(I960_WORKRAM, 0x213840, 0, 6u);

    /* @0x5904–0x596C: publish hw scroll with bit 15 set (layer disable). */
    hw = i960_ld_u16(I960_ABS, 0x0100a008u, 0);
    wr_g5 = i960_ld_u16(I960_WORKRAM, 0x20b91c, 0);
    i960_st_u16(I960_ABS, 0x0100a008u, 0, setbit15(hw));

    hw = i960_ld_u16(I960_ABS, 0x0100a00au, 0);
    wr_g6 = i960_ld_u16(I960_WORKRAM, 0x20b91e, 0);
    i960_st_u16(I960_ABS, 0x0100a00au, 0, setbit15(hw));

    hw = i960_ld_u16(I960_ABS, 0x0100a00cu, 0);
    wr_g7 = i960_ld_u16(I960_WORKRAM, 0x20b920, 0);
    i960_st_u16(I960_ABS, 0x0100a00cu, 0, setbit15(hw));

    hw = i960_ld_u16(I960_ABS, 0x0100a00eu, 0);
    wr_g0 = i960_ld_u16(I960_WORKRAM, 0x20b922, 0);
    i960_st_u16(I960_ABS, 0x0100a00eu, 0, setbit15(hw));

    i960_st_u32(I960_WORKRAM, 0x20a400, 0, (u32)g14);

    /* @0x597C–0x59A4: workram shadows with bit 15 set. */
    i960_st_u16(I960_WORKRAM, 0x20b91c, 0, setbit15(wr_g5));
    i960_st_u16(I960_WORKRAM, 0x20b91e, 0, setbit15(wr_g6));
    i960_st_u16(I960_WORKRAM, 0x20b920, 0, setbit15(wr_g7));
    i960_st_u16(I960_WORKRAM, 0x20b922, 0, setbit15(wr_g0));

    /* @0x59AC: bal 0x39678 — host: clear 0x202049 only. */
    i960_st_u8(I960_WORKRAM, 0x202049, 0, 0);

    /* @0x59B0–0x59F0: strip bits on 0x20204c. */
    flags = (u8)i960_ld_u8(I960_WORKRAM, 0x20204c, 0);
    flags = (u8)(flags & (u8)~0x04u);
    flags = (u8)(flags & (u8)~0x20u);
    flags = (u8)(flags & 0x7fu);
    i960_st_u8(I960_WORKRAM, 0x20204c, 0, flags);
    i960_st_u32(I960_WORKRAM, 0x20a2f4, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x20a564, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x20a560, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x20a280, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x20a530, 0, (u32)g14);

    /*
     * @0x5A10–0x5AA4: optional helpers omitted (tile/list prep).
     * @0x5AB0–0x5B5C: clear scroll disable bit 15 — without this, Sys24
     * draw_common returns early and the operator menu stays black.
     */
    wr_g5 = i960_ld_u16(I960_WORKRAM, 0x20b91c, 0);
    wr_g6 = i960_ld_u16(I960_WORKRAM, 0x20b91e, 0);
    wr_g7 = i960_ld_u16(I960_WORKRAM, 0x20b920, 0);
    wr_g0 = i960_ld_u16(I960_WORKRAM, 0x20b922, 0);

    i960_st_u8(I960_ABS, 0x181c000u, 0, 0xff);

    hw = i960_ld_u16(I960_ABS, 0x0100a008u, 0);
    i960_st_u16(I960_ABS, 0x0100a008u, 0, clrbit15(hw));
    i960_st_u16(I960_WORKRAM, 0x20b91c, 0, clrbit15(wr_g5));

    hw = i960_ld_u16(I960_ABS, 0x0100a00au, 0);
    i960_st_u16(I960_ABS, 0x0100a00au, 0, clrbit15(hw));
    i960_st_u16(I960_WORKRAM, 0x20b91e, 0, clrbit15(wr_g6));

    hw = i960_ld_u16(I960_ABS, 0x0100a00cu, 0);
    i960_st_u16(I960_ABS, 0x0100a00cu, 0, clrbit15(hw));
    i960_st_u16(I960_WORKRAM, 0x20b920, 0, clrbit15(wr_g7));

    hw = i960_ld_u16(I960_ABS, 0x0100a00eu, 0);
    i960_st_u16(I960_ABS, 0x0100a00eu, 0, clrbit15(hw));
    i960_st_u16(I960_WORKRAM, 0x20b922, 0, clrbit15(wr_g0));
}
