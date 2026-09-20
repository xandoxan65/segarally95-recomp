/* Copro vec push @ 0x37200 — updates 0x2138xx slot tables from g6 index.
 * Called 4× from geo_view_scene_frame (`call`, not `bal`). On i960 that
 * allocates a callee frame; 0x40(fp)/0x70(fp) are temps, while g1/g2/g5
 * still point at the caller's AABB / force slots. Sharing the global `fp`
 * stomped those vectors (and the 0x2f staging at 0x70(fp) aliased g5),
 * so follow yaw integrated the world-scale leftover → desert intro X/Y
 * dolly. Same lift bug as geo_attract_copro_vec_scale.
 * source: disasm/maincpu/maincpu_037200_478.asm */
// @rom 0x37200 +0x478 geo_view_copro_vec_push

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_rom.h"

#include <math.h>
#include <string.h>

/* abi: g0=node VA, g1=XYZ host/guest, g2=vec host/guest (Y used),
 *      g3=pair host/guest, g4=scale float, g5=out XYZ host/guest, g6=slot idx.
 * Returns g0=0 (clear path) or g0=1 (main path). */

static int ptr_is_host(uintptr_t p)
{
    /*
     * Private-frame EAs are full host pointers. Truncating to u32 can land
     * inside workram by chance — never treat a high-bit pointer as guest.
     */
    if (p > 0xffffffffull)
        return 1;
    return model2_ram_mut((u32)p) == NULL;
}

static u32 ptr_ld_u32(uintptr_t p, u32 off)
{
    if (ptr_is_host(p)) {
        u32 v;

        memcpy(&v, (u8 *)p + off, 4);
        return v;
    }
    return i960_ld_u32(I960_ABS, (u32)p, off);
}

static void ptr_st_u32(uintptr_t p, u32 off, u32 v)
{
    if (ptr_is_host(p)) {
        memcpy((u8 *)p + off, &v, 4);
        return;
    }
    i960_st_u32(I960_ABS, (u32)p, off, v);
}

static u32 fbits(float f)
{
    u32 u;

    memcpy(&u, &f, 4);
    return u;
}

static float u2f(u32 u)
{
    float f;

    memcpy(&f, &u, 4);
    return f;
}

