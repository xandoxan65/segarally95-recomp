/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_037870_e00.asm */
// @rom 0x37870 +0xe00 geo_view_frame_update

#include "i960_lift.h"
#include "i960_fp.h"
#include "model2_rom.h"
#include "i960_mem.h"

#include <math.h>
#include <string.h>

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret  callee r11,r15,r3,r9 */
/* abi: void * arg0=g0, void * arg1=g1, void * arg2=g2 → void */

/* pointers: fp=u64 *, g0=u32 *, g1=void *, g10=void *, g11=u32, g12=void *, g13=void *, g2=void *, g4=u64 *, g5=u32, g6=u32 *, g7=u32 *, g8=u32, r15=u32 *, r3=u32 *, r4=u32, r9=u32 *, sp=u32 * */

#include "lift_syms.h"

typedef struct {
    uintptr_t v[16];
    uintptr_t g14v;
} i960_local_regs;

static i960_local_regs save_local_regs(void)
{
    i960_local_regs s = {{
        r0, r1, r2, r3, r4, r5, r6, r7,
        r8, r9, r10, r11, r12, r13, r14, r15,
    }, g14};

    return s;
}

static void restore_local_regs(i960_local_regs s)
{
    r0 = s.v[0];
    r1 = s.v[1];
    r2 = s.v[2];
    r3 = s.v[3];
    r4 = s.v[4];
    r5 = s.v[5];
    r6 = s.v[6];
    r7 = s.v[7];
    r8 = s.v[8];
    r9 = s.v[9];
    r10 = s.v[10];
    r11 = s.v[11];
    r12 = s.v[12];
    r13 = s.v[13];
    r14 = s.v[14];
    r15 = s.v[15];
    /* i960 call/ret restores caller g14 (0 from scene_frame). */
    g14 = s.g14v;
}

