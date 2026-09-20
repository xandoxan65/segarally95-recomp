/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_010310_90.asm */
// @rom 0x10310 +0x90 comm_attract_script_glyph_dispatch

#include "i960_lift.h"
#include "i960_fp.h"
#include "model2_rom.h"

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret */
/* abi: u32 arg0=g0, u32 arg1=g1, u32 arg2=g2 → void */

static void glyph_add_base(void)
{
    fp1 = i960_rifl_read(r8, r9);
    i960_rifl_write(&r8, &r9, fp1 + fp0);
    fp0 = i960_rifl_read(r8, r9);
}

void comm_attract_script_glyph_dispatch(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg2;

    g4 = arg0 - 1u;
    if (g4 > 6u)
        goto L_000103a0;

    switch (g4) {
    case 0:
        fp0 = (double)(i32)(u32)arg1;
        r8 = 0;
        r9 = (uintptr_t)i960_vaddr_ptr(0x40836000);
        goto L_0001038c;
    case 1:
        fp0 = (double)(i32)(u32)arg1;
        r8 = 0;
        r9 = (uintptr_t)i960_vaddr_ptr(0xc05f0000);
        goto L_0001038c;
    case 2:
        fp0 = (double)(i32)(u32)arg1;
        r8 = 0;
        r9 = (uintptr_t)i960_vaddr_ptr(0x40955000);
        goto L_0001038c;
    case 3:
        fp0 = (double)(i32)(u32)arg1;
        r8 = 0;
        r9 = (uintptr_t)i960_vaddr_ptr(0xc08b2000);
        goto L_0001038c;
    case 4:
        fp0 = (double)(i32)(u32)arg1;
        r8 = 0;
        r9 = (uintptr_t)i960_vaddr_ptr(0x40836000);
        goto L_0001038c;
    case 5:
        fp0 = (double)(i32)(u32)arg1;
        r8 = 0;
        r9 = (uintptr_t)i960_vaddr_ptr(0xc05f0000);
        goto L_0001038c;
    case 6:
        fp0 = (double)(i32)(u32)arg1;
        r8 = 0;
        r9 = (uintptr_t)i960_vaddr_ptr(0xc08b2000);
        goto L_0001038c;
    default:
        break;
    }

    L_000103a0:
        g4 = i960_f64_to_u32(fp0);
        return;

    L_0001038c:
        glyph_add_base();
        g4 = i960_f64_to_u32(fp0);
}
