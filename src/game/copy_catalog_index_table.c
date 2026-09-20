/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_0282d0_400.asm */
// @rom 0x282d0 +0x400 copy_catalog_index_table

#include "i960_lift.h"
#include "i960_fp.h"
#include "model2_rom.h"
#include "i960_mem.h"

#include <math.h>

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret  callee r10,r11,r9 */
/* abi: void * arg0=g0, u32 arg1=g1, u32 arg2=g2 → void */

/* pointers: fp=u32 *, g0=u32 *, g10=u32, g11=u32, g12=u32, g13=u32, g4=u32, g5=void *, g6=u32, g7=void *, g8=u32, g9=u32, r10=u32 *, r11=u32 *, r12=u32 *, r14=u32, r15=u32, r3=void *, r4=u32 *, r5=void *, r8=u32, r9=u32 *, sp=u64 * */

void copy_catalog_index_table(void * arg0, u32 arg1, u32 arg2)
{
    u32 * arg0_p = (u32 *)arg0;
    g1 = (uintptr_t)arg1;
    g2 = (uintptr_t)arg2;

    sp = sp + 0xa0;
    *(u64 *)(sp - 0x20) = (u64)g8;
    *(u32 *)(sp - 0x10) = (u32)g12;
    g5 = (uintptr_t)(i32)(i960_u32_to_f64(arg1));
    r12 = (uintptr_t)arg0_p + 0x4;
    g0 = *(u32 *)arg0_p;
    r8 = *(u32 *)r12;
    r12 = r12 + 4;
    g4 = *(u32 *)r12;
    r12 = r12 + 4;
    *(u32 *)(fp + 0x80) = (u32)g0;
    g0 = g4;
    g6 = (uintptr_t)(i32)(i960_u32_to_f64(arg1));
    if (0 > (signed char)g6) {
        g6 = 0 - g6;
    }
    g5 = g6 - 1;
    r10 = g0;
    r9 = g0;
    r11 = g0;
    if (g5 < 0)
        goto L_00028338;
    g4 = r8 + (u32)(r8 << 3);
    g4 = (u32)g4 * (u32)g5;
    goto L_00028350;

    L_00028338:
        if ((unsigned char)g3 == 0) {
            g9 = *(u32 *)(fp + 0x80);
            g5 = r8 + (u32)(r8 << 3);
            g4 = g9 - 0x1 + (u32)g6;
            g4 = (u32)g4 * (u32)g5;
        }

    L_00028350:
        g0 = g0 + (u32)(g4 << 2);
        g7 = r8 + (u32)(r8 << 3);
        g5 = (u32)g7 * (u32)g6;
        g10 = *(u32 *)(fp + 0x80);
        g4 = g6 + 1;
        r11 = r11 + (u32)(g5 << 2);
        if (g4 < g10)
            goto L_00028394;
        if ((unsigned char)g3 != 0)
            goto L_00028384;
        g4 = g10 - 1;
        g4 = g6 - g4;
        g4 = (u32)g4 * (u32)g7;
        goto L_000283a0;

    L_00028384:
        g11 = *(u32 *)(fp + 0x80);
        g5 = r8 + (u32)(r8 << 3);
        g4 = g11 - 1;
        goto L_0002839c;

    L_00028394:
        g4 = g6 + 1;
        g5 = r8 + (u32)(r8 << 3);

    L_0002839c:
        g4 = (u32)g5 * (u32)g4;

    L_000283a0:
        r9 = r9 + (u32)(g4 << 2);
        g12 = *(u32 *)(fp + 0x80);
        g4 = g6 + 2;
        if ((signed char)g4 < (signed char)g12)
            goto L_000283d0;
        if ((unsigned char)g3 != 0)
            goto L_000283c0;
        g4 = g12 - 2;
        g4 = g6 - g4;
        goto L_000283d4;

    L_000283c0:
        g13 = *(u32 *)(fp + 0x80);
        g5 = r8 + (u32)(r8 << 3);
        g4 = g13 - 1;
        goto L_000283d8;

    L_000283d0:
        g4 = g6 + 2;

    L_000283d4:
        g5 = r8 + (u32)(r8 << 3);

    L_000283d8:
        g4 = (u32)g5 * (u32)g4;
        r10 = r10 + (u32)(g4 << 2);
        fp1 = i960_u32_to_f64(arg1);
        /* Disasm @ 0x283E4: cpysre fp1,+0.0,fp1 → |fp1| (MAME: sign of +0 onto |src1|). */
        fp1 = fabs(fp1);
        g4 = (uintptr_t)(i32)(fp1);
        fp0 = (double)(i32)(u32)(g4);
        r14 = i960_f64_to_u32(fp0);
        fp0 = i960_u32_to_f64(r14);
        i960_rifl_write(&g8, &g9, (fp1) - (fp0));
        fp1 = i960_rifl_read(g8, g9);
        g4 = i960_f64_to_u32(fp1);
        fp0 = i960_u32_to_f64(g4);
        i960_rifl_write(&g10, &g11, (1.0) + (fp0));
        *(u64 *)(fp + 0xa0) = (u64)g10;
        g11 = g11 ^ (1u << 31);
        g4 = g4 ^ (1u << 31);
        *(u64 *)(fp + 0x70) = (u64)g10;
        g10 = (u32)*((u64 *)(fp + 0x70));
        i960_rifl_write(&g6, &g7, (fp0) - (1.0));
        fp3 = i960_rifl_read(g10, g11);
        fp1 = i960_u32_to_f64(g4);
        i960_rifl_write(&g10, &g11, (fp0) * (fp3));
        r14 = (u32)*((u64 *)(fp + 0xa0));
        i960_rifl_write(&g12, &g13, (i960_rifl_read(g6, g7)) * (fp1));
        fp2 = i960_rifl_read(r14, r15);
        r14 = 0;
        r15 = r15 & ~(1u << 30);
        i960_rifl_write(&g4, &g5, (i960_rifl_read(g6, g7)) * (fp2));
        i960_rifl_write(&g8, &g9, (fp0) * (fp2));
        fp2 = i960_rifl_read(r14, r15);
        i960_rifl_write(&r14, &r15, (fp0) - (fp2));
        *(u64 *)(fp + 0x70) = (u64)g10;
        fp0 = i960_rifl_read(r14, r15);
        r14 = (u32)*((u64 *)(fp + 0x70));
        fp1 = i960_rifl_read(g12, g13);
        *(u64 *)(fp + 0xa0) = (u64)g8;
        i960_rifl_write(&g8, &g9, (fp0) * (fp1));
        g10 = (u32)*((u64 *)(fp + 0xa0));
        fp2 = i960_rifl_read(r14, r15);
        i960_rifl_write(&r14, &r15, (fp0) * (fp2));
        fp3 = i960_rifl_read(g10, g11);
        i960_rifl_write(&g6, &g7, (i960_rifl_read(g6, g7)) * (fp3));
        fp1 = i960_rifl_read(g8, g9);
        *(u64 *)(fp + 0x70) = (u64)r14;
        g8 = 0;
        g9 = (uintptr_t)i960_vaddr_ptr(0x40180000);
        i960_rifl_write(&g4, &g5, (fp0) * (i960_rifl_read(g4, g5)));
        fp0 = i960_rifl_read(g8, g9);
        g8 = (u32)*((u64 *)(fp + 0x70));
        g12 = 0;
        g13 = g13 & ~(1u << 30);
        fp2 = i960_rifl_read(g6, g7);
        fp3 = i960_rifl_read(g12, g13);
        i960_rifl_write(&r14, &r15, (fp2) / (fp0));
        fp2 = i960_rifl_read(g12, g13);
        i960_rifl_write(&g4, &g5, (i960_rifl_read(g4, g5)) / (fp3));
        fp3 = i960_rifl_read(g8, g9);
        i960_rifl_write(&g8, &g9, (fp3) / (fp2));
        i960_rifl_write(&g10, &g11, (fp1) / (fp0));
        *(u64 *)(fp + 0xa0) = (u64)g6;
        *(u64 *)(fp + 0xa0) = (u64)r14;
        r14 = (u32)*((u64 *)(fp + 0xa0));
        *(u64 *)(fp + 0x70) = (u64)g8;
        g8 = (u32)*((u64 *)(fp + 0x70));
        fp1 = i960_rifl_read(g10, g11);
        r3 = i960_f64_to_u32(fp1);
        r13 = 0;
        fp3 = i960_rifl_read(g4, g5);
        fp2 = i960_rifl_read(r14, r15);
        g4 = i960_f64_to_u32(fp3);
        fp3 = i960_rifl_read(g8, g9);
        fp0 = i960_u32_to_f64(r3);
        r15 = i960_f64_to_u32(fp2);
        g9 = i960_f64_to_u32(fp3);
        r3 = (uintptr_t)i960_vaddr_ptr(0x10002020);
        g12 = (uintptr_t)i960_vaddr_ptr(0x2800505);
        fp1 = i960_u32_to_f64(g4);
        *(u32 *)(fp + 0xb0) = (u32)r15;
        *(u32 *)(fp + 0x90) = (u32)g9;
        if (r8 <= 0)
            goto L_000286bc;

    L_00028528:
        r4 = fp + 0x40;
        r5 = 0;
        do {
            g4 = *(u32 *)g0;
            g6 = *(u32 *)r11;
            g5 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (fp0));
            g4 = *(u32 *)r9;
            r14 = *(u32 *)(fp + 0x90);
            g7 = i960_f64_to_u32((i960_u32_to_f64(g6)) * (fp1));
            g6 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(r14)));
            g4 = *(u32 *)r10;
            r15 = *(u32 *)(fp + 0xb0);
            g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(r15)));
            g5 = i960_f64_to_u32((i960_u32_to_f64(g7)) + (i960_u32_to_f64(g5)));
            g5 = i960_f64_to_u32((i960_u32_to_f64(g6)) + (i960_u32_to_f64(g5)));
            g5 = i960_f64_to_u32((i960_u32_to_f64(g4)) + (i960_u32_to_f64(g5)));
            r10 = r10 + 0x4;
            r9 = r9 + 4;
            r11 = r11 + 0x4;
            g0 = g0 + 4;
            r5 = r5 + 0x1;
            *(u32 *)r4 = (u32)g5;
            r4 = r4 + 4;
        } while (r5 <= 8);
    /* @0x28588 ldq 0x40(fp),r4 — pose pack: T.xyz + euler Z/Y/X. */
    r4 = *(u32 *)(fp + 0x40);
    r5 = *(u32 *)(fp + 0x44);
    r6 = *(u32 *)(fp + 0x48);
    r7 = *(u32 *)(fp + 0x4c);
    i960_mmio_write_u32(0x884000, (u32)r3); /* 0x10002020 begin */
    g8 = (uintptr_t)i960_vaddr_ptr(0x13802727);
    i960_mmio_write_u32(0x884000, (u32)g8);
    g4 = *(u32 *)(fp + 0x4c); /* Z angle bits (same as r7) */
    i960_mmio_write_u32(0x884000, (u32)r4);
    i960_mmio_write_u32(0x884000, (u32)r5);
    i960_mmio_write_u32(0x884000, (u32)r6);
    g9 = (uintptr_t)i960_vaddr_ptr(0x15802b2b);
    i960_mmio_write_u32(0x884000, (u32)g9);
    g5 = *(u32 *)(fp + 0x50); /* Y angle */
    i960_mmio_write_u32(0x884000, (u32)g4);
    g10 = (uintptr_t)i960_vaddr_ptr(0x15002a2a);
    i960_mmio_write_u32(0x884000, (u32)g10);
    g7 = *(u32 *)(fp + 0x54); /* X angle */
    i960_mmio_write_u32(0x884000, (u32)g5);
    g11 = (uintptr_t)i960_vaddr_ptr(0x14802929);
    i960_mmio_write_u32(0x884000, (u32)g11);
    i960_mmio_write_u32(0x884000, (u32)g7);
    g6 = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
    g4 = i960_ld_u32(I960_MMIO, 0x802008, 0); /* geo_write_start_read */;
    *(u32 *)g6 = (u32)g4;
    i960_mmio_write_u32(0x884000, (u32)g12); /* 0x02800505 */
    g4 = g4 + 0x34;
    i960_mmio_write_u32(0x801008, (u32)g4); /* geo_write_start */;
    g4 = i960_ld_u32(I960_WORKRAM, 0x202278, 0);
    g5 = *(u32 *)r12;
    g4 = 3 & g4;
    g4 = g4 << 10;
    /* MAME geo_w @ 0x800010+(bank<<10): encodes object_data opcode into PRG. */
    i960_mmio_write_u32((u32)(g4 + 0x800010), (u32)g14);
    /* @0x28658 ldq catalog[ix]; @0x28680 stq → prg_fifo (4 words). */
    {
        u32 cat = 0x2864b40u + ((u32)g5 << 4);
        u32 c0 = i960_ld_u32(I960_ROM, cat, 0);
        u32 c1 = i960_ld_u32(I960_ROM, cat, 4);
        u32 c2 = i960_ld_u32(I960_ROM, cat, 8);
        u32 c3 = i960_ld_u32(I960_ROM, cat, 12);

        *(u32 *)(fp + 0x60) = c0;
        *(u32 *)(fp + 0x64) = c1;
        *(u32 *)(fp + 0x68) = c2;
        *(u32 *)(fp + 0x6c) = c3;
        r7 = c3;
        i960_mmio_write_u32(0x804000, c0);
        i960_mmio_write_u32(0x804000, c1);
        i960_mmio_write_u32(0x804000, c2);
        i960_mmio_write_u32(0x804000, c3);
    }
    g4 = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
    r12 = r12 + 4;
    g9 = (uintptr_t)i960_vaddr_ptr(0x10802121);
    r13 = r13 + 1;
    i960_mmio_write_u32(0x884000, (u32)g9);
    g6 = g6 + 4;
    i960_st_u32(I960_WORKRAM, 0x20a290, 0, (u32)g6);
    g4 = g4 + r7;
    i960_st_u32(I960_WORKRAM, 0x20b940, 0, (u32)g4);
    if (r13 < r8)
        goto L_00028528;

    L_000286bc:
        /*
         * Disasm @ 0x286BC–0x2873C: abs(arg1+arg2) vs cvtir(fp+0x80);
         * g3 selects return float in g0 (caller ABI).
         */
        g10 = *(u32 *)(fp + 0x80);
        fp1 = (double)(i32)(u32)g10; /* cvtir g10,fp1 */
        g0 = i960_f64_to_u32(i960_u32_to_f64(arg1) + i960_u32_to_f64(arg2));
        fp0 = i960_u32_to_f64(g0);
        g11 = i960_f64_to_u32(fp1);
        fp1 = i960_u32_to_f64(g11);
        /* cpysre fp0,+0.0,fp0 → |fp0| */
        fp0 = fabs(fp0);
        if (fp0 < fp1)
            goto L_0002872c;
        if ((unsigned char)g3 == 0) {
            g0 = 0;
            goto L_0002872c;
        }
        if ((unsigned char)g3 == 1) {
            g12 = *(u32 *)(fp + 0x80);
            /* cvtir g12,g4; notbit 31; addrl +1.0 */
            g4 = i960_f64_to_u32((double)(i32)g12);
            g4 = g4 ^ (1u << 31);
            fp0 = i960_u32_to_f64(g4) + 1.0;
            g0 = i960_f64_to_u32(fp0);
            goto L_0002872c;
        }
        {
            u32 bits = *(u32 *)(fp + 0x80);

            /* cvtir; subrl +1.0 → value - 1.0 */
            fp0 = (double)(i32)bits - 1.0;
            g0 = i960_f64_to_u32(fp0);
        }

    L_0002872c:
        sp = sp - 0xa0;
}