void geo_view_copro_vec_push(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    uintptr_t node = g0;
    uintptr_t p1 = g1;
    uintptr_t p2 = g2;
    uintptr_t p3 = g3;
    uintptr_t p5 = g5;
    u8 frame[0xa0];
    u32 g4_in = (u32)g4;
    u32 g6_i = (u32)g6;
    u32 slot_base;
    u32 lr8, lr9, lr10, lr11;
    u32 lr12, lr13, lr14, lr15;
    u32 slot_w20;
    u32 g2s;
    u32 mask;
    u32 nibble;
    u32 packed;
    u32 flag_byte;
    u32 y_sum;
    u32 os_addr;
    u32 g7v, g2v;
    u32 r4v, r5v, r8v, r9v, r10v;
    float f58;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x37200: call frame + lda 0x80(sp),sp; stq g8; st g12. */
    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    sp = sp + 0x80;
    *(u32 *)(sp - 0x20) = (u32)g8;
    *(u32 *)(sp - 0x1c) = (u32)g9;
    *(u32 *)(sp - 0x18) = (u32)g10;
    *(u32 *)(sp - 0x14) = (u32)g11;
    *(u32 *)(sp - 0x10) = (u32)g12;

    /* lda (g6)[g6*8] → g6*9; lda 0x2138d0[g7*4]. */
    slot_base = 0x2138d0u + (((g6_i + (g6_i << 3)) << 2));

    /* ldq 0x10(slot) → r8..r11 (r10/r11 are inputs to addr / nibble). */
    lr8 = i960_ld_u32(I960_WORKRAM, slot_base + 0x10u, 0);
    lr9 = i960_ld_u32(I960_WORKRAM, slot_base + 0x14u, 0);
    lr10 = i960_ld_u32(I960_WORKRAM, slot_base + 0x18u, 0);
    lr11 = i960_ld_u32(I960_WORKRAM, slot_base + 0x1cu, 0);

    /* ld 0x4(g2); addr r10,g13,g13. */
    y_sum = fbits(u2f(ptr_ld_u32(p2, 4)) + u2f(lr10));

    os_addr = (u32)node + 0x12u;
    {
        u16 os = (u16)i960_ld_u16(I960_WORKRAM, os_addr, 0);

        lr12 = i960_ld_u32(I960_WORKRAM, slot_base, 0);
        lr13 = i960_ld_u32(I960_WORKRAM, slot_base + 4u, 0);
        lr14 = i960_ld_u32(I960_WORKRAM, slot_base + 8u, 0);
        lr15 = i960_ld_u32(I960_WORKRAM, slot_base + 0xcu, 0);
        slot_w20 = i960_ld_u32(I960_WORKRAM, slot_base + 0x20u, 0);

        g2s = g6_i << 2;
        mask = 15u << g2s;
        packed = (u32)os & ~mask;
        nibble = lr11 & 15u;
        i960_st_u32(I960_WORKRAM, 0x2138a0u + g2s, 0, y_sum);
        packed |= (nibble << g2s);
        i960_st_u16(I960_WORKRAM, os_addr, 0, (u16)packed);

        /*
         * @0x37274: shlo r4,1,g7; or g7, 0x214208.
         * r4 is the flag nibble — analog bits at nibble<<1, not 1<<nibble.
         * 1<<nibble sent miss nibble 15 to bit 15 (frame_update scans 9..0
         * and would load 0x5d67a0[-1]); nibble 4 selected the wrong row.
         */
        i960_st_u32(I960_WORKRAM, 0x214208, 0,
                    i960_ld_u32(I960_WORKRAM, 0x214208, 0) | (nibble << 1));

        /* stq r8,0x50(fp); stq r12,0x40(fp); st r6,0x60(fp) — caller frame. */
        *(u32 *)(fp + 0x50) = lr8;
        *(u32 *)(fp + 0x54) = lr9;
        *(u32 *)(fp + 0x58) = lr10;
        *(u32 *)(fp + 0x5c) = lr11;
        *(u32 *)(fp + 0x40) = lr12;
        *(u32 *)(fp + 0x44) = lr13;
        *(u32 *)(fp + 0x48) = lr14;
        *(u32 *)(fp + 0x4c) = lr15;
        *(u32 *)(fp + 0x60) = slot_w20;

        /* @0x37274: shlo nibble,1 then or into 0x214208 (not 1<<nibble). */
        i960_st_u32(I960_WORKRAM, 0x214208, 0,
                    i960_ld_u32(I960_WORKRAM, 0x214208, 0) | (nibble << 1));

        flag_byte = (u32)(*(u8 *)(fp + 0x5c));
        f58 = u2f(*(u32 *)(fp + 0x58));

        /* bbs 7 → clear; else if f58 >= 0 → main @ 0x37364; else clear. */
        if (((flag_byte >> 7) & 1u) != 0u || f58 < 0.f)
            goto clear_path;
        goto main_path;
    }

clear_path:
    /* @0x372A0 */
    g2s = g6_i << 2;
    i960_st_u32(I960_WORKRAM, (u32)node + 0x64u + g2s, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x2138b0u + g2s, 0, 0);
    ptr_st_u32(p5, 0, 0);
    ptr_st_u32(p5, 4, 0);
    ptr_st_u32(p5, 8, 0);
    {
        u32 other = g6_i ^ 1u;
        float oval = u2f(i960_ld_u32(I960_WORKRAM, (u32)node + 0x64u + (other << 2), 0));

        if (oval > 0.f) {
            float k = (float)i960_rifl_read(0xbc6a7efau, 0x3f789374u);
            float acc = k * u2f(slot_w20);

            acc *= u2f(g4_in);
            acc *= u2f(i960_ld_u32(I960_WORKRAM, (u32)node + 0x50u, 0));
            acc *= u2f(*(u32 *)(fp + 0x50));
            ptr_st_u32(p5, 4, fbits(acc));
        }
    }
    g2s = g6_i << 2;
    i960_st_u32(I960_WORKRAM, 0x213890u + g2s, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x213880u + g2s, 0, 0);
    g0 = 0;
    goto done;

main_path:
    /* @0x37364: ldq 0x2141d0 → r4..r7. */
    r4v = i960_ld_u32(I960_WORKRAM, 0x2141d0, 0);
    r5v = i960_ld_u32(I960_WORKRAM, 0x2141d4, 0);
    {
        u32 r6_tab = i960_ld_u32(I960_WORKRAM, 0x2141d8, 0);
        u32 r7_tab = i960_ld_u32(I960_WORKRAM, 0x2141dc, 0);

        (void)r4v;
        /*
         * @0x3736C: cmpibl 1,g6,0x3737c.
         * The branch is taken when 1 < g6, so slots 2/3 use r6 and
         * 0x2141e0; slots 0/1 fall through to r5/r7.
         */
        if (g6_i > 1u) {
            g7v = r6_tab;
            g2v = i960_ld_u32(I960_WORKRAM, 0x2141e0, 0);
        } else {
            g7v = r5v;
            g2v = r7_tab;
        }
    }

    /* lda 0x50(fp),r5; ldq (r5) → r8..r11 (the stq'd slot+0x10 quad). */
    r8v = *(u32 *)(fp + 0x50);
    r9v = *(u32 *)(fp + 0x54);
    r10v = *(u32 *)(fp + 0x58);
    /* r11 at 0x5c unused until later */

    /* mulr g7,r10,g7; ×100 (0x40590000); ×0x214124. */
    g7v = fbits(u2f(g7v) * u2f(r10v));
    {
        float t = u2f(g7v) * 100.f;
        float scale124 = u2f(i960_ld_u32(I960_WORKRAM, 0x214124, 0));
        float fp2_save = scale124;

        t *= scale124;
        r4v = fbits(t);

        /* if r10 > 0.2: extra term into r4. */
        if (u2f(r10v) > 0.2f) {
            float d = u2f(r10v) - 0.2f;
            float acc = d * u2f(g4_in);

            acc *= u2f(i960_ld_u32(I960_WORKRAM, (u32)node + 0x50u, 0));
            acc += u2f(r4v);
            r10v = 0x3e4ccccdu; /* 0.2f */
            r4v = fbits(acc);
        }

        {
            float g4f = u2f(i960_ld_u32(I960_WORKRAM,
                                        (u32)node + 0x64u + (g6_i << 2), 0));

            g4f = g4f - u2f(r10v);
            g4f = g4f * u2f(g2v);
            g4f = g4f * fp2_save;
            /* × 3600 (double 0:0x40ac2000); sub from r4. */
            {
                float prod = g4f * 3600.f;
                float r4f = u2f(r4v) - prod;

                r4v = fbits(r4f);
            }
        }
    }

    if (u2f(r4v) < 0.f)
        r4v = 0;

    {
        u32 g13v = fbits(u2f(r4v) * u2f(r8v));
        u32 g7o = fbits(u2f(r4v) * u2f(r9v));
        u32 g4_fp = *(u32 *)(fp + 0x4c);
        u32 g2o = fbits(u2f(r4v) * u2f(g4_fp));

        g2s = g6_i << 2;
        i960_st_u32(I960_WORKRAM, (u32)node + 0x64u + g2s, 0, r10v);
        i960_st_u32(I960_WORKRAM, 0x2138b0u + g2s, 0, r4v);
        *(u32 *)(fp + 0x58) = r10v; /* st r10,0x8(r5) with r5=&fp+0x50 */
        *(u32 *)(fp + 0x90) = g13v;
        ptr_st_u32(p5, 4, g13v);
        ptr_st_u32(p5, 8, g7o);
        ptr_st_u32(p5, 0, g2o);
    }

    /* TGP 0x58: |B−A| with A=0, B=g1 XYZ. */
    i960_mmio_write_u32(0x884000, 0x2c005858u);
    i960_mmio_write_u32(0x884000, 0);
    i960_mmio_write_u32(0x884000, 0);
    i960_mmio_write_u32(0x884000, 0);
    i960_mmio_write_u32(0x884000, ptr_ld_u32(p1, 0));
    i960_mmio_write_u32(0x884000, ptr_ld_u32(p1, 4));
    i960_mmio_write_u32(0x884000, ptr_ld_u32(p1, 8));
    {
        u32 dist = i960_mmio_read_u32(0x884000);
        float eps = (float)i960_rifl_read(0xd9d7bdbbu, 0x3ddb7cdfu);

        /* movr dist → fp0/fp1; fp1 kept for 0x213890 store below. */
        if (u2f(dist) <= eps) {
            g2s = g6_i << 2;
            i960_st_u32(I960_WORKRAM, 0x213880u + g2s, 0, 0);
            i960_st_u32(I960_WORKRAM, 0x213890u + g2s, 0, 0);
            g0 = 1;
            goto done;
        }

        /*
         * @0x37538: stage (x,0,z) at fp+0x70; TGP 0x2f normalize readback.
         * Disasm @ 0x37550–0x375B4: bare 0x2f, no 0x20/0x21/0x25.
         * Host 0x2f is normalize-only (no R). scene_frame @ 0x35E2C does
         * 0x20+0x25 then rebuilds from angles — it does not need look-along
         * left by this 0x2f. Invented push/pop around 0x2f restored the
         * pre-0x2f parent and cam follow Y ran away; bare 0x25 (no push)
         * zeroed live T → NaNs. Leave bare 0x2f as disasm.
         */
        {
            u32 gx = ptr_ld_u32(p1, 0);
            u32 gz = ptr_ld_u32(p1, 8);
            u32 n0, n1, n2;
            u32 p3_0, p3_8;
            float cross;
            u32 g3_sign;
            float dot;
            u32 g4v;

            *(u32 *)(fp + 0x70) = gx;
            *(u32 *)(fp + 0x74) = 0;
            *(u32 *)(fp + 0x78) = gz;
            i960_mmio_write_u32(0x884000, 0x17802f2fu);
            i960_mmio_write_u32(0x884000, *(u32 *)(fp + 0x70));
            i960_mmio_write_u32(0x884000, *(u32 *)(fp + 0x74));
            i960_mmio_write_u32(0x884000, *(u32 *)(fp + 0x78));
            n0 = i960_mmio_read_u32(0x884000);
            *(u32 *)(fp + 0x70) = n0;
            n1 = i960_mmio_read_u32(0x884000);
            n2 = i960_mmio_read_u32(0x884000);
            *(u32 *)(fp + 0x74) = n1;
            *(u32 *)(fp + 0x80) = n0;
            *(u32 *)(fp + 0x88) = n2;

            p3_0 = ptr_ld_u32(p3, 0);
            p3_8 = ptr_ld_u32(p3, 8);
            /* cross = n0*p3_z − n2*p3_x; sign → ±1. */
            cross = u2f(n0) * u2f(p3_8) - u2f(n2) * u2f(p3_0);
            g3_sign = (cross >= 0.f) ? 0x3f800000u : 0xbf800000u;
            /* dot = n0*p3_x + n2*p3_z. */
            dot = u2f(n0) * u2f(p3_0) + u2f(n2) * u2f(p3_8);
            g4v = fbits(dot);
            if (dot < 0.f)
                g4v ^= 1u << 31; /* notbit 31 → abs */
            if (u2f(g4v) > 1.f)
                g4v = 0x3f800000u;
            {
                float s = u2f(g4v);

                s = s * s;
                s = 1.f - s;
                if (s < 0.f)
                    s = 0.f;
                s = sqrtf(s);
                s = u2f(g3_sign) * s;
                g2s = g6_i << 2;
                /* movr fp1,g12 — fp1 still holds 0x58 distance. */
                i960_st_u32(I960_WORKRAM, 0x213890u + g2s, 0, dist);
                i960_st_u32(I960_WORKRAM, 0x213880u + g2s, 0, fbits(s));
            }
        }
    }
    g0 = 1;

done:
    g8 = *(u32 *)(sp_save + 0x80 - 0x20);
    g9 = *(u32 *)(sp_save + 0x80 - 0x1c);
    g10 = *(u32 *)(sp_save + 0x80 - 0x18);
    g11 = *(u32 *)(sp_save + 0x80 - 0x14);
    g12 = *(u32 *)(sp_save + 0x80 - 0x10);
    sp = sp_save;
    fp = fp_save;
}