void geo_view_frame_update(void * arg0, void * arg1, void * arg2)
{
    /* arg0 = cam/node guest VA; arg1/arg2 may be host private-frame outs. */
    u32 cam = (u32)(uintptr_t)arg0;
    uintptr_t a1 = (uintptr_t)arg1;
    uintptr_t a2 = (uintptr_t)arg2;
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    /*
     * Disasm: lda 0x50(sp),sp only — keeps caller's fp for 0x40(fp) temps on
     * hardware. Host scene_frame already uses fp+0x40..0x6c for vec_push;
     * sharing that range stomped those temps. Give this leaf a private frame
     * for 0x40(fp) scratch; a1/a2 stay the caller's output pointers (r3/r15).
     */
    u8 frame[0x80];
    /*
     * Hardware: lda 0x50(sp),sp then st/stl at -0x20/-0x18/-0x10(sp) while
     * fp stays the caller's. Body scratch is 0x40(fp) on that same caller frame.
     * Host private fp puts sp-0x10 at fp+0x40 — the body then stomps the saved
     * g12 (node VA). scene_frame's post-call ld 0x54(g12) then divides by 0/junk
     * → rate=inf → pitch stuck. Keep callee-saves in C locals instead.
     */
    u32 save_g8 = (u32)g8;
    u32 save_g10 = (u32)g10;
    u32 save_g11 = (u32)g11;
    u32 save_g12 = (u32)g12;
    /*
     * i960 `call` switches to a fresh local r0-r15 register frame.  These
     * caller locals therefore survive every helper call in this function:
     *   r10 = selected 0x5d67a0 dynamics row
     *   r11 = correction accumulator seeded from incoming g3
     *
     * Lifted helpers share the host r10/r11 globals, so keeping either live
     * there lets a callee corrupt the camera-angle force path.
     */
    u32 selected_row;
    u32 correction_accum = (u32)g3;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    sp = (uintptr_t)frame + 0x50u;
    r9 = cam;
    r3 = a1;
    r15 = a2;
    r10 = 9;
    /*
     * scene_frame zeros g14 at entry; i960 call/ret keeps that across this
     * leaf. Host helpers otherwise leave a link in g14, and the TGP 0x42/0x48
     * `st g14` payload (slot index + Y=0) plus 21420c/254 clears become rates.
     */
    g14 = 0;
    g4 = i960_ld_u32(I960_WORKRAM, 0x214208, 0);

    L_000378a8:
        if ((g4 >> r10) & 1)
            goto L_000378b4;
        r10 = r10 - 1;
        if (0 < (signed char)r10)
            goto L_000378a8;

    L_000378b4:
        selected_row = (u32)r10;
        /* Row ptr is a guest VA (ROM mirror); use i960_ld — not host *(u64*). */
        g4 = i960_ld_u32(I960_WORKRAM, 0x5d67a0, selected_row << 2);
        {
            u32 qi;

            for (qi = 0; qi < 4u; qi++) {
                i960_st_u32(I960_WORKRAM, 0x2141c0, qi * 4u,
                            i960_ld_u32(I960_WORKRAM, (u32)g4, qi * 4u));
                i960_st_u32(I960_WORKRAM, 0x2141d0, qi * 4u,
                            i960_ld_u32(I960_WORKRAM, (u32)g4 + 0x10u, qi * 4u));
                i960_st_u32(I960_WORKRAM, 0x2141e0, qi * 4u,
                            i960_ld_u32(I960_WORKRAM, (u32)g4 + 0x20u, qi * 4u));
            }
            g4 = i960_ld_u32(I960_WORKRAM, (u32)g4, 0x30u);
        }
        /* @0x378E4: lda 0x214254,g1 — workram scratch, not CRX. */
        g1 = 0x214254u;
        i960_st_u32(I960_WORKRAM, 0x2141f0, 0, (u32)g4);
        /*
         * Hardware r9 is a caller-local register and survives this call.
         * The lifted matrix_prep uses the shared host r9 global in its own
         * emulated local frame, so retain the entry node in private C state.
         */
        {
            i960_local_regs caller_r = save_local_regs();

            g0 = geo_view_matrix_prep(cam, (u32)g1, (u32)g2);
            restore_local_regs(caller_r);
        }
    i960_st_u32(I960_WORKRAM, 0x214258, 0, (u32)g0);
    g5 = i960_ld_u32(I960_WORKRAM, cam, 0x64);
    g4 = i960_ld_u32(I960_WORKRAM, cam, 0x68);
    r5 = i960_f64_to_u32((i960_u32_to_f64(g4)) + (i960_u32_to_f64(g5)));
    fp0 = i960_u32_to_f64(r5);
    g5 = i960_ld_u32(I960_WORKRAM, cam, 0x6c);
    g4 = i960_ld_u32(I960_WORKRAM, cam, 0x70);
    r12 = i960_ld_u32(I960_WORKRAM, 0x21425c, 0);
    r7 = i960_f64_to_u32((i960_u32_to_f64(g4)) + (i960_u32_to_f64(g5)));
    if (fp0 != 0.0)
        goto L_00037934;
    r14 = 0;
    goto L_00037978;

    L_00037934:
        g11 = i960_ld_u32(I960_WORKRAM, cam, 0x68);
        g4 = i960_ld_u32(I960_WORKRAM, cam, 0x64);
        fp0 = i960_u32_to_f64(g11);
        g4 = i960_f64_to_u32((fp0) - (i960_u32_to_f64(g4)));
        fp0 = i960_u32_to_f64(g4);
        fp0 = i960_u32_to_f64(g4);
        fp1 = 1.1;
        i960_rifl_write(&g10, &g11, (fp1) * (fp0));
        fp1 = i960_u32_to_f64(r5);
        fp0 = i960_rifl_read(g10, g11);
        i960_rifl_write(&g12, &g13, (fp0) / (fp1));
        fp0 = i960_rifl_read(g12, g13);
        r14 = i960_f64_to_u32(fp0);

    L_00037978:
        fp0 = i960_u32_to_f64(r7);
        if (fp0 != 0.0)
            goto L_0003798c;
        r13 = 0;
        goto L_000379d0;

    L_0003798c:
        g13 = i960_ld_u32(I960_WORKRAM, cam, 0x70);
        g4 = i960_ld_u32(I960_WORKRAM, cam, 0x6c);
        fp0 = i960_u32_to_f64(g13);
        g4 = i960_f64_to_u32((fp0) - (i960_u32_to_f64(g4)));
        fp0 = i960_u32_to_f64(g4);
        fp0 = i960_u32_to_f64(g4);
        fp1 = 1.1;
        i960_rifl_write(&g12, &g13, (fp1) * (fp0));
        fp1 = i960_u32_to_f64(r7);
        fp0 = i960_rifl_read(g12, g13);
        i960_rifl_write(&g10, &g11, (fp0) / (fp1));
        fp0 = i960_rifl_read(g10, g11);
        r13 = i960_f64_to_u32(fp0);

    L_000379d0:
        g11 = i960_f64_to_u32((i960_u32_to_f64(r7)) * (i960_u32_to_f64(r5)));
        fp0 = i960_u32_to_f64(g11);
        fp0 = i960_u32_to_f64(g11);
        if (fp0 != 0.0)
            goto L_00037a08;
        g12 = 1;
        i960_st_u32(I960_WORKRAM, 0x214260, 0, (u32)g12);
        {
            i960_local_regs caller_r = save_local_regs();

            g0 = geo_view_input_gate(0, (u32)g1, (u32)g2);
            restore_local_regs(caller_r);
        }
    r8 = 0;
    r6 = r8;
    r4 = g0;
    goto L_00037a58;

    L_00037a08:
        g4 = i960_ld_u32(I960_WORKRAM, 0x2020b4, 0);
        r4 = 0x3e4ccccdu;
        if (g4 == 0) {
            r4 = i960_ld_u32(I960_WORKRAM, 0x2141c8, 0);
        }
    i960_st_u32(I960_WORKRAM, 0x214260, 0, (u32)g14);
    {
        i960_local_regs caller_r = save_local_regs();

        g0 = geo_view_input_gate(1, (u32)g1, (u32)g2);
        restore_local_regs(caller_r);
    }
    g4 = i960_ld_u32(I960_WORKRAM, 0x21420c, 0);
    g4 = g4 ^ (1u << 31);
    g5 = i960_ld_u32(I960_WORKRAM, 0x214258, 0);
    r6 = i960_f64_to_u32((i960_u32_to_f64(g5)) * (i960_u32_to_f64(r4)));
    r4 = i960_f64_to_u32((i960_u32_to_f64(g0)) * (i960_u32_to_f64(g4)));
    r8 = i960_f64_to_u32((i960_u32_to_f64(g5)) - (i960_u32_to_f64(r6)));

    L_00037a58:
        g13 = i960_ld_u32(I960_WORKRAM, 0x20b0b4, 0);
        g5 = i960_f64_to_u32((i960_u32_to_f64(r7)) + (i960_u32_to_f64(r5)));
        fp0 = i960_u32_to_f64(g13);
        fp0 = i960_u32_to_f64(g13);
        fp1 = 0.3;
        i960_rifl_write(&g12, &g13, (fp1) * (fp0));
        fp1 = i960_u32_to_f64(g5);
        fp0 = i960_rifl_read(g12, g13);
        i960_rifl_write(&g10, &g11, (1.0) + (fp0));
        fp0 = i960_rifl_read(g10, g11);
        g8 = i960_f64_to_u32(fp0);
        if (fp1 != 0.0)
            goto L_00037aac;
        i960_st_u32(I960_WORKRAM, 0x21425c, 0, (u32)g14);
        goto L_00037b54;

    L_00037aac:
        g4 = i960_f64_to_u32((i960_u32_to_f64(r5)) - (i960_u32_to_f64(r7)));
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) / (i960_u32_to_f64(g5)));
        g5 = i960_f64_to_u32((i960_u32_to_f64(g4)) - (i960_u32_to_f64(r12)));
        fp0 = i960_u32_to_f64(g5);
        i960_st_u32(I960_WORKRAM, 0x21425c, 0, (u32)g4);
        if (fp0 <= 0.0)
            goto L_00037ad8;
        g4 = i960_ld_u32(I960_WORKRAM, 0x2141e4, 0);
        goto L_00037ae0;

    L_00037ad8:
        g4 = i960_ld_u32(I960_WORKRAM, 0x2141e8, 0);

    L_00037ae0:
        g5 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(g5)));
        g0 = i960_ld_u32(I960_WORKRAM, 0x21425c, 0);
        g0 = i960_f64_to_u32((i960_u32_to_f64(g0)) - (i960_u32_to_f64(g5)));
        i960_st_u32(I960_WORKRAM, 0x21425c, 0, (u32)g0);
        {
            i960_local_regs caller_r = save_local_regs();

            g0 = geo_view_sin_mix((u32)g0, (u32)g1, (u32)g2);
            restore_local_regs(caller_r);
        }
    fp2 = i960_u32_to_f64(g0);
    g11 = i960_ld_u32(I960_WORKRAM, 0x2141c0, 0);
    i960_rifl_write(&g12, &g13, (1.0) + (fp2));
    fp1 = i960_u32_to_f64(g11);
    i960_rifl_write(&g10, &g11, (1.0) - (fp2));
    fp3 = i960_rifl_read(g12, g13);
    g13 = i960_ld_u32(I960_WORKRAM, 0x2141c4, 0);
    fp2 = i960_rifl_read(g10, g11);
    g11 = i960_f64_to_u32(fp1);
    fp1 = i960_u32_to_f64(g11);
    fp0 = i960_u32_to_f64(g13);
    fp0 = i960_u32_to_f64(g13);
    i960_rifl_write(&g12, &g13, (fp3) * (fp1));
    i960_rifl_write(&g10, &g11, (fp2) * (fp0));
    i960_st_u32(I960_WORKRAM, 0x21425c, 0, (u32)g0);
    fp1 = i960_rifl_read(g12, g13);
    fp0 = i960_rifl_read(g10, g11);
    r5 = i960_f64_to_u32(fp1);
    r7 = i960_f64_to_u32(fp0);

    L_00037b54:
        g5 = i960_ld_u32(I960_WORKRAM, 0x21420c, 0);
        if (i960_u32_to_f64(g5) < i960_u32_to_f64(+0.0)) {
            g5 = g5 ^ (1u << 31);
        }
    g4 = i960_ld_u32(I960_WORKRAM, 0x5d6690, 0);
    if (i960_u32_to_f64(g5) >= i960_u32_to_f64(g4))
        goto L_00037b84;
    i960_st_u32(I960_WORKRAM, 0x21420c, 0, (u32)g14);
    goto L_00037bd8;

    L_00037b84:
        g11 = i960_ld_u32(I960_WORKRAM, 0x21420c, 0);
        g12 = i960_ld_u32(I960_WORKRAM, 0x5d6690, 0);
        fp1 = i960_u32_to_f64(g11);
        fp0 = i960_u32_to_f64(g12);
        fp0 = i960_u32_to_f64(g12);
        fp1 = i960_u32_to_f64(g11);
        /* Disasm @ 0x37B98: cmpr fp1,+0.0 — fp1 is already a host double. */
        if (fp1 >= 0.0)
            goto L_00037bc4;
        i960_rifl_write(&g10, &g11, fp0);
        g11 = g11 ^ (1u << 31);
        fp0 = i960_rifl_read(g10, g11);
        i960_rifl_write(&g12, &g13, (fp1) - (fp0));
        fp0 = i960_rifl_read(g12, g13);
        goto L_00037bcc;

    L_00037bc4:
        i960_rifl_write(&g10, &g11, (fp1) - (fp0));
        fp0 = i960_rifl_read(g10, g11);

    L_00037bcc:
        g4 = i960_f64_to_u32(fp0);
        i960_st_u32(I960_WORKRAM, 0x21420c, 0, (u32)g4);

    L_00037bd8:
        g11 = i960_ld_u32(I960_WORKRAM, 0x21420c, 0);
        g12 = i960_ld_u32(I960_WORKRAM, 0x214124, 0);
        fp2 = i960_u32_to_f64(g11);
        fp2 = i960_u32_to_f64(g11);
        fp1 = (2.0 * 3.141592653589793);
        g10 = 0;
        g11 = 0x408f4000u;
        fp0 = i960_u32_to_f64(g12);
        fp0 = i960_u32_to_f64(g12);
        i960_rifl_write(&g12, &g13, (fp1) * (fp2));
        fp1 = i960_rifl_read(g10, g11);
        fp2 = i960_rifl_read(g12, g13);
        i960_rifl_write(&g12, &g13, (fp1) * (fp0));
        fp3 = i960_u32_to_f64(r5);
        fp0 = i960_rifl_read(g12, g13);
        i960_rifl_write(&g12, &g13, (fp3) * (fp0));
        g10 = 0;
        g11 = 0x40080000u;
        fp1 = i960_rifl_read(g10, g11);
        i960_rifl_write(&g4, &g5, (fp2) / (fp1));
        fp1 = i960_u32_to_f64(g8);
        fp0 = i960_rifl_read(g12, g13);
        i960_rifl_write(&g10, &g11, (fp1) * (fp0));
        fp0 = i960_rifl_read(g10, g11);
        g0 = i960_f64_to_u32(fp0);
        if (i960_rifl_read(g4, g5) >= 0.0)
            goto L_00037c6c;
        g5 = g5 ^ (1u << 31);
        goto L_00037cac;

    L_00037c6c:
        g11 = i960_ld_u32(I960_WORKRAM, 0x21420c, 0);
        fp0 = i960_u32_to_f64(g11);
        fp0 = i960_u32_to_f64(g11);
        fp1 = (2.0 * 3.141592653589793);
        g12 = 0;
        g13 = 0x40080000u;
        i960_rifl_write(&g10, &g11, (fp1) * (fp0));
        fp1 = i960_rifl_read(g12, g13);
        fp0 = i960_rifl_read(g10, g11);
        i960_rifl_write(&g4, &g5, (fp0) / (fp1));

    L_00037cac:
        fp3 = 1.5707963267948966;
        if (i960_rifl_read(g4, g5) <= fp3)
            goto L_00037cd4;
        g4 = 0x3fc90fdbu;
        goto L_00037d70;

    L_00037cd4:
        g12 = i960_ld_u32(I960_WORKRAM, 0x21420c, 0);
        fp0 = i960_u32_to_f64(g12);
        fp0 = i960_u32_to_f64(g12);
        fp1 = (2.0 * 3.141592653589793);
        g10 = 0;
        g11 = 0x40080000u;
        i960_rifl_write(&g12, &g13, (fp1) * (fp0));
        fp1 = i960_rifl_read(g10, g11);
        fp0 = i960_rifl_read(g12, g13);
        i960_rifl_write(&g4, &g5, (fp0) / (fp1));
        if (i960_rifl_read(g4, g5) >= 0.0)
            goto L_00037d28;
        g5 = g5 ^ (1u << 31);
        fp0 = i960_rifl_read(g4, g5);
        goto L_00037d6c;

    L_00037d28:
        g11 = i960_ld_u32(I960_WORKRAM, 0x21420c, 0);
        fp0 = i960_u32_to_f64(g11);
        fp0 = i960_u32_to_f64(g11);
        fp1 = (2.0 * 3.141592653589793);
        g12 = 0;
        g13 = 0x40080000u;
        i960_rifl_write(&g10, &g11, (fp1) * (fp0));
        fp1 = i960_rifl_read(g12, g13);
        fp0 = i960_rifl_read(g10, g11);
        i960_rifl_write(&g10, &g11, (fp0) / (fp1));
        fp0 = i960_rifl_read(g10, g11);

    L_00037d6c:
        g4 = i960_f64_to_u32(fp0);

    L_00037d70:
        g11 = 0xa801515u;
        i960_mmio_write_u32(0x884000, (u32)g11); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        r5 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        g5 = i960_f64_to_u32((i960_u32_to_f64(r5)) * (i960_u32_to_f64(r4)));
        g5 = i960_f64_to_u32((i960_u32_to_f64(g0)) * (i960_u32_to_f64(g5)));
        g4 = i960_ld_u32(I960_WORKRAM, 0x21420c, 0);
        fp1 = i960_u32_to_f64(g0);
        fp0 = i960_u32_to_f64(r5);
        correction_accum =
            i960_f64_to_u32(i960_u32_to_f64(g5)
                            + i960_u32_to_f64(correction_accum));
        if (i960_u32_to_f64(g4) >= i960_u32_to_f64(+0.0)) {
            i960_rifl_write(&g10, &g11, fp0);
            g11 = g11 ^ (1u << 31);
            fp0 = i960_rifl_read(g10, g11);
        }
    i960_rifl_write(&g12, &g13, (fp1) * (fp0));
    fp0 = i960_rifl_read(g12, g13);
    r5 = i960_f64_to_u32(fp0);
    g4 = i960_f64_to_u32((i960_u32_to_f64(r5)) * (i960_u32_to_f64(r5)));
    g5 = i960_f64_to_u32((i960_u32_to_f64(r6)) * (i960_u32_to_f64(r6)));
    g13 = i960_ld_u32(I960_WORKRAM, 0x214254, 0);
    fp1 = i960_u32_to_f64(g13);
    fp1 = i960_u32_to_f64(g13);
    g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) + (i960_u32_to_f64(g4)));
    i960_rifl_write(&g10, &g11, (1.0) + (fp1));
    g5 = i960_f64_to_u32(sqrt(i960_u32_to_f64(g4)));
    fp0 = i960_u32_to_f64(g5);
    fp1 = i960_rifl_read(g10, g11);
    i960_rifl_write(&g12, &g13, (fp1) * (fp0));
    fp0 = i960_rifl_read(g12, g13);
    g5 = i960_f64_to_u32(fp0);
    if (i960_u32_to_f64(g5) <= i960_u32_to_f64(g0))
        goto L_00037e48;
    g0 = i960_f64_to_u32((i960_u32_to_f64(g0)) / (i960_u32_to_f64(g5)));
    {
        i960_local_regs caller_r = save_local_regs();

        g0 = geo_view_sqrt_mix_a((u32)g0, (u32)g1, (u32)g2);
        restore_local_regs(caller_r);
    }
    fp0 = i960_u32_to_f64(g0);
    fp1 = 1.1;
    i960_rifl_write(&g12, &g13, (fp1) * (fp0));
    fp1 = i960_u32_to_f64(r5);
    fp0 = i960_rifl_read(g12, g13);
    i960_rifl_write(&g10, &g11, (fp0) * (fp1));
    r6 = i960_f64_to_u32((i960_u32_to_f64(g0)) * (i960_u32_to_f64(r6)));
    fp1 = i960_rifl_read(g10, g11);
    r5 = i960_f64_to_u32(fp1);

    L_00037e48:
        g5 = i960_ld_u32(I960_WORKRAM, 0x21420c, 0);
        g6 = 0x3ca3d70au;
        if (i960_u32_to_f64(g5) >= i960_u32_to_f64(+0.0))
            goto L_00037e70;
        g5 = g5 ^ (1u << 31);
        g4 = i960_ld_u32(I960_WORKRAM, 0x214210, 0);
        goto L_00037e80;

    L_00037e70:
        g5 = i960_ld_u32(I960_WORKRAM, 0x214210, 0);
        g4 = i960_ld_u32(I960_WORKRAM, 0x21420c, 0);

    L_00037e80:
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(g5)));
        if (i960_u32_to_f64(g4) < i960_u32_to_f64(g6)) {
            g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) / (i960_u32_to_f64(g6)));
            r5 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(r5)));
        }
    g11 = i960_ld_u32(I960_WORKRAM, 0x20b0b4, 0);
    fp0 = i960_u32_to_f64(g11);
    fp0 = i960_u32_to_f64(g11);
    fp3 = 0.023148148148148147;
    if (fp0 >= fp3)
        goto L_00037ec8;
    {
        i960_local_regs caller_r = save_local_regs();

        geo_view_scene_apply(g0, g1, g2);
        restore_local_regs(caller_r);
    }
    goto L_00037edc;

    L_00037ec8:
        g1 = i960_f64_to_u32(i960_u32_to_f64(r5)
                             + i960_u32_to_f64(correction_accum));
        g0 = i960_ld_u32(I960_WORKRAM, cam, 0x74);
        g2 = i960_ld_u32(I960_WORKRAM, 0x21420c, 0);
        {
            i960_local_regs caller_r = save_local_regs();

            geo_view_mode_apply(g0, g1, g2);
            restore_local_regs(caller_r);
        }

    L_00037edc:
        g4 = i960_ld_u32(I960_WORKRAM, 0x214120, 0);
        if ((unsigned char)g4 == 2) {
            i960_st_u8(I960_WORKRAM, 0x202049, 0, (u8)g14);
        }
    fp0 = i960_u32_to_f64(r6);
    if (fp0 < 0.0) {
        r14 = 0;
    }
    g12 = i960_ld_u32(I960_WORKRAM, 0x214124, 0);
    fp1 = i960_u32_to_f64(g12);
    fp1 = i960_u32_to_f64(g12);
    g10 = 0;
    g11 = 0x408f4000u;
    fp0 = i960_rifl_read(g10, g11);
    i960_rifl_write(&g12, &g13, (fp0) * (fp1));
    fp0 = i960_u32_to_f64(r7);
    fp1 = i960_rifl_read(g12, g13);
    i960_rifl_write(&g10, &g11, (fp0) * (fp1));
    fp1 = i960_rifl_read(g10, g11);
    g11 = i960_ld_u32(I960_WORKRAM, 0x214214, 0);
    fp0 = i960_u32_to_f64(g11);
    fp0 = i960_u32_to_f64(g11);
    fp2 = 3.141592653589793;
    i960_rifl_write(&g4, &g5, (fp2) * (fp0));
    fp0 = i960_u32_to_f64(g8);
    i960_rifl_write(&g10, &g11, (fp0) * (fp1));
    fp1 = i960_rifl_read(g10, g11);
    g0 = i960_f64_to_u32(fp1);
    if (i960_rifl_read(g4, g5) < 0.0) {
        g5 = g5 ^ (1u << 31);
    }
    fp3 = 1.5707963267948966;
    if (i960_rifl_read(g4, g5) <= fp3)
        goto L_00037fa0;
    g4 = 0x3fc90fdbu;
    goto L_0003800c;

    L_00037fa0:
        g12 = i960_ld_u32(I960_WORKRAM, 0x214214, 0);
        fp0 = i960_u32_to_f64(g12);
        fp0 = i960_u32_to_f64(g12);
        fp1 = 3.141592653589793;
        i960_rifl_write(&g4, &g5, (fp1) * (fp0));
        if (i960_rifl_read(g4, g5) >= 0.0)
            goto L_00037fdc;
        g5 = g5 ^ (1u << 31);
        fp0 = i960_rifl_read(g4, g5);
        goto L_00038008;

    L_00037fdc:
        g11 = i960_ld_u32(I960_WORKRAM, 0x214214, 0);
        fp0 = i960_u32_to_f64(g11);
        fp0 = i960_u32_to_f64(g11);
        fp1 = 3.141592653589793;
        i960_rifl_write(&g10, &g11, (fp1) * (fp0));
        fp0 = i960_rifl_read(g10, g11);

    L_00038008:
        g4 = i960_f64_to_u32(fp0);

    L_0003800c:
        g11 = 0xa801515u;
        i960_mmio_write_u32(0x884000, (u32)g11); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
        r4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(g0)));
        g5 = i960_f64_to_u32((i960_u32_to_f64(r8)) * (i960_u32_to_f64(r8)));
        g4 = i960_f64_to_u32((i960_u32_to_f64(r4)) * (i960_u32_to_f64(r4)));
        g12 = i960_ld_u32(I960_WORKRAM, 0x214254, 0);
        fp1 = i960_u32_to_f64(g12);
        fp1 = i960_u32_to_f64(g12);
        g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) + (i960_u32_to_f64(g4)));
        i960_rifl_write(&g10, &g11, (1.0) + (fp1));
        g5 = i960_f64_to_u32(sqrt(i960_u32_to_f64(g4)));
        fp0 = i960_u32_to_f64(g5);
        fp1 = i960_rifl_read(g10, g11);
        i960_rifl_write(&g12, &g13, (fp1) * (fp0));
        fp0 = i960_rifl_read(g12, g13);
        g5 = i960_f64_to_u32(fp0);
        if (i960_u32_to_f64(g5) <= i960_u32_to_f64(g0))
            goto L_000380ac;
        g0 = i960_f64_to_u32((i960_u32_to_f64(g0)) / (i960_u32_to_f64(g5)));
        {
            i960_local_regs caller_r = save_local_regs();

            g0 = geo_view_sqrt_mix_b((u32)g0, (u32)g1, (u32)g2);
            restore_local_regs(caller_r);
        }
    fp0 = i960_u32_to_f64(g0);
    fp1 = 1.1;
    i960_rifl_write(&g12, &g13, (fp1) * (fp0));
    fp1 = i960_u32_to_f64(r4);
    fp0 = i960_rifl_read(g12, g13);
    i960_rifl_write(&g10, &g11, (fp0) * (fp1));
    r8 = i960_f64_to_u32((i960_u32_to_f64(g0)) * (i960_u32_to_f64(r8)));
    fp1 = i960_rifl_read(g10, g11);
    r4 = i960_f64_to_u32(fp1);

    L_000380ac:
        g4 = i960_ld_u32(I960_WORKRAM, 0x214214, 0);
        fp0 = i960_u32_to_f64(r4);
        if (i960_u32_to_f64(g4) >= i960_u32_to_f64(+0.0)) {
            i960_rifl_write(&g12, &g13, fp0);
            g13 = g13 ^ (1u << 31);
            fp0 = i960_rifl_read(g12, g13);
        }
    r4 = i960_f64_to_u32(fp0);
    g5 = i960_ld_u32(I960_WORKRAM, 0x214214, 0);
    g6 = 0x3ca3d70au;
    if (i960_u32_to_f64(g5) >= i960_u32_to_f64(+0.0))
        goto L_000380f8;
    g5 = g5 ^ (1u << 31);
    g4 = i960_ld_u32(I960_WORKRAM, 0x214218, 0);
    goto L_00038108;

    L_000380f8:
        g5 = i960_ld_u32(I960_WORKRAM, 0x214218, 0);
        g4 = i960_ld_u32(I960_WORKRAM, 0x214214, 0);

    L_00038108:
        g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(g5)));
        if (i960_u32_to_f64(g4) < i960_u32_to_f64(g6)) {
            g4 = i960_f64_to_u32((i960_u32_to_f64(g4)) / (i960_u32_to_f64(g6)));
            r4 = i960_f64_to_u32((i960_u32_to_f64(g4)) * (i960_u32_to_f64(r4)));
        }
    fp0 = i960_u32_to_f64(r8);
    if (fp0 < 0.0) {
        r13 = 0;
    }
    g0 = i960_f64_to_u32((i960_u32_to_f64(r8)) + (i960_u32_to_f64(r6)));
    g1 = i960_ld_u32(I960_WORKRAM, 0x214258, 0);
    {
        i960_local_regs caller_r = save_local_regs();

        geo_view_pitch_blend((u32)g0, (u32)g1, (u32)g2);
        restore_local_regs(caller_r);
    }
    g13 = i960_f64_to_u32((i960_u32_to_f64(r8)) * (i960_u32_to_f64(r6)));
    fp0 = i960_u32_to_f64(g13);
    fp0 = i960_u32_to_f64(g13);
    if (fp0 == 0.0) {
        i960_st_u32(I960_WORKRAM, 0x214254, 0, (u32)g14);
    }
    g4 = i960_ld_u32(I960_WORKRAM, 0x2020b4, 0);
    if ((unsigned char)g4 == 0)
        goto L_0003824c;
    g10 = i960_ld_u32(I960_WORKRAM, 0x214254, 0);
    fp0 = i960_u32_to_f64(g10);
    fp0 = i960_u32_to_f64(g10);
    fp3 = 0.15;
    if (fp0 >= fp3)
        goto L_00038214;
    g4 = i960_ld_u32(I960_WORKRAM, 0x214200, 0);
    if (i960_u32_to_f64(g4) >= i960_u32_to_f64(+0.0))
        goto L_000381ac;
    g4 = g4 ^ (1u << 31);
    fp0 = i960_u32_to_f64(g4);
    goto L_000381bc;

    L_000381ac:
        g12 = i960_ld_u32(I960_WORKRAM, 0x214200, 0);
        fp0 = i960_u32_to_f64(g12);
        fp0 = i960_u32_to_f64(g12);

    L_000381bc:
        fp3 = 0.2;
        if (fp0 <= fp3)
            goto L_00038214;
        g12 = i960_ld_u32(I960_WORKRAM, 0x214218, 0);
        fp0 = i960_u32_to_f64(g12);
        fp0 = i960_u32_to_f64(g12);
        fp3 = 0.1;
        if (fp0 > fp3) {
            g12 = 0x3e19999au;
            i960_st_u32(I960_WORKRAM, 0x214254, 0, (u32)g12);
        }

    L_00038214:
        g13 = i960_ld_u32(I960_WORKRAM, 0x214254, 0);
        fp0 = i960_u32_to_f64(g13);
        fp0 = i960_u32_to_f64(g13);
        fp3 = 0.15;
        if (fp0 >= fp3)
            goto L_000382e8;
        i960_st_u32(I960_WORKRAM, 0x214254, 0, (u32)g14);
        goto L_000382e8;

    L_0003824c:
        g12 = i960_ld_u32(I960_WORKRAM, 0x214254, 0);
        fp0 = i960_u32_to_f64(g12);
        fp0 = i960_u32_to_f64(g12);
        if (fp0 != 0.0)
            goto L_000382e8;
        g4 = i960_ld_u32(I960_WORKRAM, 0x214200, 0);
        if (i960_u32_to_f64(g4) >= i960_u32_to_f64(+0.0))
            goto L_00038280;
        g4 = g4 ^ (1u << 31);
        fp0 = i960_u32_to_f64(g4);
        goto L_00038290;

    L_00038280:
        g13 = i960_ld_u32(I960_WORKRAM, 0x214200, 0);
        fp0 = i960_u32_to_f64(g13);
        fp0 = i960_u32_to_f64(g13);

    L_00038290:
        fp3 = 0.2;
        if (fp0 <= fp3)
            goto L_000382e8;
        g12 = i960_ld_u32(I960_WORKRAM, 0x214218, 0);
        fp0 = i960_u32_to_f64(g12);
        fp0 = i960_u32_to_f64(g12);
        fp3 = 0.1;
        if (fp0 > fp3) {
            g12 = 0x3c23d70au;
            i960_st_u32(I960_WORKRAM, 0x214254, 0, (u32)g12);
        }

    L_000382e8:
        g0 = i960_ld_u32(I960_WORKRAM, 0x214254, 0);
        g1 = selected_row;
        {
            i960_local_regs caller_r = save_local_regs();

            geo_view_latch_apply((u32)g0, (u32)g1, (u32)g2);
            restore_local_regs(caller_r);
        }
    fp0 = i960_u32_to_f64(r6);
    g13 = 0x10002020u;
    i960_mmio_write_u32(0x884000, (u32)g13); /* copro_fifo */;
    if (fp0 < 0.0) {
        r8 = i960_f64_to_u32((i960_u32_to_f64(r6)) + (i960_u32_to_f64(r8)));
        r6 = 0;
    }
    fp1 = i960_u32_to_f64(r13);
    fp3 = i960_u32_to_f64(r14);
    fp0 = 0.66;
    g10 = 0x851eb852u;
    g11 = 0x3fe451ebu;
    i960_rifl_write(&g12, &g13, (fp0) * (fp1));
    fp0 = i960_rifl_read(g10, g11);
    i960_rifl_write(&g6, &g7, fp3);
    i960_rifl_write(&g6, &g7, (fp0) * (i960_rifl_read(g6, g7)));
    fp0 = i960_u32_to_f64(r8);
    fp1 = i960_rifl_read(g12, g13);
    g12 = 0x21004242u;
    i960_mmio_write_u32(0x884000, (u32)g12); /* copro_fifo */;
    i960_rifl_write(&g12, &g13, (fp0) * (fp1));
    g14 = 0;
    i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)r5); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)r6); /* copro_fifo */;
    g11 = 0x12802525u;
    i960_mmio_write_u32(0x884000, (u32)g11); /* copro_fifo */;
    g4 = i960_ld_u32(I960_WORKRAM, cam, 0x74);
    fp1 = i960_rifl_read(g12, g13);
    g13 = 0x15002a2au;
    i960_mmio_write_u32(0x884000, (u32)g13); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g10 = 0x24004848u;
    i960_mmio_write_u32(0x884000, (u32)g10); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
    g11 = 0x21804343u;
    i960_mmio_write_u32(0x884000, (u32)g11); /* copro_fifo */;
    g12 = i960_f64_to_u32(fp1);
    i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
    fp3 = i960_u32_to_f64(r4);
    fp0 = i960_u32_to_f64(r6);
    g13 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    fp1 = i960_u32_to_f64(g12);
    fp1 = i960_u32_to_f64(g12);
    i960_rifl_write(&g10, &g11, fp3);
    *(u64 *)(fp + 0x60) = ((u64)g11 << 32) | (u32)g10;
    { u64 _p = *(u64 *)(fp + 0x60); g10 = (u32)_p; g11 = (u32)(_p >> 32); }
    i960_rifl_write(&g6, &g7, (fp0) * (i960_rifl_read(g6, g7)));
    fp0 = i960_u32_to_f64(g13);
    i960_rifl_write(&g6, &g7, (fp1) - (i960_rifl_read(g6, g7)));
    fp3 = i960_rifl_read(g10, g11);
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    i960_rifl_write(&g10, &g11, (fp3) - (fp0));
    g5 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    g12 = 0x12802525u;
    fp1 = i960_rifl_read(g6, g7);
    i960_mmio_write_u32(0x884000, (u32)g12); /* copro_fifo */;
    g12 = i960_f64_to_u32(fp1);
    *(u32 *)(fp + 0x40) = (u32)g13;
    *(u64 *)(fp + 0x60) = ((u64)g11 << 32) | (u32)g10;
    fp2 = i960_u32_to_f64(g13);
    *(u32 *)(fp + 0x54) = (u32)g12;
    fp1 = i960_u32_to_f64(g12);
    { u64 _p = *(u64 *)(fp + 0x60); g12 = (u32)_p; g13 = (u32)(_p >> 32); }
    g10 = 0;
    g11 = 0x3ff40000u;
    fp0 = i960_rifl_read(g10, g11);
    fp3 = i960_rifl_read(g12, g13);
    i960_rifl_write(&g12, &g13, (fp0) * (fp3));
    *(u32 *)(fp + 0x44) = (u32)g4;
    *(u32 *)(fp + 0x48) = (u32)g5;
    *(u64 *)(fp + 0x60) = ((u64)g13 << 32) | (u32)g12;
    g4 = i960_ld_u32(I960_WORKRAM, cam, 0x3c);
    g12 = 0x15002a2au;
    i960_mmio_write_u32(0x884000, (u32)g12); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g13 = 0x21004242u;
    i960_mmio_write_u32(0x884000, (u32)g13); /* copro_fifo */;
    { u64 _p = *(u64 *)(fp + 0x60); g12 = (u32)_p; g13 = (u32)(_p >> 32); }
    g10 = i960_f64_to_u32(fp1);
    fp0 = i960_u32_to_f64(g10);
    r4 = i960_f64_to_u32((fp2) + (i960_u32_to_f64(r4)));
    g11 = *(u32 *)(fp + 0x48);
    fp3 = i960_rifl_read(g12, g13);
    i960_rifl_write(&g12, &g13, (fp3) + (fp0));
    i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
    fp1 = i960_u32_to_f64(g11);
    g4 = *(u32 *)(fp + 0x44);
    r8 = i960_f64_to_u32((fp1) + (i960_u32_to_f64(r8)));
    i960_mmio_write_u32(0x884000, (u32)r4); /* copro_fifo */;
    fp0 = i960_rifl_read(g12, g13);
    g10 = 0x9999999au;
    g11 = 0xbfc99999u;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g4 = i960_f64_to_u32(fp0);
    fp0 = i960_rifl_read(g10, g11);
    *(u32 *)(fp + 0x40) = (u32)r4;
    fp2 = i960_u32_to_f64(r4);
    fp2 = i960_u32_to_f64(r4);
    *(u32 *)(fp + 0x48) = (u32)r8;
    fp1 = i960_u32_to_f64(r8);
    fp1 = i960_u32_to_f64(r8);
    *(u32 *)(fp + 0x54) = (u32)g4;
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    g12 = 0x24004848u;
    g10 = 0x9999999au;
    g11 = 0x3fc99999u;
    i960_mmio_write_u32(0x884000, (u32)g12); /* copro_fifo */;
    i960_rifl_write(&g12, &g13, (fp0) * (fp2));
    fp0 = i960_rifl_read(g10, g11);
    fp2 = i960_rifl_read(g12, g13);
    i960_rifl_write(&g12, &g13, (fp0) * (fp1));
    i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
    fp1 = i960_rifl_read(g12, g13);
    g13 = 0x22004444u;
    i960_mmio_write_u32(0x884000, (u32)g13); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
    memcpy(&g4, (void *)a1, 4);
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g0 = (uintptr_t)a1 + 4;
    g4 = *(u32 *)g0;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g4 = i960_f64_to_u32(fp2);
    g7 = (uintptr_t)a1 + 0x8;
    g6 = *(u32 *)g7;
    g5 = i960_f64_to_u32(fp1);
    *(u32 *)(fp + 0x58) = (u32)g4;
    *(u32 *)(fp + 0x50) = (u32)g5;
    i960_mmio_write_u32(0x884000, (u32)g6); /* copro_fifo */;
    memcpy(&g4, (void *)a2, 4);
    g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) + (i960_u32_to_f64(g4)));
    memcpy((void *)a2, &g4, 4);
    g6 = (uintptr_t)a2 + 4;
    g4 = *(u32 *)g6;
    g5 = *(u32 *)(fp + 0x54);
    g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) + (i960_u32_to_f64(g4)));
    *(u32 *)g6 = (u32)g4;
    g6 = (uintptr_t)a2 + 8;
    g4 = *(u32 *)g6;
    g5 = *(u32 *)(fp + 0x58);
    g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) + (i960_u32_to_f64(g4)));
    *(u32 *)g6 = (u32)g4;
    g10 = 0x21804343u;
    i960_mmio_write_u32(0x884000, (u32)g10); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g14); /* copro_fifo */;
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    memcpy((void *)a1, &g4, 4);
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    *(u32 *)g0 = (u32)g4;
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0); /* copro_fifo */;
    *(u32 *)g7 = (u32)g4;
    i960_st_u32(I960_WORKRAM, 0x214264, 0, selected_row);
    g11 = 0x10802121u;
    i960_mmio_write_u32(0x884000, (u32)g11); /* copro_fifo */;
    g8 = save_g8;
    g10 = save_g10;
    g11 = save_g11;
    g12 = save_g12;
    fp = fp_save;
    sp = sp_save;
}
