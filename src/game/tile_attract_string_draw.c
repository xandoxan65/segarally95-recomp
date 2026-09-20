/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_028990_450.asm */
// @rom 0x28990 +0x450 tile_attract_string_draw

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_rom.h"
#include <string.h>

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret  callee r10 */
/* abi: void * arg0=g0, u32 arg1=g1, u32 arg2=g2 → void */

/* pointers: g0=u32 *, g1=u32, g2=void *, g4=void *, g5=u32, r10=u32 *, r12=void *, r13=void *, r4=u32 *, r5=u32 *, r8=u32 * */

#include "lift_syms.h"

static u32 rom_u32_at(u32 vaddr)
{
    const u8 *p = model2_rom_at(vaddr);
    u32 v = 0;

    if (p)
        memcpy(&v, p, 4);
    return v;
}

void tile_attract_string_draw(void * arg0, u32 arg1, u32 arg2)
{
    u32 str_va = arg1;
    u32 * arg0_p = (u32 *)arg0;
    void * a2 = (void *)(uintptr_t)arg2;

    if (!arg0_p || !model2_rom_at(str_va))
        return;
    /* Scene-table sentinel start link is -1; skip rather than huge keyframe EA. */
    if ((i32)(u32)(uintptr_t)a2 < 0)
        return;

    g1 = str_va;

    g4 = 3 & (uintptr_t)a2;
    r13 = 0xff;
    g4 = r13 & g4;
    r10 = (uintptr_t)arg0_p;
    if (g4 == 0)
        goto L_00028ddc;
    g4 = 3 & (uintptr_t)a2;
    g2 = (u32)(uintptr_t)a2 >> 2;
    /* Disasm: lda 0x58,r12 — keyframe stride immediate, not a ROM pointer. */
    r12 = 0x58;
    g5 = (u32)r12 * (u32)g2;
    g2 = g2 + 0x1;
    g3 = (u32)g3 >> 2;
    g6 = i960_f64_to_u32((double)(i32)(u32)(g4));
    /* Disasm: lda 0x40800000 — float 4.0 for (link&3)/4.0, not a vaddr. */
    g4 = 0x40800000u;
    r8 = str_va + g5;
    r9 = i960_f64_to_u32((i960_u32_to_f64(g6)) / (i960_u32_to_f64(g4)));
    if (g2 <= g3)
        goto L_000289e4;
    r5 = r8;
    goto L_000289f0;

    L_000289e4:
        r13 = 0x58;
        g4 = (u32)r13 * (u32)g2;
        r5 = str_va + g4;

    L_000289f0:
        g4 = rom_u32_at(r5 + 0x50);
        *(u32 *)((uintptr_t)arg0_p + 0x50) = (u32)g4;
        g4 = rom_u32_at(r5 + 0x54);
        fp0 = i960_u32_to_f64(r9);
        *(u32 *)((uintptr_t)arg0_p + 0x54) = (u32)g4;
        r12 = rom_u32_at(r8);
        i960_rifl_write(&r6, &r7, (1.0) - (fp0));
        g4 = rom_u32_at(r5);
        fp1 = i960_u32_to_f64(r12);
        fp1 = i960_u32_to_f64(r12);
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(r9)));
        i960_rifl_write(&r12, &r13, (fp1) * (i960_rifl_read(r6, r7)));
        fp0 = i960_u32_to_f64(g4);
        fp0 = i960_u32_to_f64(g4);
        fp1 = i960_rifl_read(r12, r13);
        i960_rifl_write(&r12, &r13, (fp0) + (fp1));
        fp1 = i960_rifl_read(r12, r13);
        g4 = i960_f64_to_u32(fp1);
        *(u32 *)arg0_p = (u32)g4;
        r13 = rom_u32_at(r8 + 0x4);
        g4 = rom_u32_at(r5 + 0x4);
        fp1 = i960_u32_to_f64(r13);
        fp1 = i960_u32_to_f64(r13);
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(r9)));
        i960_rifl_write(&r12, &r13, (fp1) * (i960_rifl_read(r6, r7)));
        fp0 = i960_u32_to_f64(g4);
        fp0 = i960_u32_to_f64(g4);
        fp1 = i960_rifl_read(r12, r13);
        i960_rifl_write(&r12, &r13, (fp0) + (fp1));
        fp1 = i960_rifl_read(r12, r13);
        g4 = i960_f64_to_u32(fp1);
        *(u32 *)((uintptr_t)arg0_p + 0x4) = (u32)g4;
        r13 = rom_u32_at(r8 + 0x8);
        g4 = rom_u32_at(r5 + 0x8);
        fp1 = i960_u32_to_f64(r13);
        fp1 = i960_u32_to_f64(r13);
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(r9)));
        i960_rifl_write(&r12, &r13, (fp1) * (i960_rifl_read(r6, r7)));
        fp0 = i960_u32_to_f64(g4);
        fp0 = i960_u32_to_f64(g4);
        fp1 = i960_rifl_read(r12, r13);
        i960_rifl_write(&r12, &r13, (fp0) + (fp1));
        fp1 = i960_rifl_read(r12, r13);
        g4 = i960_f64_to_u32(fp1);
        *(u32 *)((uintptr_t)arg0_p + 0x8) = (u32)g4;
        r13 = rom_u32_at(r8 + 0xc);
        g4 = rom_u32_at(r5 + 0xc);
        fp1 = i960_u32_to_f64(r13);
        fp1 = i960_u32_to_f64(r13);
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(r9)));
        i960_rifl_write(&r12, &r13, (fp1) * (i960_rifl_read(r6, r7)));
        fp0 = i960_u32_to_f64(g4);
        fp0 = i960_u32_to_f64(g4);
        fp1 = i960_rifl_read(r12, r13);
        i960_rifl_write(&r12, &r13, (fp0) + (fp1));
        fp1 = i960_rifl_read(r12, r13);
        g4 = i960_f64_to_u32(fp1);
        *(u32 *)((uintptr_t)arg0_p + 0xc) = (u32)g4;
        r13 = rom_u32_at(r8 + 0x10);
        g4 = rom_u32_at(r5 + 0x10);
        fp1 = i960_u32_to_f64(r13);
        fp1 = i960_u32_to_f64(r13);
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(r9)));
        i960_rifl_write(&r12, &r13, (fp1) * (i960_rifl_read(r6, r7)));
        fp0 = i960_u32_to_f64(g4);
        fp0 = i960_u32_to_f64(g4);
        fp1 = i960_rifl_read(r12, r13);
        i960_rifl_write(&r12, &r13, (fp0) + (fp1));
        fp1 = i960_rifl_read(r12, r13);
        g4 = i960_f64_to_u32(fp1);
        *(u32 *)((uintptr_t)arg0_p + 0x10) = (u32)g4;
        r13 = rom_u32_at(r8 + 0x14);
        g4 = rom_u32_at(r5 + 0x14);
        fp1 = i960_u32_to_f64(r13);
        fp1 = i960_u32_to_f64(r13);
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(r9)));
        i960_rifl_write(&r12, &r13, (fp1) * (i960_rifl_read(r6, r7)));
        fp0 = i960_u32_to_f64(g4);
        fp0 = i960_u32_to_f64(g4);
        fp1 = i960_rifl_read(r12, r13);
        i960_rifl_write(&r12, &r13, (fp0) + (fp1));
        fp1 = i960_rifl_read(r12, r13);
        g4 = i960_f64_to_u32(fp1);
        r4 = r8 + 0x18;
        *(u32 *)((uintptr_t)arg0_p + 0x14) = (u32)g4;
        g0 = rom_u32_at(r4);
        g1 = rom_u32_at(r5 + 0x18);
        tile_attract_angle_fixup((u32)g0, (u32)g1);
    g4 = i960_f64_to_u32((i960_u32_to_f64(g0)) * (i960_u32_to_f64(r9)));
    g5 = rom_u32_at(r4);
    g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) + (i960_u32_to_f64(g4)));
    *(u32 *)((uintptr_t)arg0_p + 0x18) = (u32)g4;
    r4 = r8 + 28;
    g0 = rom_u32_at(r4);
    g1 = rom_u32_at(r5 + 0x1c);
    tile_attract_angle_fixup((u32)g0, (u32)g1);
    g4 = i960_f64_to_u32((i960_u32_to_f64(g0)) * (i960_u32_to_f64(r9)));
    g5 = rom_u32_at(r4);
    g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) + (i960_u32_to_f64(g4)));
    *(u32 *)((uintptr_t)arg0_p + 0x1c) = (u32)g4;
    r4 = r8 + 0x20;
    g0 = rom_u32_at(r4);
    g1 = rom_u32_at(r5 + 0x20);
    tile_attract_angle_fixup((u32)g0, (u32)g1);
    g4 = i960_f64_to_u32((i960_u32_to_f64(g0)) * (i960_u32_to_f64(r9)));
    g5 = rom_u32_at(r4);
    g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) + (i960_u32_to_f64(g4)));
    *(u32 *)((uintptr_t)arg0_p + 0x20) = (u32)g4;
    r13 = rom_u32_at(r8 + 0x24);
    g4 = rom_u32_at(r5 + 0x24);
    fp1 = i960_u32_to_f64(r13);
    fp1 = i960_u32_to_f64(r13);
    g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(r9)));
    i960_rifl_write(&r12, &r13, (fp1) * (i960_rifl_read(r6, r7)));
    fp0 = i960_u32_to_f64(g4);
    fp0 = i960_u32_to_f64(g4);
    fp1 = i960_rifl_read(r12, r13);
    i960_rifl_write(&r12, &r13, (fp0) + (fp1));
    fp1 = i960_rifl_read(r12, r13);
    g4 = i960_f64_to_u32(fp1);
    *(u32 *)((uintptr_t)arg0_p + 0x24) = (u32)g4;
    r13 = rom_u32_at(r8 + 0x30);
    g4 = rom_u32_at(r5 + 0x30);
    fp1 = i960_u32_to_f64(r13);
    fp1 = i960_u32_to_f64(r13);
    g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(r9)));
    i960_rifl_write(&r12, &r13, (fp1) * (i960_rifl_read(r6, r7)));
    fp0 = i960_u32_to_f64(g4);
    fp0 = i960_u32_to_f64(g4);
    fp1 = i960_rifl_read(r12, r13);
    i960_rifl_write(&r12, &r13, (fp0) + (fp1));
    fp1 = i960_rifl_read(r12, r13);
    g4 = i960_f64_to_u32(fp1);
    *(u32 *)((uintptr_t)arg0_p + 0x30) = (u32)g4;
    r13 = rom_u32_at(r8 + 0x34);
    g4 = rom_u32_at(r5 + 0x34);
    fp1 = i960_u32_to_f64(r13);
    fp1 = i960_u32_to_f64(r13);
    g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(r9)));
    i960_rifl_write(&r12, &r13, (fp1) * (i960_rifl_read(r6, r7)));
    fp0 = i960_u32_to_f64(g4);
    fp0 = i960_u32_to_f64(g4);
    fp1 = i960_rifl_read(r12, r13);
    i960_rifl_write(&r12, &r13, (fp0) + (fp1));
    fp1 = i960_rifl_read(r12, r13);
    g4 = i960_f64_to_u32(fp1);
    *(u32 *)((uintptr_t)arg0_p + 0x34) = (u32)g4;
    r13 = rom_u32_at(r8 + 0x38);
    g4 = rom_u32_at(r5 + 0x38);
    fp1 = i960_u32_to_f64(r13);
    fp1 = i960_u32_to_f64(r13);
    g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(r9)));
    i960_rifl_write(&r12, &r13, (fp1) * (i960_rifl_read(r6, r7)));
    fp0 = i960_u32_to_f64(g4);
    fp0 = i960_u32_to_f64(g4);
    fp1 = i960_rifl_read(r12, r13);
    i960_rifl_write(&r12, &r13, (fp0) + (fp1));
    fp1 = i960_rifl_read(r12, r13);
    g4 = i960_f64_to_u32(fp1);
    *(u32 *)((uintptr_t)arg0_p + 0x38) = (u32)g4;
    r13 = rom_u32_at(r8 + 0x3c);
    g4 = rom_u32_at(r5 + 0x3c);
    fp1 = i960_u32_to_f64(r13);
    fp1 = i960_u32_to_f64(r13);
    g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(r9)));
    i960_rifl_write(&r12, &r13, (fp1) * (i960_rifl_read(r6, r7)));
    fp0 = i960_u32_to_f64(g4);
    fp0 = i960_u32_to_f64(g4);
    fp1 = i960_rifl_read(r12, r13);
    i960_rifl_write(&r12, &r13, (fp0) + (fp1));
    fp1 = i960_rifl_read(r12, r13);
    g4 = i960_f64_to_u32(fp1);
    *(u32 *)((uintptr_t)arg0_p + 0x3c) = (u32)g4;
    r13 = rom_u32_at(r8 + 0x40);
    g4 = rom_u32_at(r5 + 0x40);
    fp1 = i960_u32_to_f64(r13);
    fp1 = i960_u32_to_f64(r13);
    g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(r9)));
    i960_rifl_write(&r12, &r13, (fp1) * (i960_rifl_read(r6, r7)));
    fp0 = i960_u32_to_f64(g4);
    fp0 = i960_u32_to_f64(g4);
    fp1 = i960_rifl_read(r12, r13);
    i960_rifl_write(&r12, &r13, (fp0) + (fp1));
    fp1 = i960_rifl_read(r12, r13);
    g4 = i960_f64_to_u32(fp1);
    *(u32 *)((uintptr_t)arg0_p + 0x40) = (u32)g4;
    r13 = rom_u32_at(r8 + 0x44);
    g4 = rom_u32_at(r5 + 0x44);
    fp1 = i960_u32_to_f64(r13);
    fp1 = i960_u32_to_f64(r13);
    g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(r9)));
    i960_rifl_write(&r12, &r13, (fp1) * (i960_rifl_read(r6, r7)));
    fp0 = i960_u32_to_f64(g4);
    fp0 = i960_u32_to_f64(g4);
    fp1 = i960_rifl_read(r12, r13);
    i960_rifl_write(&r12, &r13, (fp0) + (fp1));
    fp1 = i960_rifl_read(r12, r13);
    g4 = i960_f64_to_u32(fp1);
    *(u32 *)((uintptr_t)arg0_p + 0x44) = (u32)g4;
    r13 = rom_u32_at(r8 + 0x48);
    g4 = rom_u32_at(r5 + 0x48);
    fp1 = i960_u32_to_f64(r13);
    fp1 = i960_u32_to_f64(r13);
    g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(r9)));
    i960_rifl_write(&r12, &r13, (fp1) * (i960_rifl_read(r6, r7)));
    fp0 = i960_u32_to_f64(g4);
    fp0 = i960_u32_to_f64(g4);
    fp1 = i960_rifl_read(r12, r13);
    i960_rifl_write(&r12, &r13, (fp0) + (fp1));
    fp1 = i960_rifl_read(r12, r13);
    g4 = i960_f64_to_u32(fp1);
    *(u32 *)((uintptr_t)arg0_p + 0x48) = (u32)g4;
    r13 = rom_u32_at(r8 + 0x4c);
    g4 = rom_u32_at(r5 + 0x4c);
    fp1 = i960_u32_to_f64(r13);
    fp1 = i960_u32_to_f64(r13);
    g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(r9)));
    i960_rifl_write(&r12, &r13, (fp1) * (i960_rifl_read(r6, r7)));
    fp0 = i960_u32_to_f64(g4);
    fp0 = i960_u32_to_f64(g4);
    fp1 = i960_rifl_read(r12, r13);
    i960_rifl_write(&r12, &r13, (fp0) + (fp1));
    fp1 = i960_rifl_read(r12, r13);
    g4 = i960_f64_to_u32(fp1);
    *(u32 *)((uintptr_t)arg0_p + 0x4c) = (u32)g4;
    r13 = rom_u32_at(r8 + 0x28);
    g4 = rom_u32_at(r5 + 0x28);
    fp1 = i960_u32_to_f64(r13);
    fp1 = i960_u32_to_f64(r13);
    g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(r9)));
    i960_rifl_write(&r12, &r13, (fp1) * (i960_rifl_read(r6, r7)));
    fp0 = i960_u32_to_f64(g4);
    fp0 = i960_u32_to_f64(g4);
    fp1 = i960_rifl_read(r12, r13);
    i960_rifl_write(&r12, &r13, (fp0) + (fp1));
    fp1 = i960_rifl_read(r12, r13);
    g4 = i960_f64_to_u32(fp1);
    *(u32 *)((uintptr_t)arg0_p + 0x28) = (u32)g4;
    r8 = rom_u32_at(r8 + 0x2c);
    g4 = rom_u32_at(r5 + 0x2c);
    fp1 = i960_u32_to_f64(r8);
    r9 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(r9)));
    fp1 = i960_u32_to_f64(r8);
    i960_rifl_write(&r6, &r7, (fp1) * (i960_rifl_read(r6, r7)));
    fp0 = i960_u32_to_f64(r9);
    fp0 = i960_u32_to_f64(r9);
    i960_rifl_write(&r6, &r7, (fp0) + (i960_rifl_read(r6, r7)));
    fp2 = i960_rifl_read(r6, r7);
    g4 = i960_f64_to_u32(fp2);
    *(u32 *)((uintptr_t)arg0_p + 0x2c) = (u32)g4;
    return;

    L_00028ddc:
        /*
         * link & 3 == 0: exact keyframe copy (no lerp). Disasm @ 0x28DDC–0x28E1C:
         *   src = str_va + (link >> 2) * 0x58
         *   copy 0x58 bytes → object (5×ldq/stq + ldl/stl @ +0x50)
         */
        {
            u32 src_va = str_va + (((u32)(uintptr_t)a2 >> 2) * 0x58u);
            u32 i;

            if (!model2_rom_at(src_va) || !model2_rom_at(src_va + 0x54u))
                return;
            for (i = 0; i < 0x58u; i += 4u)
                *(u32 *)((uintptr_t)arg0_p + i) = rom_u32_at(src_va + i);
        }
}
