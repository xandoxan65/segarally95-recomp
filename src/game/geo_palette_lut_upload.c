/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_00003c80_20c.asm */
// @rom 0x3c80 +0x20c geo_palette_lut_upload

#include "i960_lift.h"
#include "i960_fp.h"
#include "model2_rom.h"
#include "i960_mem.h"

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret */
/* abi: u32 arg0=g0, u32 arg1=g1, u32 arg2=g2 → void */
/* call site: caller 0x3720 */
/* call site: caller 0x4bc0 */
/* call site: caller 0x3720 */

/* pointers: g0=unsigned short *, g1=void *, g2=void *, g3=unsigned short *, g5=void *, g6=unsigned short *, g7=unsigned short *, r10=void *, r11=void *, r13=void *, r14=void *, r9=void * */

void geo_palette_lut_upload(u32 arg0, u32 arg1, u32 arg2)
{
    unsigned short * arg0_p = (unsigned short *)arg0;
    void * a1 = (void *)arg1;
    void * a2 = (void *)arg2;

    r12 = i960_ld_u32(I960_WORKRAM, 0x5a2c74, 0);
    r9 = i960_ld_u32(I960_WORKRAM, 0x5a2c70, 0);
    g2 = 0x1800000;
    fp0 = i960_u32_to_f64(r12);
    g13 = 0;
    fp1 = i960_u32_to_f64(r9);
    r13 = 0xffff;
    r14 = 0xff;
    fp2 = i960_u32_to_f64(r9);
    i960_rifl_write(&r6, &r7, (1.0) - (fp0));
    i960_rifl_write(&r4, &r5, (1.0) - (fp1));

    L_00003cbc:
        r10 = 0xffff;
        g4 = r10 & g13;
        g5 = g4 << 3;
        g4 = g4 >> 2;
        g5 = g5 + g4;
        g4 = g5 & r10;
        fp0 = (double)(i32)(u32)(g4);
        r11 = i960_f64_to_u32(fp0);
        fp0 = i960_u32_to_f64(r11);
        r8 = 0;
        r9 = 0x406fe000;
        fp3 = i960_rifl_read(r8, r9);
        i960_rifl_write(&r8, &r9, (fp0) / (fp3));
        fp0 = i960_rifl_read(r8, r9);
        g6 = i960_f64_to_u32(fp0);
        fp1 = i960_u32_to_f64(g6);
        i960_rifl_write(&r8, &r9, (fp1) * (i960_rifl_read(r6, r7)));
        g4 = i960_f64_to_u32(sqrt(i960_u32_to_f64(g6)));
        g4 = i960_f64_to_u32((i960_u32_to_f64(r12)) * (i960_u32_to_f64(g4)));
        fp0 = i960_rifl_read(r8, r9);
        i960_rifl_write(&r10, &r11, (fp1) * (fp0));
        fp1 = i960_u32_to_f64(g4);
        fp0 = i960_rifl_read(r10, r11);
        i960_rifl_write(&r8, &r9, (fp0) + (fp1));
        g3 = (uintptr_t)i960_vaddr_ptr((u32)(g2 + 0x18000));
        g0 = (uintptr_t)i960_vaddr_ptr((u32)(g2 + 0x14000));
        g1 = 0;
        fp1 = i960_rifl_read(r8, r9);
        g7 = (uintptr_t)i960_vaddr_ptr((u32)(g2 + 0x10000));
        g6 = i960_f64_to_u32(fp1);

    L_00003d48:
        g4 = r13 & g1;
        fp0 = (double)(i32)(u32)(g4);
        r10 = i960_f64_to_u32(fp0);
        fp0 = i960_u32_to_f64(r10);
        r8 = 0;
        r9 = 0x404f8000;
        fp3 = i960_rifl_read(r8, r9);
        i960_rifl_write(&r8, &r9, (fp0) / (fp3));
        fp0 = i960_rifl_read(r8, r9);
        g4 = i960_f64_to_u32(fp0);
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(g6)));
        fp0 = i960_u32_to_f64(g4);
        i960_rifl_write(&r8, &r9, (fp0) * (i960_rifl_read(r4, r5)));
        g4 = i960_f64_to_u32(sqrt(i960_u32_to_f64(g4)));
        g4 = i960_f64_to_u32((fp2) * (i960_u32_to_f64(g4)));
        fp1 = i960_rifl_read(r8, r9);
        i960_rifl_write(&r10, &r11, (fp0) * (fp1));
        fp0 = i960_u32_to_f64(g4);
        fp1 = i960_rifl_read(r10, r11);
        i960_rifl_write(&r8, &r9, (fp1) + (fp0));
        fp0 = i960_rifl_read(r8, r9);
        r9 = i960_f64_to_u32(fp0);
        fp0 = i960_u32_to_f64(r9);
        fp0 = i960_u32_to_f64(r9);
        r10 = 0;
        r11 = 0x406fe000;
        fp3 = i960_rifl_read(r10, r11);
        i960_rifl_write(&r10, &r11, (fp3) * (fp0));
        fp0 = i960_rifl_read(r10, r11);
        { i64 _cvt = (i64)(fp0); g4 = (uintptr_t)(u32)_cvt; g5 = (uintptr_t)(u32)((u64)_cvt >> 32); };
        g5 = g4;
        g4 = g5 & r13;
        if ((u32)g4 > (u32)r14)
            g5 = 0xff;
    *(unsigned short *)g3 = (unsigned short)g5;
    g3 = g3 + 2;
    g1 = g1 + 0x1;
    r11 = 0x3f;
    g4 = r13 & g1;
    *(unsigned short *)g0 = (unsigned short)g5;
    *(unsigned short *)g7 = (unsigned short)g5;
    g0 = g0 + 2;
    g7 = g7 + 0x2;
    g2 = g2 + 2;
    if (g4 <= r11)
        goto L_00003d48;
    fp0 = i960_u32_to_f64(g6);
    r8 = 0;
    r9 = 0x406fe000;
    fp3 = i960_rifl_read(r8, r9);
    i960_rifl_write(&r8, &r9, (fp3) * (fp0));
    fp0 = i960_rifl_read(r8, r9);
    { i64 _cvt = (i64)(fp0); g4 = (uintptr_t)(u32)_cvt; g5 = (uintptr_t)(u32)((u64)_cvt >> 32); };
    g5 = g4;
    g4 = g5 & r13;
    if ((u32)g4 > (u32)r14)
        g5 = 0xff;
    g0 = (uintptr_t)i960_vaddr_ptr((u32)(g2 + 0x18000));
    g7 = (uintptr_t)i960_vaddr_ptr((u32)(g2 + 0x14000));
    g6 = (uintptr_t)i960_vaddr_ptr((u32)(g2 + 0x10000));
    g1 = 0xbf;
    do {
        *(unsigned short *)g0 = (unsigned short)g5;
        g0 = g0 + 2;
        g1 = g1 - 1;
        g4 = g1 & r13;
        *(unsigned short *)g7 = (unsigned short)g5;
        *(unsigned short *)g6 = (unsigned short)g5;
        g7 = g7 + 2;
        g6 = g6 + 0x2;
        g2 = g2 + 2;
    } while (g4 != r13);
    g13 = g13 + 1;
    g4 = g13 & r13;
    if (31 >= (unsigned char)g4)
        goto L_00003cbc;
    return;
}
