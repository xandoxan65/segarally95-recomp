/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_011b90_efc.asm */
// @rom 0x11b90 +0xefc comm_attract_geo_script_finish

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "model2_rom.h"
#include "i960_mem.h"
#include "model2_hw.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret  callee r4 */
/* abi: void * arg0=g0, void * arg1=g1, u32 arg2=g2 → void */

/* pointers: fp=u64 *, g0=u32 *, g1=u32 *, g2=u32, g3=u32, g4=u64 *, g5=unsigned char *, g6=u32, g7=u32 *, r4=u32 *, r6=u32, r8=void *, r9=void * */

extern void geo_attract_copro_vec_scale(u32 arg0, void * arg1, void * arg2);
extern void geo_attract_object_pen(void * arg0, u32 arg1, u32 arg2);
extern void geo_attract_copro_submit(u32 arg0, u32 arg1, u32 arg2);
extern void geo_attract_fp_series(u32 table_va, u32 arg1, u32 arg2);
extern u32 geo_attract_course_carousel_draw(u32 arg0, void * arg1, u32 arg2);
extern void geo_attract_record_index(u32 arg0, u32 arg1, u32 arg2);

void comm_attract_geo_script_finish(void * arg0, void * arg1, u32 arg2)
{
    u32 * arg0_p = (u32 *)arg0;
    uintptr_t fp_save = fp;
    /*
     * Private frame — do not use global `sp`. Attract bind_fp points `fp` at a
     * host shadow buffer, while script_frame_setup leaks `sp += 0x30` every
     * call. `fp = sp` then wrote scene/delta into rising guest-ish addresses
     * and segfaulted (memmove to ~0x78e430). Same pattern as object_extra:
     * stack-local buffer, fp relative to it. Locals use 0x40(fp)…0x88(fp).
     */
    u8 frame[0xc0];
    /* Pair object — keep a C local; nested lifts clobber global r5. */
    uintptr_t pair_obj = (uintptr_t)arg1;

    /*
     * Call sites pass g0 = caller's object (string_draw). Prologue stores the
     * scene row into *this* frame's 0x40(fp); `ld (g0)` still reads the object.
     */

    g0 = (uintptr_t)arg0;
    g1 = (uintptr_t)arg1;
    g2 = (uintptr_t)arg2;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    g4 = i960_ld_u32(I960_WORKRAM, 0x20a800, 0);
    g5 = i960_ld_u32(I960_WORKRAM, 0x20a80c, 0);
    /* Disasm: lda 0x11802323 / 0x12802525 — marker immediates, not pointers. */
    i960_mmio_write_u32(0x884000, 0x11802323u);
    g6 = i960_ld_u32(I960_WORKRAM, 0x20a7f4, 0);
    i960_mmio_write_u32(0x884000, 0x12802525u);
    g4 = g4 + (u32)(g4 << 3);
    /* g5 is a guest workram base from 0x20a80c — not a host pointer. */
    r6 = i960_ld_u8(I960_WORKRAM, (u32)g5, 0x14 + ((u32)(g4 << 2)));
    r4 = (uintptr_t)arg0_p;
    /* Disasm: movl g0,r4 copies g0+g1 → r4+r5. Pair calls pass g1=0xa0(fp). */
    r5 = pair_obj;
    if ((unsigned char)r6 != g6) {
        /* ROM st g14 — frame counter reset (link ≈ 0 as small ROM IP / cleared). */
        i960_st_u32(I960_WORKRAM, 0x20a7f0, 0, 0);
    }
    g4 = i960_ld_u32(I960_WORKRAM, 0x20a800, 0);
    g5 = i960_ld_u32(I960_WORKRAM, 0x20a80c, 0);
    i960_st_u32(I960_WORKRAM, 0x20a7f4, 0, (u32)r6);
    g4 = g4 + (u32)(g4 << 3);
    g4 = g5 + (u32)(g4 << 2);
    {
        u32 row = (u32)g4;
        /* Disasm: ldl (g4),r8 / stl r8,0x40(fp) — X and Y as a pair. */
        *(u64 *)(fp + 0x40) = i960_ld_u64(I960_WORKRAM, row, 0);
        g4 = i960_ld_u32(I960_WORKRAM, row, 0x8);
    }
    *(u32 *)(fp + 0x48) = (u32)g4;
    if (r6 > 11)
        goto L_000128ac;
    /* bx 0x5b0c2c[r6] — intra-function jump table (staged workram → ROM labels). */
    switch ((unsigned)r6) {
    case 0: goto L_00011c5c;
    case 1: goto L_00011d50;
    case 2: goto L_00011ddc;
    case 3: goto L_00011e60;
    case 4: goto L_00011f24;
    case 5: goto L_00011fbc;
    case 6: goto L_000120fc;
    case 7: goto L_00012268;
    case 8: goto L_00012308;
    case 9: goto L_000123b8;
    case 10: goto L_00012450;
    case 11: goto L_0001267c;
    default: goto L_000128ac;
    }

    L_00011c5c:
    g4 = *(u32 *)((uintptr_t)arg0_p + 0x20);
    g4 = g4 ^ (1u << 31);
    r9 = (uintptr_t)i960_vaddr_ptr(0x15802b2b);
    i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g4 = *(u32 *)((uintptr_t)arg0_p + 0x18);
    g4 = g4 ^ (1u << 31);
    r8 = (uintptr_t)i960_vaddr_ptr(0x14802929);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g4 = *(u32 *)((uintptr_t)arg0_p + 0x1c);
    g4 = g4 ^ (1u << 31);
    r9 = (uintptr_t)i960_vaddr_ptr(0x15002a2a);
    i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g4 = *(u32 *)((uintptr_t)arg0_p + 0x4);
    g4 = g4 ^ (1u << 31);
    fp0 = i960_u32_to_f64(g4);
    r8 = 0;
    /* Disasm: lda 0x3fe80000,r9 / movrl → double 0.75 (not a guest pointer). */
    r9 = 0x3fe80000u;
    fp1 = i960_rifl_read(r8, r9);
    g6 = *(u32 *)arg0_p;
    i960_rifl_write(&r8, &r9, (fp0) - (fp1));
    g7 = (uintptr_t)arg0_p + 0x8;
    g4 = *(u32 *)g7;
    g6 = g6 ^ (1u << 31);
    g4 = g4 ^ (1u << 31);
    fp0 = i960_rifl_read(r8, r9);
    r9 = (uintptr_t)i960_vaddr_ptr(0x13802727);
    i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
    g5 = i960_f64_to_u32(fp0);
    i960_mmio_write_u32(0x884000, (u32)g6); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    /* Disasm: ldl (g0),r8 / stl r8,0x40(fp) — keep object X/Y pair. */
    *(u64 *)(fp + 0x40) = *(u64 *)arg0_p;
    r9 = *(u32 *)(fp + 0x44);
    fp0 = i960_u32_to_f64(r9);
    fp0 = i960_u32_to_f64(r9);
    i960_rifl_write(&r8, &r9, (fp1) + (fp0));
    fp0 = i960_rifl_read(r8, r9);
    g5 = i960_f64_to_u32(fp0);
    g4 = *(u32 *)g7;
    *(u32 *)(fp + 0x44) = (u32)g5;
    *(u32 *)(fp + 0x48) = (u32)g4;
    goto L_000128ac;
    L_00011d50:
    /* Mode 1: ld (g0) after scene→0x40(fp) alias. */
    g5 = arg0_p ? arg0_p[0] : 0;
    g2 = *(u32 *)(fp + 0x40);
    g5 = i960_f64_to_u32((i960_u32_to_f64(g5)) - (i960_u32_to_f64(g2)));
    i960_st_u32(I960_WORKRAM, 0x20a7e0, 0, (u32)g5);
    g1 = *(u32 *)(fp + 0x44);
    g4 = arg0_p ? arg0_p[1] : 0;
    g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) - (i960_u32_to_f64(g1)));
    i960_st_u32(I960_WORKRAM, 0x20a7e4, 0, (u32)g4);
    g7 = arg0_p ? arg0_p[2] : 0;
    r9 = (uintptr_t)i960_vaddr_ptr(0x2a005454);
    i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
    g6 = *(u32 *)(fp + 0x48);
    i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
    g2 = g2 ^ (1u << 31);
    g1 = g1 ^ (1u << 31);
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g7 = i960_f64_to_u32((i960_u32_to_f64(g7)) - (i960_u32_to_f64(g6)));
    g6 = g6 ^ (1u << 31);
    i960_mmio_write_u32(0x884000, (u32)g7); /* copro_fifo */;
    r8 = (uintptr_t)i960_vaddr_ptr(0x13802727);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g2); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g1); /* copro_fifo */;
    goto L_000120e8;
    L_00011ddc:
    /* Mode 2: ld (g1) — pair object (prologue r5 / pair_obj). */
    if (!pair_obj)
        goto L_000128ac;
    g5 = *(u32 *)pair_obj;
    g2 = *(u32 *)(fp + 0x40);
    g5 = i960_f64_to_u32((i960_u32_to_f64(g5)) - (i960_u32_to_f64(g2)));
    i960_st_u32(I960_WORKRAM, 0x20a7e0, 0, (u32)g5);
    g0 = *(u32 *)(fp + 0x44);
    g4 = *(u32 *)(pair_obj + 0x4);
    g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) - (i960_u32_to_f64(g0)));
    i960_st_u32(I960_WORKRAM, 0x20a7e4, 0, (u32)g4);
    g7 = *(u32 *)(pair_obj + 0x8);
    r9 = (uintptr_t)i960_vaddr_ptr(0x2a005454);
    i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
    g6 = *(u32 *)(fp + 0x48);
    i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
    g2 = g2 ^ (1u << 31);
    g0 = g0 ^ (1u << 31);
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g7 = i960_f64_to_u32((i960_u32_to_f64(g7)) - (i960_u32_to_f64(g6)));
    g6 = g6 ^ (1u << 31);
    i960_mmio_write_u32(0x884000, (u32)g7); /* copro_fifo */;
    r8 = (uintptr_t)i960_vaddr_ptr(0x13802727);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g2); /* copro_fifo */;
    goto L_000120e0;
    L_00011e60:
    g4 = i960_ld_u32(I960_WORKRAM, 0x20a800, 0);
    g5 = i960_ld_u32(I960_WORKRAM, 0x20a80c, 0);
    g4 = g4 + (u32)(g4 << 3);
    g4 = i960_ld_u32(I960_WORKRAM, g5, 0x18 + ((u32)(g4 << 2)));
    g1 = *(u32 *)(fp + 0x40);
    g4 = g4 + (u32)(g4 << 2);
    g4 = (u32)(g4 << 2); /* index into 0x5acbe0 vec3 table */
    g6 = i960_ld_u32(I960_WORKRAM, 0x5acbe0, g4);
    g6 = i960_f64_to_u32((i960_u32_to_f64(g6)) - (i960_u32_to_f64(g1)));
    i960_st_u32(I960_WORKRAM, 0x20a7e0, 0, (u32)g6);
    g0 = *(u32 *)(fp + 0x44);
    g5 = i960_ld_u32(I960_WORKRAM, 0x5acbe0, g4 + 4);
    g5 = i960_f64_to_u32((i960_u32_to_f64(g5)) - (i960_u32_to_f64(g0)));
    i960_st_u32(I960_WORKRAM, 0x20a7e4, 0, (u32)g5);
    g4 = i960_ld_u32(I960_WORKRAM, 0x5acbe0, g4 + 8);
    r9 = (uintptr_t)i960_vaddr_ptr(0x2a005454);
    i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
    g7 = *(u32 *)(fp + 0x48);
    i960_mmio_write_u32(0x884000, (u32)g6); /* copro_fifo */;
    g1 = g1 ^ (1u << 31);
    g0 = g0 ^ (1u << 31);
    i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
    g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) - (i960_u32_to_f64(g7)));
    g7 = g7 ^ (1u << 31);
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    r8 = (uintptr_t)i960_vaddr_ptr(0x13802727);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g1); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g0); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g7); /* copro_fifo */;
    i960_st_u32(I960_WORKRAM, 0x20a7e8, 0, (u32)g4);
    goto L_000128ac;
    L_00011f24:
    /* Disasm: ldl 0x40(fp),r8 / stl r8,0x50(fp) — scene X/Y copy. */
    *(u64 *)(fp + 0x50) = *(u64 *)(fp + 0x40);
    g4 = *(u32 *)(fp + 0x48);
    *(u32 *)(fp + 0x58) = (u32)g4;
    /* Mode 4: ld (g0) = object; 0x50(fp) = scene copy. T = −(obj−scene). */
    g6 = arg0_p ? arg0_p[0] : 0;
    g2 = *(u32 *)(fp + 0x50);
    g6 = i960_f64_to_u32((i960_u32_to_f64(g6)) - (i960_u32_to_f64(g2)));
    *(u32 *)(fp + 0x40) = (u32)g6;
    g1 = *(u32 *)(fp + 0x54);
    g4 = arg0_p ? arg0_p[1] : 0;
    g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) - (i960_u32_to_f64(g1)));
    g7 = *(u32 *)(fp + 0x58);
    *(u32 *)(fp + 0x44) = (u32)g4;
    g5 = arg0_p ? arg0_p[2] : 0;
    r9 = (uintptr_t)i960_vaddr_ptr(0x2a005454);
    g5 = i960_f64_to_u32((i960_u32_to_f64(g5)) - (i960_u32_to_f64(g7)));
    {
        static unsigned s_m4;

        if (s_m4 < 6u || (s_m4 % 60u) == 0u) {
            lift_log(
                    "lift: mode4 obj=(%.3g,%.3g,%.3g) scene=(%.3g,%.3g,%.3g) "
                    "delta=(%.3g,%.3g,%.3g)\n",
                    (float)i960_u32_to_f64(arg0_p ? arg0_p[0] : 0),
                    (float)i960_u32_to_f64(arg0_p ? arg0_p[1] : 0),
                    (float)i960_u32_to_f64(arg0_p ? arg0_p[2] : 0),
                    (float)i960_u32_to_f64(g2),
                    (float)i960_u32_to_f64(g1),
                    (float)i960_u32_to_f64(g7),
                    (float)i960_u32_to_f64(g6),
                    (float)i960_u32_to_f64(g4),
                    (float)i960_u32_to_f64(g5));
            s_m4++;
        }
    }
    i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g2); /* copro_fifo */;
    g6 = g6 ^ (1u << 31);
    i960_mmio_write_u32(0x884000, (u32)g1); /* copro_fifo */;
    g4 = g4 ^ (1u << 31);
    i960_mmio_write_u32(0x884000, (u32)g7); /* copro_fifo */;
    g7 = g5 ^ (1u << 31);
    r8 = (uintptr_t)i960_vaddr_ptr(0x13802727);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g6); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    *(u32 *)(fp + 0x48) = (u32)g5;
    goto L_000128a4;
    L_00011fbc:
    g4 = i960_ld_u32(I960_WORKRAM, 0x20a800, 0);
    g5 = i960_ld_u32(I960_WORKRAM, 0x20a80c, 0);
    g4 = g4 + (u32)(g4 << 3);
    g4 = i960_ld_u32(I960_WORKRAM, g5, 0x18 + ((u32)(g4 << 2)));
    g5 = i960_ld_u32(I960_WORKRAM, 0x20a7f0, 0);
    g4 = g4 + (u32)(g4 << 2);
    g4 = i960_ld_u32(I960_WORKRAM, 0x5acbec, (u32)(g4 << 2));
    g5 = i960_f64_to_u32((double)(i32)(u32)(g5));
    g4 = i960_f64_to_u32((double)(i32)(u32)(g4));
    g1 = i960_f64_to_u32((i960_u32_to_f64(g5)) / (i960_u32_to_f64(g4)));
    fp0 = i960_u32_to_f64(g1);
    if (fp0 > 1.0) {
        /* Disasm: lda 0x3f800000,g1 — float 1.0 bits. */
        g1 = 0x3f800000u;
    }
    g4 = i960_ld_u32(I960_WORKRAM, 0x20a7f0, 0);
    g5 = i960_ld_u32(I960_WORKRAM, 0x20a800, 0);
    g6 = i960_ld_u32(I960_WORKRAM, 0x20a80c, 0);
    g4 = g4 + 1;
    i960_st_u32(I960_WORKRAM, 0x20a7f0, 0, (u32)g4);
    g5 = g5 + (u32)(g5 << 3);
    g4 = i960_ld_u32(I960_WORKRAM, (u32)g6, 0x18 + ((u32)(g5 << 2)));
    g4 = g4 + (u32)(g4 << 2);
    g0 = i960_ld_u32(I960_WORKRAM, 0x5acbf0, (u32)(g4 << 2));
    g0 = g0 + (u32)(g0 << 1);
    g0 = 0x5ae3d0 + (u32)(g0 << 2);
    /* Matrix/coeff table lives in workram — resolve guest VA before C pointer use. */
    geo_attract_fp_series((u32)g0, g1, g2);
    /* Disasm: stl g0,0x40(fp) — fp_series returns X/Y in g0/g1 via ldq. */
    *(u32 *)(fp + 0x40) = (u32)g0;
    *(u32 *)(fp + 0x44) = (u32)g1;
    *(u32 *)(fp + 0x48) = (u32)g2;
    /* Mode 5: ld (r4) = object; 0x40(fp) = fp_series result. */
    g5 = arg0_p ? arg0_p[0] : 0;
    g1 = *(u32 *)(fp + 0x40);
    g5 = i960_f64_to_u32((i960_u32_to_f64(g5)) - (i960_u32_to_f64(g1)));
    i960_st_u32(I960_WORKRAM, 0x20a7e0, 0, (u32)g5);
    g0 = *(u32 *)(fp + 0x44);
    g4 = arg0_p ? arg0_p[1] : 0;
    g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) - (i960_u32_to_f64(g0)));
    i960_st_u32(I960_WORKRAM, 0x20a7e4, 0, (u32)g4);
    g7 = arg0_p ? arg0_p[2] : 0;
    r9 = (uintptr_t)i960_vaddr_ptr(0x2a005454);
    i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
    g6 = *(u32 *)(fp + 0x48);
    i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
    g1 = g1 ^ (1u << 31);
    g0 = g0 ^ (1u << 31);
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g7 = i960_f64_to_u32((i960_u32_to_f64(g7)) - (i960_u32_to_f64(g6)));
    g6 = g6 ^ (1u << 31);
    i960_mmio_write_u32(0x884000, (u32)g7); /* copro_fifo */;
    r8 = (uintptr_t)i960_vaddr_ptr(0x13802727);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g1); /* copro_fifo */;

    L_000120e0:
        i960_mmio_write_u32(0x884000, (u32)g0); /* copro_fifo */;

    L_000120e8:
        i960_mmio_write_u32(0x884000, (u32)g6); /* copro_fifo */;
        i960_st_u32(I960_WORKRAM, 0x20a7e8, 0, (u32)g7);
        goto L_000128ac;
        L_000120fc:
    g4 = i960_ld_u32(I960_WORKRAM, 0x20a7fc, 0);
        fp0 = (double)(i32)(u32)(g4);
        r9 = i960_f64_to_u32(fp0);
        fp0 = i960_u32_to_f64(r9);
        r8 = 0;
        r9 = 0x4079c000;
        fp1 = i960_rifl_read(r8, r9);
        i960_rifl_write(&r8, &r9, (fp0) / (fp1));
        fp0 = i960_rifl_read(r8, r9);
        g1 = i960_f64_to_u32(fp0);
        fp0 = i960_u32_to_f64(g1);
        /*
         * Disasm: lda imm → st 0x60/64/68(fp). These are float bit-patterns
         * (endpoint XYZ), not guest addresses — do not wrap i960_vaddr_ptr.
         */
        *(u32 *)(fp + 0x60) = 0xc29d3845u;
        *(u32 *)(fp + 0x64) = 0x41777ae1u;
        *(u32 *)(fp + 0x68) = 0xc404d2a3u;
        if (fp0 >= 1.0)
            goto L_000121c0;
        /* Disasm: lda 0x5ae400,g0 — ROM-mirrored control-point table. */
        geo_attract_fp_series(0x5ae400u, g1, g2);
    i960_st_u32(I960_WORKRAM, 0x20a820, 0, (u32)g0); i960_st_u32(I960_WORKRAM, 0x20a820, 4, (u32)g1);
    i960_st_u32(I960_WORKRAM, 0x20a828, 0, (u32)g2);
    g4 = *(u32 *)(fp + 0x60);
    g5 = *(u32 *)(fp + 0x64);
    g6 = *(u32 *)(fp + 0x68);
    g7 = i960_ld_u32(I960_WORKRAM, 0x20a820, 0);
    g0 = i960_ld_u32(I960_WORKRAM, 0x20a824, 0);
    g1 = i960_ld_u32(I960_WORKRAM, 0x20a828, 0);
    g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) - (i960_u32_to_f64(g7)));
    g5 = i960_f64_to_u32((i960_u32_to_f64(g5)) - (i960_u32_to_f64(g0)));
    g6 = i960_f64_to_u32((i960_u32_to_f64(g6)) - (i960_u32_to_f64(g1)));
    i960_st_u32(I960_WORKRAM, 0x20a7e0, 0, (u32)g4);
    i960_st_u32(I960_WORKRAM, 0x20a7e4, 0, (u32)g5);
    i960_st_u32(I960_WORKRAM, 0x20a7e8, 0, (u32)g6);

    L_000121c0:
        g7 = i960_ld_u32(I960_WORKRAM, 0x20a7e0, 0);
        g0 = i960_ld_u32(I960_WORKRAM, 0x20a7e4, 0);
        g1 = i960_ld_u32(I960_WORKRAM, 0x20a7e8, 0);
        /* Disasm: ldl 0x20a820,r8 / stl r8,0x40(fp). */
        *(u64 *)(fp + 0x40) = i960_ld_u64(I960_WORKRAM, 0x20a820, 0);
        g4 = *(u32 *)(fp + 0x40);
        g5 = *(u32 *)(fp + 0x44);
        r9 = (uintptr_t)i960_vaddr_ptr(0x2a005454);
        i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
        g6 = i960_ld_u32(I960_WORKRAM, 0x20a828, 0);
        i960_mmio_write_u32(0x884000, (u32)g7); /* copro_fifo */;
        g7 = i960_ld_u32(I960_WORKRAM, 0x20a828, 0);
        i960_mmio_write_u32(0x884000, (u32)g0); /* copro_fifo */;
        g4 = g4 ^ (1u << 31);
        g5 = g5 ^ (1u << 31);
        i960_mmio_write_u32(0x884000, (u32)g1); /* copro_fifo */;
        g6 = g6 ^ (1u << 31);
        r8 = (uintptr_t)i960_vaddr_ptr(0x13802727);
        i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
        *(u32 *)(fp + 0x48) = (u32)g7;
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g6); /* copro_fifo */;
        goto L_000128ac;
        L_00012268:
        g4 = i960_ld_u32(I960_WORKRAM, 0x20a800, 0);
        g7 = i960_ld_u32(I960_WORKRAM, 0x20a80c, 0);
        /* Mode 7: ld (g0) = object; 0x40(fp) = scene (this frame). */
        g5 = arg0_p ? arg0_p[0] : 0;
        g6 = *(u32 *)(fp + 0x40);
        g4 = g4 + (u32)(g4 << 3);
        g4 = i960_ld_u32(I960_WORKRAM, (u32)g7, 0x18 + ((u32)(g4 << 2)));
        g5 = i960_f64_to_u32((i960_u32_to_f64(g5)) - (i960_u32_to_f64(g6)));
        g4 = g4 + (u32)(g4 << 2);
        g4 = (u32)(g4 << 2);
        g1 = i960_ld_u32(I960_WORKRAM, 0x5acbe0, g4);
        g2 = i960_ld_u32(I960_WORKRAM, 0x5acbe0, g4 + 4);
        i960_st_u32(I960_WORKRAM, 0x20a7e0, 0, (u32)g5);
        g6 = *(u32 *)(fp + 0x44);
        g4 = arg0_p ? arg0_p[1] : 0;
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) - (i960_u32_to_f64(g6)));
        i960_st_u32(I960_WORKRAM, 0x20a7e4, 0, (u32)g4);
        g7 = *(u32 *)(fp + 0x48);
        g6 = arg0_p ? arg0_p[2] : 0;
        g5 = i960_f64_to_u32((i960_u32_to_f64(g5)) * (i960_u32_to_f64(g5)));
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(g4)));
        g6 = i960_f64_to_u32((i960_u32_to_f64(g6)) - (i960_u32_to_f64(g7)));
        g7 = i960_f64_to_u32((i960_u32_to_f64(g6)) * (i960_u32_to_f64(g6)));
        g5 = i960_f64_to_u32((i960_u32_to_f64(g4)) + (i960_u32_to_f64(g5)));
        g5 = i960_f64_to_u32((i960_u32_to_f64(g7)) + (i960_u32_to_f64(g5)));
        g0 = i960_f64_to_u32(sqrt(i960_u32_to_f64(g5)));
        i960_st_u32(I960_WORKRAM, 0x20a7e8, 0, (u32)g6);
        if (i960_u32_to_f64(g0) >= i960_u32_to_f64(g1))
            goto L_000122fc;
        g0 = 0;
        goto L_000123a4;

    L_000122fc:
        {
            static unsigned s_m7;
            float dist = (float)i960_u32_to_f64(g0);
            float radius = (float)i960_u32_to_f64(g1);
            float falloff = (float)i960_u32_to_f64(g2);

            g4 = i960_f64_to_u32((i960_u32_to_f64(g2)) * (i960_u32_to_f64(g0)));
            g0 = i960_f64_to_u32((i960_u32_to_f64(g1)) - (i960_u32_to_f64(g4)));
            if (s_m7 < 4u) {
                lift_log(
                        "lift: mode7 radius=%.3g falloff=%.3g |delta|=%.3g "
                        "scale=%.3g scene=(%.3g,%.3g,%.3g)\n",
                        radius, falloff, dist,
                        (float)i960_u32_to_f64(g0),
                        (float)i960_u32_to_f64(*(u32 *)(fp + 0x40)),
                        (float)i960_u32_to_f64(*(u32 *)(fp + 0x44)),
                        (float)i960_u32_to_f64(*(u32 *)(fp + 0x48)));
                s_m7++;
            }
        }
        goto L_000123a4;
        L_00012308:
        /* Mode 8: same dolly as mode 7, but ld (g1) = pair object. */
        if (!pair_obj)
            goto L_000128ac;
        g4 = i960_ld_u32(I960_WORKRAM, 0x20a800, 0);
        g7 = i960_ld_u32(I960_WORKRAM, 0x20a80c, 0);
        g5 = *(u32 *)pair_obj;
        g6 = *(u32 *)(fp + 0x40);
        g4 = g4 + (u32)(g4 << 3);
        g4 = i960_ld_u32(I960_WORKRAM, g7, 0x18 + ((u32)(g4 << 2)));
        g5 = i960_f64_to_u32((i960_u32_to_f64(g5)) - (i960_u32_to_f64(g6)));
        g4 = g4 + (u32)(g4 << 2);
        g4 = (u32)(g4 << 2);
        g2 = i960_ld_u32(I960_WORKRAM, 0x5acbe0, g4);
        g3 = i960_ld_u32(I960_WORKRAM, 0x5acbe0, g4 + 4);
        i960_st_u32(I960_WORKRAM, 0x20a7e0, 0, (u32)g5);
        g6 = *(u32 *)(fp + 0x44);
        g4 = *(u32 *)(pair_obj + 0x4);
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) - (i960_u32_to_f64(g6)));
        i960_st_u32(I960_WORKRAM, 0x20a7e4, 0, (u32)g4);
        g7 = *(u32 *)(fp + 0x48);
        g6 = *(u32 *)(pair_obj + 0x8);
        g5 = i960_f64_to_u32((i960_u32_to_f64(g5)) * (i960_u32_to_f64(g5)));
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(g4)));
        g6 = i960_f64_to_u32((i960_u32_to_f64(g6)) - (i960_u32_to_f64(g7)));
        g7 = i960_f64_to_u32((i960_u32_to_f64(g6)) * (i960_u32_to_f64(g6)));
        g5 = i960_f64_to_u32((i960_u32_to_f64(g4)) + (i960_u32_to_f64(g5)));
        g5 = i960_f64_to_u32((i960_u32_to_f64(g7)) + (i960_u32_to_f64(g5)));
        g0 = i960_f64_to_u32(sqrt(i960_u32_to_f64(g5)));
        i960_st_u32(I960_WORKRAM, 0x20a7e8, 0, (u32)g6);
        if (i960_u32_to_f64(g0) >= i960_u32_to_f64(g2))
            goto L_0001239c;
        g0 = 0;
        goto L_000123a4;

    L_0001239c:
        g4 = i960_f64_to_u32((i960_u32_to_f64(g3)) * (i960_u32_to_f64(g0)));
        g0 = i960_f64_to_u32((i960_u32_to_f64(g2)) - (i960_u32_to_f64(g4)));

    L_000123a4:
        /* Disasm: lda 0x20a7e0,g1 — delta in CRX window @ 0x0020a7e0. */
        g1 = (uintptr_t)i960_vaddr_ptr(0x20a7e0u);
        g2 = fp + 0x40;
        geo_attract_copro_vec_scale((u32)g0, (void *)g1, (void *)g2);
    goto L_000128ac;
    L_000123b8:
    /* Mode 9: mode-4 style close-up on pair object (ld g1). */
    if (!pair_obj)
        goto L_000128ac;
    /* Disasm: ldl 0x40(fp),r8 / stl r8,0x50(fp). */
    *(u64 *)(fp + 0x50) = *(u64 *)(fp + 0x40);
    g4 = *(u32 *)(fp + 0x48);
    *(u32 *)(fp + 0x58) = (u32)g4;
    g6 = *(u32 *)pair_obj;
    g2 = *(u32 *)(fp + 0x50);
    g6 = i960_f64_to_u32((i960_u32_to_f64(g6)) - (i960_u32_to_f64(g2)));
    *(u32 *)(fp + 0x40) = (u32)g6;
    g0 = *(u32 *)(fp + 0x54);
    g4 = *(u32 *)(pair_obj + 0x4);
    g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) - (i960_u32_to_f64(g0)));
    g7 = *(u32 *)(fp + 0x58);
    *(u32 *)(fp + 0x44) = (u32)g4;
    g5 = *(u32 *)(pair_obj + 0x8);
    r9 = (uintptr_t)i960_vaddr_ptr(0x2a005454);
    g5 = i960_f64_to_u32((i960_u32_to_f64(g5)) - (i960_u32_to_f64(g7)));
    i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g2); /* copro_fifo */;
    g6 = g6 ^ (1u << 31);
    i960_mmio_write_u32(0x884000, (u32)g0); /* copro_fifo */;
    g4 = g4 ^ (1u << 31);
    i960_mmio_write_u32(0x884000, (u32)g7); /* copro_fifo */;
    g7 = g5 ^ (1u << 31);
    r8 = (uintptr_t)i960_vaddr_ptr(0x13802727);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g6); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    *(u32 *)(fp + 0x48) = (u32)g5;
    goto L_000128a4;
    L_00012450:
    /* Disasm: ldl 0x5c7890,r8 / stl r8,0x60(fp). */
    *(u64 *)(fp + 0x60) = i960_ld_u64(I960_WORKRAM, 0x5c7890, 0);
    g4 = i960_ld_u32(I960_WORKRAM, 0x5c7898, 0);
    r9 = (uintptr_t)i960_vaddr_ptr(0x10002020);
    i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
    *(u32 *)(fp + 0x68) = (u32)g4;
    g4 = *(u32 *)(g0 + 0x1c);
    r8 = (uintptr_t)i960_vaddr_ptr(0x15002a2a);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g4 = *(u32 *)(g0 + 0x18);
    r9 = (uintptr_t)i960_vaddr_ptr(0x14802929);
    i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
    g5 = *(u32 *)(fp + 0x40);
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g4 = *(u32 *)(g0 + 0x20);
    r8 = (uintptr_t)i960_vaddr_ptr(0x15802b2b);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    g6 = *(u32 *)(fp + 0x44);
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    r9 = (uintptr_t)i960_vaddr_ptr(0x13802727);
    i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
    g7 = *(u32 *)(fp + 0x48);
    i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g6); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g7); /* copro_fifo */;
    r8 = (uintptr_t)i960_vaddr_ptr(0x16002c2c);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    g4 = *(u32 *)(fp + 0x60);
    g5 = *(u32 *)(fp + 0x64);
    g6 = *(u32 *)(fp + 0x68);
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g6); /* copro_fifo */;
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    *(u32 *)(fp + 0x70) = (u32)g4;
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    *(u32 *)(fp + 0x74) = (u32)g4;
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    r9 = (uintptr_t)i960_vaddr_ptr(0x10802121);
    i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
    *(u32 *)(fp + 0x78) = (u32)g4;
    g2 = *(u32 *)g0;
    g4 = *(u32 *)(fp + 0x70);
    g2 = i960_f64_to_u32((i960_u32_to_f64(g2)) - (i960_u32_to_f64(g4)));
    *(u32 *)(fp + 0x40) = (u32)g2;
    g1 = *(u32 *)(g0 + 0x4);
    g4 = *(u32 *)(fp + 0x74);
    g1 = i960_f64_to_u32((i960_u32_to_f64(g1)) - (i960_u32_to_f64(g4)));
    g4 = *(u32 *)(fp + 0x78);
    *(u32 *)(fp + 0x44) = (u32)g1;
    g7 = *(u32 *)(g0 + 0x8);
    g6 = i960_ld_u32(I960_WORKRAM, 0x20a800, 0);
    g7 = i960_f64_to_u32((i960_u32_to_f64(g7)) - (i960_u32_to_f64(g4)));
    g4 = i960_ld_u32(I960_WORKRAM, 0x20a80c, 0);
    *(u32 *)(fp + 0x48) = (u32)g7;
    g5 = *(u32 *)(g0 + 0x20);
    g6 = g6 + (u32)(g6 << 3);
    g6 = g4 + (u32)(g6 << 2); /* guest row @ 0x20a80c */
    g4 = i960_ld_u32(I960_WORKRAM, (u32)g6, 0x18);
    g5 = g5 ^ (1u << 31);
    g4 = g4 + (u32)(g4 << 2);
    g4 = i960_ld_u32(I960_WORKRAM, 0x5acbe8, (u32)(g4 << 2));
    g5 = i960_f64_to_u32((i960_u32_to_f64(g4)) + (i960_u32_to_f64(g5)));
    r8 = (uintptr_t)i960_vaddr_ptr(0x15802b2b);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
    g5 = *(u32 *)(g0 + 0x18);
    g4 = i960_ld_u32(I960_WORKRAM, (u32)g6, 0x18);
    g5 = g5 ^ (1u << 31);
    g4 = g4 + (u32)(g4 << 2);
    g4 = i960_ld_u32(I960_WORKRAM, 0x5acbe0, (u32)(g4 << 2));
    g5 = i960_f64_to_u32((i960_u32_to_f64(g4)) + (i960_u32_to_f64(g5)));
    r9 = (uintptr_t)i960_vaddr_ptr(0x14802929);
    i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
    g5 = *(u32 *)(g0 + 0x1c);
    g4 = i960_ld_u32(I960_WORKRAM, (u32)g6, 0x18);
    g5 = g5 ^ (1u << 31);
    g4 = g4 + (u32)(g4 << 2);
    g4 = i960_ld_u32(I960_WORKRAM, 0x5acbe4, (u32)(g4 << 2));
    g5 = i960_f64_to_u32((i960_u32_to_f64(g4)) + (i960_u32_to_f64(g5)));
    r8 = (uintptr_t)i960_vaddr_ptr(0x15002a2a);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    g2 = g2 ^ (1u << 31);
    g1 = g1 ^ (1u << 31);
    g7 = g7 ^ (1u << 31);
    i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
    r9 = (uintptr_t)i960_vaddr_ptr(0x13802727);
    i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g2); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g1); /* copro_fifo */;
    goto L_000128a4;
    L_0001267c:
    /* Mode 11: object angles from pair (g1); scale from 0x5c7890. */
    if (!pair_obj)
        goto L_000128ac;
    /* Disasm: ldl 0x5c7890,r8 / stl r8,0x70(fp). */
    *(u64 *)(fp + 0x70) = i960_ld_u64(I960_WORKRAM, 0x5c7890, 0);
    g4 = i960_ld_u32(I960_WORKRAM, 0x5c7898, 0);
    r9 = (uintptr_t)i960_vaddr_ptr(0x10002020);
    i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
    *(u32 *)(fp + 0x78) = (u32)g4;
    g4 = *(u32 *)(pair_obj + 0x1c);
    r8 = (uintptr_t)i960_vaddr_ptr(0x15002a2a);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g4 = *(u32 *)(pair_obj + 0x18);
    r9 = (uintptr_t)i960_vaddr_ptr(0x14802929);
    i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
    g5 = *(u32 *)(fp + 0x40);
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g4 = *(u32 *)(pair_obj + 0x20);
    r8 = (uintptr_t)i960_vaddr_ptr(0x15802b2b);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    g6 = *(u32 *)(fp + 0x44);
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    r9 = (uintptr_t)i960_vaddr_ptr(0x13802727);
    i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
    g7 = *(u32 *)(fp + 0x48);
    i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g6); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g7); /* copro_fifo */;
    r8 = (uintptr_t)i960_vaddr_ptr(0x16002c2c);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    g4 = *(u32 *)(fp + 0x70);
    g5 = *(u32 *)(fp + 0x74);
    g6 = *(u32 *)(fp + 0x78);
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g6); /* copro_fifo */;
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    *(u32 *)(fp + 0x80) = (u32)g4;
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    *(u32 *)(fp + 0x84) = (u32)g4;
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    r9 = (uintptr_t)i960_vaddr_ptr(0x10802121);
    i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
    *(u32 *)(fp + 0x88) = (u32)g4;
    g2 = *(u32 *)pair_obj;
    g4 = *(u32 *)(fp + 0x80);
    g2 = i960_f64_to_u32((i960_u32_to_f64(g2)) - (i960_u32_to_f64(g4)));
    *(u32 *)(fp + 0x40) = (u32)g2;
    g0 = *(u32 *)(pair_obj + 0x4);
    g4 = *(u32 *)(fp + 0x84);
    g0 = i960_f64_to_u32((i960_u32_to_f64(g0)) - (i960_u32_to_f64(g4)));
    g4 = *(u32 *)(fp + 0x88);
    *(u32 *)(fp + 0x44) = (u32)g0;
    g7 = *(u32 *)(pair_obj + 0x8);
    g6 = i960_ld_u32(I960_WORKRAM, 0x20a800, 0);
    g7 = i960_f64_to_u32((i960_u32_to_f64(g7)) - (i960_u32_to_f64(g4)));
    g4 = i960_ld_u32(I960_WORKRAM, 0x20a80c, 0);
    *(u32 *)(fp + 0x48) = (u32)g7;
    g5 = *(u32 *)(pair_obj + 0x20);
    g6 = g6 + (u32)(g6 << 3);
    g6 = g4 + (u32)(g6 << 2); /* guest row @ 0x20a80c */
    g4 = i960_ld_u32(I960_WORKRAM, (u32)g6, 0x18);
    g5 = g5 ^ (1u << 31);
    g4 = g4 + (u32)(g4 << 2);
    g4 = i960_ld_u32(I960_WORKRAM, 0x5acbe8, (u32)(g4 << 2));
    g5 = i960_f64_to_u32((i960_u32_to_f64(g4)) + (i960_u32_to_f64(g5)));
    r8 = (uintptr_t)i960_vaddr_ptr(0x15802b2b);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
    g5 = *(u32 *)(pair_obj + 0x18);
    g4 = i960_ld_u32(I960_WORKRAM, (u32)g6, 0x18);
    g5 = g5 ^ (1u << 31);
    g4 = g4 + (u32)(g4 << 2);
    g4 = i960_ld_u32(I960_WORKRAM, 0x5acbe0, (u32)(g4 << 2));
    g5 = i960_f64_to_u32((i960_u32_to_f64(g4)) + (i960_u32_to_f64(g5)));
    r9 = (uintptr_t)i960_vaddr_ptr(0x14802929);
    i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
    g5 = *(u32 *)(pair_obj + 0x1c);
    g4 = i960_ld_u32(I960_WORKRAM, (u32)g6, 0x18);
    g5 = g5 ^ (1u << 31);
    g4 = g4 + (u32)(g4 << 2);
    g4 = i960_ld_u32(I960_WORKRAM, 0x5acbe4, (u32)(g4 << 2));
    g5 = i960_f64_to_u32((i960_u32_to_f64(g4)) + (i960_u32_to_f64(g5)));
    r8 = (uintptr_t)i960_vaddr_ptr(0x15002a2a);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    g2 = g2 ^ (1u << 31);
    g0 = g0 ^ (1u << 31);
    g7 = g7 ^ (1u << 31);
    i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
    r9 = (uintptr_t)i960_vaddr_ptr(0x13802727);
    i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g2); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g0); /* copro_fifo */;

    L_000128a4:
        i960_mmio_write_u32(0x884000, (u32)g7); /* copro_fifo */;

    L_000128ac:
        {
            u64 tmp = *(u64 *)(fp + 0x40);
            r8 = (u32)tmp;
            r9 = (u32)(tmp >> 32);
        }
        g4 = *(u32 *)(fp + 0x48);
        i960_st_u32(I960_WORKRAM, 0x20220c, 0, (u32)r8);
        i960_st_u32(I960_WORKRAM, 0x20220c, 4, (u32)r9);
        i960_st_u32(I960_WORKRAM, 0x202214, 0, (u32)g4);
    /*
     * After focus store @ 0x128B8 — latch TGP view + workram look-at / seeds.
     * Log camera-script mode byte (disasm r6 from scene_row+0x14) so transitions
     * can be tied to the 0x11C20 jump table, not guessed from visuals.
     */
    {
        static unsigned s_mode_log;

        if (s_mode_log < 8u || (s_mode_log % 60u) == 0u) {
            lift_log(
                    "lift: script_finish cam_mode=%u (>11=identity-only) "
                    "obj=%p\n",
                    (unsigned)(r6 & 0xffu), (void *)arg0_p);
            s_mode_log++;
        }
    }
    model2_hw_latch_view_matrix();
        /* Object at g0/r4 is untouched — scene/delta live in this frame's 0x40(fp). */
        {
            static unsigned s_frustum_log;
            u32 cam_mode = (u32)(r6 & 0xffu);

            if ((cam_mode == 4u || cam_mode == 7u)
                && (s_frustum_log < 8u || (s_frustum_log % 90u) == 0u)
                && arg0_p) {
                float ox = (float)i960_u32_to_f64(arg0_p[0]);
                float oy = (float)i960_u32_to_f64(arg0_p[1]);
                float oz = (float)i960_u32_to_f64(arg0_p[2]);
                float eye[3], foc[3], pitch, sa, sb;

                if (model2_hw_view_camera(eye, foc, &pitch, &sa, &sb)) {
                    float wx = ox - eye[0], wy = oy - eye[1], wz = oz - eye[2];
                    float cx = wx, cy = wy, cz = wz;
                    float bearing_deg;
                    float R[12];

                    if (model2_hw_view_matrix(R)) {
                        cx = R[0] * wx + R[3] * wy + R[6] * wz;
                        cy = R[1] * wx + R[4] * wy + R[7] * wz;
                        cz = R[2] * wx + R[5] * wy + R[8] * wz;
                    }
                    bearing_deg =
                        (float)(atan2((double)cx, (double)cz) * (180.0 / 3.141592653589793));

                    lift_log(
                            "lift: cam%d obj_world=(%.3g,%.3g,%.3g) "
                            "obj_cam=(%.3g,%.3g,%.3g) bearing_xz=%.1fdeg "
                            "ang=(%.3g,%.3g,%.3g)\n",
                            (unsigned)cam_mode, ox, oy, oz, cx, cy, cz, bearing_deg,
                            (float)i960_u32_to_f64(arg0_p[0x18 / 4]),
                            (float)i960_u32_to_f64(arg0_p[0x1c / 4]),
                            (float)i960_u32_to_f64(arg0_p[0x20 / 4]));
                }
                s_frustum_log++;
            }
        }
        if ((uintptr_t)arg0_p != pair_obj)
            goto L_00012904;
        g5 = (uintptr_t)arg0_p + 0x51;
        g4 = *(unsigned char *)g5;
        r9 = 0 - 4;
        g4 = r9 & g4;
        *(unsigned char *)g5 = (unsigned char)g4;
        r8 = 1;
        g0 = (uintptr_t)arg0_p;
        i960_st_u32(I960_WORKRAM, 0x213978, 0, (u32)r8);
        g1 = 0x3f800000u;
        /*
         * Disasm: st r4, 0x213980 — guest object VA. Attract call sites pass
         * host fp-shadow pointers; storing (u32)host pollutes the race cam
         * slot (desert @ 0x1FA20 / race_frame object_pen). Only publish when
         * the truncated value maps as guest RAM (CRX/workram).
         */
        {
            u32 obj_va = (u32)(uintptr_t)arg0_p;

            if (model2_ram_mut(obj_va) == (u8 *)arg0_p)
                i960_st_u32(I960_WORKRAM, 0x213980, 0, obj_va);
            else
                i960_st_u32(I960_WORKRAM, 0x213980, 0, 0u);
        }
        goto L_00012a08;

    L_00012904:
        if (pair_obj != 0)
            goto L_00012968;
        g5 = (uintptr_t)arg0_p + 0x51;
        g4 = *(unsigned char *)g5;
        r9 = 0 - 4;
        g4 = r9 & g4;
        g4 = g4 | 1;
        *(unsigned char *)g5 = (unsigned char)g4;
        r8 = 1;
        i960_st_u32(I960_WORKRAM, 0x213978, 0, (u32)r8);
        {
            u32 obj_va = (u32)(uintptr_t)arg0_p;

            if (model2_ram_mut(obj_va) == (u8 *)arg0_p)
                i960_st_u32(I960_WORKRAM, 0x213980, 0, obj_va);
            else
                i960_st_u32(I960_WORKRAM, 0x213980, 0, 0u);
        }
        g5 = (uintptr_t)arg0_p + 0x50;
        g4 = *(unsigned char *)g5;
        g0 = (uintptr_t)arg0_p;
        g1 = 0x3f800000u;
        r9 = 0 - 16;
        g4 = r9 & g4;
        g4 = g4 | 1;
        g2 = 0x1869f;
        *(unsigned char *)g5 = (unsigned char)g4;
        geo_attract_object_pen((void *)(uintptr_t)g0, g1, g2);
    goto L_00012a14;

    L_00012968:
        g4 = *(unsigned char *)((uintptr_t)arg0_p + 0x50);
        g4 = g4 & 15;
        if ((unsigned char)g4 != 1)
            goto L_0001299c;
        g5 = (uintptr_t)arg0_p + 0x51;
        g4 = *(unsigned char *)g5;
        r8 = 0 - 4;
        g4 = r8 & g4;
        g4 = g4 | 1;
        *(unsigned char *)g5 = (unsigned char)g4;
        g5 = pair_obj + 0x51;
        g4 = *(unsigned char *)g5;
        g4 = g4 & r8;
        goto L_000129c0;

    L_0001299c:
        g5 = (uintptr_t)arg0_p + 0x51;
        g4 = *(unsigned char *)g5;
        r9 = 0 - 4;
        g4 = r9 & g4;
        *(unsigned char *)g5 = (unsigned char)g4;
        g5 = pair_obj + 0x51;
        g4 = *(unsigned char *)g5;
        g4 = g4 & r9;
        g4 = g4 | 1;

    L_000129c0:
        *(unsigned char *)g5 = (unsigned char)g4;
        /*
         * Disasm: st r4,0x213980 / st r5,0x213984 — guest object VAs only.
         * See first store site @ 0x128F8.
         */
        {
            u32 obj_va = (u32)(uintptr_t)arg0_p;
            u32 pair_va = (u32)pair_obj;

            if (model2_ram_mut(obj_va) == (u8 *)arg0_p)
                i960_st_u32(I960_WORKRAM, 0x213980, 0, obj_va);
            else
                i960_st_u32(I960_WORKRAM, 0x213980, 0, 0u);
            if (pair_obj != 0 && model2_ram_mut(pair_va) == (u8 *)pair_obj)
                i960_st_u32(I960_WORKRAM, 0x213984, 0, pair_va);
            else
                i960_st_u32(I960_WORKRAM, 0x213984, 0, 0u);
        }
        r8 = 2;
        i960_st_u32(I960_WORKRAM, 0x213978, 0, (u32)r8);
        if ((unsigned char)r6 == 0)
            goto L_000129fc;
        g0 = (uintptr_t)arg0_p;
        g1 = 0x3f800000u;
        g2 = 0x1869f;
        geo_attract_object_pen((void *)(uintptr_t)g0, g1, g2);

    L_000129fc:
        g0 = pair_obj;
        g1 = 0x3f800000u;

    L_00012a08:
        g2 = 0x1869f;
        geo_attract_object_pen((void *)(uintptr_t)g0, g1, g2);

    L_00012a14:
        geo_attract_copro_submit(g0, g1, g2);
    g0 = *(unsigned short *)((uintptr_t)arg0_p + 0x54);
    g1 = 0 - 30;
    g2 = 30;
    geo_attract_record_index(g0, g1, g2);
    g4 = i960_ld_u32(I960_WORKRAM, 0x2020a4, 0);
    /* Disasm: cmpibe 0,g4 — full-word zero test. */
    if (g4 == 0)
        goto L_00012a48;
    g4 = i960_ld_u32(I960_WORKRAM, 0x214354, 0);
    g6 = i960_ld_u32(I960_WORKRAM, 0x5dc9c0, (u32)(g4 << 2));
    goto L_00012a58;

    L_00012a48:
        g4 = i960_ld_u32(I960_WORKRAM, 0x214354, 0);
        g6 = i960_ld_u32(I960_WORKRAM, 0x5dc9a0, (u32)(g4 << 2));

    L_00012a58:
        g4 = *(unsigned short *)((uintptr_t)arg0_p + 0x54);
        g4 = (uintptr_t)((i32)g4 / (i32)10);
        g5 = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
        g4 = g4 << 16;
        g4 = (uintptr_t)((i32)g4 >> 15);
        /* @0x12A70: ldos (g4)[g6] — signed halfword budget base. */
        {
            u32 half = i960_ld_u16(I960_WORKRAM, (u32)g6, (u32)g4);
            /* ldos — sign-extend halfword into budget base. */
            g2 = (half & 0x8000u) ? (half | 0xffff0000u) : half;
        }
        g0 = i960_ld_u32(I960_WORKRAM, 0x214354, 0);
        g1 = fp + 0x40;
        /* @0x12A80: lda 0xc8(g2)[g5] → budget = g2 + 0xc8 + bank_ctr. */
        g2 = g2 + 0xc8 + (u32)g5;
        geo_attract_course_carousel_draw(g0, (void *)(uintptr_t)g1, g2);
    fp = fp_save;
}
