/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_0005d7e8_1dc.asm */
// @rom 0x5d7e8 +0x1dc cgm_template_emit

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"
#include "model2_rom.h"

#define CGM_TEMPLATE_G6     10u
#define CGM_R9_MASK_WIDTH   0x30u /* @0x05D890: and r9, 31+17 */
#define CGM_R9_BIT6         0x40u
#define CGM_R9_BIT4         0x10u

static u8 template_byte(u32 vaddr)
{
    if (vaddr >= 0x005ca000u && vaddr < 0x005cc000u)
        return (u8)i960_ld_u8(I960_WORKRAM, vaddr, 0);
    /* @0x05D828: 0x005FBF10 mirrors maincpu ROM @ 0x05CF10 (not zero workram). */
    if (vaddr >= 0x0059f000u)
        return model2_workram_mirror_u8(vaddr);
    {
        const u8 *p = model2_rom_at(vaddr);
        if (p)
            return *p;
    }
    return 0;
}

/* @0x027008 via 0x05D8B0: every emit is bal 0x27008 (no synthetic shortcuts). */
static void cgm_emit_27008(u32 g0)
{
    boot_tile_opcode_dispatch(g0);
}

static void cgm_emit_repeat_loop(u32 count)
{
    u32 n;

    for (n = 0; n < count; n++)
        cgm_emit_27008(32u);
}

static void cgm_emit_width_loop(u32 count)
{
    u32 n;

    for (n = 0; n < count; n++)
        cgm_emit_27008(48u);
}

static void cgm_emit_template_bytes(const u8 *row, u32 len)
{
    u32 idx;

    /* @0x05D94C: each template byte → bal 0x027008 (ASCII >31 included). */
    for (idx = 0; idx < len; idx++)
        cgm_emit_27008((u32)row[idx]);
}

/* @0x05D864–0x05D9C0 — semantic correction of uplift @ L_0005d8ac..L_0005d9c0. */
static void cgm_d860_emit(
    u32 r9,
    u32 r13,
    u32 r8,
    u32 r3,
    u32 r14,
    const u8 *template_row,
    u32 row_len)
{
    u32 r7;
    u32 r10;
    u32 g11 = CGM_R9_MASK_WIDTH;

    /* @0x05D86C: r7 = r8 + r15 (r15=1); optional +1/+2 from r13/r9 bit6. */
    r7 = r8 + 1u;
    if ((r13 & 0xffu) != 0)
        r7++;
    if ((r9 & CGM_R9_BIT6) != 0)
        r7 += 2u;
    r10 = (r3 >= r7) ? r3 : r7;

    if ((r9 & g11) != 0 && r14 > 0u && r10 < r14)
        cgm_emit_repeat_loop(r14 - r10);

    if ((r13 & 0xffu) != 0)
        cgm_emit_repeat_loop(r13 & 0xffu);

    if ((r9 & CGM_R9_BIT6) != 0) {
        cgm_emit_27008(48u);
        if (row_len > 0)
            cgm_emit_27008((u32)template_row[0]);
    }

    if ((r9 & g11) == 1u && r14 > 0u && r10 < r14)
        cgm_emit_width_loop(r14 - r10);

    if (r3 < r7)
        cgm_emit_width_loop(r7 - r3);

    if (r8 > 0u && template_row != 0)
        cgm_emit_template_bytes(template_row, row_len < r8 ? row_len : r8);

    if ((r9 & CGM_R9_BIT4) != 0 && r14 > 0u && r10 < r14)
        cgm_emit_repeat_loop(r14 - r10);
}

void cgm_template_emit(u32 tpl_vaddr, u32 g6, u32 r9_flags, u32 r14_repeat)
{
    u8 row[CGM_TEMPLATE_G6];
    u32 repeat;
    u32 row_idx;
    u32 idx;
    u32 r3;

    if (g6 == 0)
        g6 = CGM_TEMPLATE_G6;

    /* @0x05D7EC–0x05D850: copy template row; r14 is digit repeat from @0x05CFA8. */
    repeat = r14_repeat;
    if (repeat == 0)
        repeat = 1;
    r3 = repeat;
    /* @0x05D808 remo g6,g5,g4 — row index = repeat % g6. */
    row_idx = (repeat % g6) % 10u;

    for (idx = 0; idx < g6; idx++)
        row[idx] = template_byte(tpl_vaddr + row_idx * g6 + idx);

    cgm_d860_emit(r9_flags, 0u, g6, r3, repeat, row, g6);
}
