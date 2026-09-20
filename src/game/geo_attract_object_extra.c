/* Semantic C from MAME disasm @ 0x317b0 — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_0317b0_80c.asm */
// @rom 0x317b0 +0x80c geo_attract_object_extra

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "lift_syms.h"
#include "model2_rom.h"

#include <string.h>

/*
 * Extra attract object copro burst when remaining budget >= 5.
 *
 * Jump table @ ROM-mirrored 0x5D0AE8 (literal words @ 0x31AE8):
 *   index 0,1,4 → case @ 0x5D0D2C (ROM 0x31D2C)
 *   index 2,3   → case @ 0x5D0B00 (ROM 0x31B00)
 *
 * Bit loop: r7 (=obj cursor) advances +4 each bit so scale is ld 0x30(r7)
 * → obj+0x30/34/38/3c. r12 keeps base object. r13 = 2×depth from entry 0x2c.
 */

static u32 f_xor_sign(u32 bits)
{
    return bits ^ 0x80000000u;
}

static u32 f_abs(u32 bits)
{
    return bits & 0x7fffffffu;
}

static u32 rec_va(u32 slot)
{
    return 0x20cd90u + ((slot + (slot << 3)) << 2);
}

static void fill_record_common(u32 slot, u32 nibble, u32 r5bits, u8 *frame,
                               const u8 *obj_bytes, int or4, double scale_k)
{
    u32 ea = rec_va(slot);
    u8 *rb;
    u8 b;
    u16 w16;
    u32 w32;
    u32 half_dx;
    u32 rng;
    u32 q;

    rb = model2_ram_mut(ea);
    if (!rb)
        return;
    b = rb[0];
    b = (u8)((b & (u8)(3u << 6)) | ((obj_bytes[0x50] >> 5) & 3u) | (or4 ? 4u : 0u));
    rb[0] = b;
    w16 = *(u16 *)rb;
    w16 = (u16)((w16 & 0xfc3fu) | (nibble << 6));
    *(u16 *)rb = w16;

    half_dx = i960_f64_to_u32(i960_u32_to_f64(r5bits) * scale_k);
    if (!or4) {
        w32 = *(u32 *)rb;
        w32 = (w32 & 0xfffc03ffu) | (15u << 11);
        *(u32 *)rb = w32;
        *(u64 *)(rb + 4) = *(u64 *)(frame + 0x40);
        *(u32 *)(rb + 0xc) = *(u32 *)(frame + 0x48);
        *(u32 *)(rb + 0x10) = 0;
        *(u32 *)(rb + 0x14) = half_dx;
        *(u32 *)(rb + 0x18) = 0;
    } else {
        *(u64 *)(rb + 4) = *(u64 *)(frame + 0x40);
        *(u32 *)(rb + 0xc) = *(u32 *)(frame + 0x48);
        *(u32 *)(rb + 0x10) = i960_f64_to_u32(
            i960_u32_to_f64(*(u32 *)(frame + 0x50))
            - i960_u32_to_f64(*(u32 *)(frame + 0x40)));
        *(u32 *)(rb + 0x14) = half_dx;
        *(u32 *)(rb + 0x18) = i960_f64_to_u32(
            i960_u32_to_f64(*(u32 *)(frame + 0x58))
            - i960_u32_to_f64(*(u32 *)(frame + 0x48)));
        *(u32 *)(rb + 0x20) = 0;
    }

    geo_rng_next();
    rng = (g0 & 3u) + 1u;
    q = i960_f64_to_u32((double)(i32)rng);
    if ((i32)rng < 0)
        q = i960_f64_to_u32(i960_u32_to_f64(0x4f800000u) + i960_u32_to_f64(q));
    *(u32 *)(rb + 28) = i960_f64_to_u32(i960_u32_to_f64(r5bits) * i960_u32_to_f64(q));
}

void geo_attract_object_extra(void * arg0, u32 arg1, u32 arg2)
{
    u32 *obj = (u32 *)arg0;
    const u8 *obj_b;
    u8 frame[0x80];
    uintptr_t fp_save = fp;
    u32 v;
    u32 depth2_bits;
    u32 *cnt_p;
    u32 *lim_p;
    u32 tbl_va;
    u32 bit_i;
    u32 mode_nibbles;
    u32 nibble;
    u32 jt;
    u32 slot;
    u32 used;
    u32 free_ix;
    u32 d0, d1, d2;
    u32 t0, t1, t2;
    u32 r5bits;
    u32 half_sc;
    u32 scale_word;
    u32 tx, ty, tz;
    u32 y_r13_scale;
    double half = i960_rifl_read(0u, 0x3fe00000u);
    double scale_raw;
    double scale_half;

    (void)arg1;
    (void)arg2;
    if (!obj)
        return;
    obj_b = (const u8 *)obj;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;

    /* @0x317C0–0x31840: depth query; r13 ← 2×distance. */
    i960_mmio_write_u32(0x884000, 0x2c005858u);
    i960_mmio_write_u32(0x884000, 0u);
    i960_mmio_write_u32(0x884000, 0u);
    i960_mmio_write_u32(0x884000, 0u);
    i960_mmio_write_u32(0x884000, obj[3]);
    i960_mmio_write_u32(0x884000, obj[4]);
    i960_mmio_write_u32(0x884000, obj[5]);
    v = i960_mmio_read_u32(0x884000);
    depth2_bits = i960_f64_to_u32(i960_u32_to_f64(v) + i960_u32_to_f64(v));
    if (i960_u32_to_f64(depth2_bits) <= 0.0)
        goto done;
    if (i960_ld_u8(I960_WORKRAM, 0x20d7fc, 0) == 0)
        goto done;

    /* @0x31850–0x3192C */
    i960_mmio_write_u32(0x884000, 0x10002020u);
    i960_mmio_write_u32(0x884000, 0x12802525u);
    i960_mmio_write_u32(0x884000, 0x15002a2au);
    i960_mmio_write_u32(0x884000, f_xor_sign(obj[7]));
    i960_mmio_write_u32(0x884000, 0x16002c2cu);
    i960_mmio_write_u32(0x884000, obj[3]);
    i960_mmio_write_u32(0x884000, 0u);
    i960_mmio_write_u32(0x884000, obj[5]);
    *(u32 *)(frame + 0x60) = i960_mmio_read_u32(0x884000);
    {
        double s = i960_u32_to_f64(*(u32 *)(frame + 0x60));
        double k = i960_rifl_read(0x33333333u, 0x3fe33333u);

        s *= k;
        *(u32 *)(frame + 0x60) = i960_f64_to_u32(s);
        *(u32 *)(frame + 0x64) = f_abs(*(u32 *)(frame + 0x60));
    }
    (void)i960_mmio_read_u32(0x884000);
    (void)i960_mmio_read_u32(0x884000);
    i960_mmio_write_u32(0x884000, 0x10802121u);

    if ((obj_b[0x50] & (3u << 5)) == 0) {
        cnt_p = (u32 *)(model2_crx_ram + 0xd7e8);
        lim_p = (u32 *)(model2_crx_ram + 0xd7f0);
    } else {
        cnt_p = (u32 *)(model2_crx_ram + 0xd7ec);
        lim_p = (u32 *)(model2_crx_ram + 0xd7f4);
    }
    v = obj_b[0x50] & 15u;
    tbl_va = (v == 1u || v == 4u) ? 0x5d0690u : 0x5d0660u;

    /* @0x31990–0x31A34: object pose */
    i960_mmio_write_u32(0x884000, 0x10002020u);
    i960_mmio_write_u32(0x884000, 0x12802525u);
    i960_mmio_write_u32(0x884000, 0x13802727u);
    i960_mmio_write_u32(0x884000, obj[0]);
    i960_mmio_write_u32(0x884000, obj[1]);
    i960_mmio_write_u32(0x884000, obj[2]);
    i960_mmio_write_u32(0x884000, 0x15002a2au);
    i960_mmio_write_u32(0x884000, obj[7]);
    i960_mmio_write_u32(0x884000, 0x14802929u);
    i960_mmio_write_u32(0x884000, obj[6]);
    i960_mmio_write_u32(0x884000, 0x15802b2bu);
    i960_mmio_write_u32(0x884000, obj[8]);

    mode_nibbles = *(const u16 *)(obj_b + 0x52);
    bit_i = 0;

bit_loop:
    v = i960_ld_u8(I960_WORKRAM, 0x20d7fc, 0);
    nibble = mode_nibbles & 15u;
    if (((v >> bit_i) & 1u) == 0)
        goto bit_next;

    /* @0x31A58: ld 0x30(r7) with r7 = obj + bit_i*4 */
    scale_word = obj[12 + bit_i];
    scale_raw = i960_u32_to_f64(scale_word);

    /* even bit_i → −half; odd → +half; gate on original scale */
    if ((bit_i & 1u) == 0)
        scale_half = -(scale_raw * half);
    else
        scale_half = scale_raw * half;

    if ((i32)i960_ld_u32(I960_WORKRAM, 0x20d7e0, 0)
        >= (i32)i960_ld_u32(I960_WORKRAM, 0x20d800, 0))
        goto bit_next;
    if (scale_raw <= 0.0)
        goto bit_next;

    jt = nibble - 2u;
    if (jt >= 5u)
        goto bit_next;
    half_sc = i960_f64_to_u32(scale_half);

    if ((i32)*cnt_p >= (i32)*lim_p)
        goto bit_next;
    *cnt_p = *cnt_p + 1u;
    free_ix = i960_ld_u32(I960_WORKRAM, 0x20d7e4, 0) - 1u;
    i960_st_u32(I960_WORKRAM, 0x20d7e4, 0, free_ix);
    slot = i960_ld_u32(I960_WORKRAM, 0x20d6f0, free_ix << 2);
    used = i960_ld_u32(I960_WORKRAM, 0x20d7e0, 0);
    i960_st_u32(I960_WORKRAM, 0x20d7e0, 0, used + 1u);
    i960_st_u32(I960_WORKRAM, 0x20d600, used << 2, slot);

    tx = i960_ld_u32(I960_WORKRAM, tbl_va, bit_i * 12u);
    ty = i960_ld_u32(I960_WORKRAM, tbl_va, bit_i * 12u + 4u);
    tz = i960_ld_u32(I960_WORKRAM, tbl_va, bit_i * 12u + 8u);

    i960_mmio_write_u32(0x884000, 0x10002020u);
    i960_mmio_write_u32(0x884000, 0x13802727u);
    i960_mmio_write_u32(0x884000, tx);
    i960_mmio_write_u32(0x884000,
                        i960_f64_to_u32(i960_u32_to_f64(ty)
                                        + i960_u32_to_f64(scale_word)));
    i960_mmio_write_u32(0x884000, tz);

    if (jt == 2u || jt == 3u) {
        /* @0x31B00: mulr r13,fp2 → Y offset on second 0x2c */
        y_r13_scale = i960_f64_to_u32(i960_u32_to_f64(depth2_bits)
                                      * i960_u32_to_f64(scale_word));

        i960_mmio_write_u32(0x884000, 0x15002a2au);
        i960_mmio_write_u32(0x884000, obj[9]); /* +0x24, base object */
        i960_mmio_write_u32(0x884000, 0x16002c2cu);
        i960_mmio_write_u32(0x884000, 0u);
        i960_mmio_write_u32(0x884000, 0u);
        i960_mmio_write_u32(0x884000, 0u);
        d0 = i960_mmio_read_u32(0x884000);
        d1 = i960_mmio_read_u32(0x884000);
        d2 = i960_mmio_read_u32(0x884000);
        i960_mmio_write_u32(0x884000, 0x16002c2cu);
        i960_mmio_write_u32(0x884000,
                            i960_f64_to_u32(i960_u32_to_f64(*(u32 *)(frame + 0x60))
                                            + i960_u32_to_f64(half_sc)));
        i960_mmio_write_u32(0x884000,
                            i960_f64_to_u32(i960_u32_to_f64(*(u32 *)(frame + 0x64))
                                            + i960_u32_to_f64(y_r13_scale)));
        i960_mmio_write_u32(0x884000, 0u);
        t0 = i960_mmio_read_u32(0x884000);
        t1 = i960_mmio_read_u32(0x884000);
        t2 = i960_mmio_read_u32(0x884000);
        *(u32 *)(frame + 0x40) = d0;
        *(u32 *)(frame + 0x44) = d1;
        *(u32 *)(frame + 0x48) = d2;
        *(u32 *)(frame + 0x50) = t0;
        *(u32 *)(frame + 0x54) = t1;
        *(u32 *)(frame + 0x58) = t2;
        r5bits = i960_f64_to_u32(i960_u32_to_f64(t1) - i960_u32_to_f64(d1));
        if (i960_u32_to_f64(r5bits) < 0.0)
            r5bits = f_xor_sign(r5bits);
        fill_record_common(slot, nibble, r5bits, frame, obj_b, 1, half);
    } else {
        /*
         * @0x31D90–0x31E54: Y probe is frame+0x64 + 0.1·r13 (depth2), not
         * 1.1·(frame+0x64). Lift had mul by k01 on 0x64 twice; that left dust
         * sy≈0.05 (sub-pixel) instead of O(depth).
         */
        double k01 = i960_rifl_read(0x9999999au, 0x3fb99999u);

        i960_mmio_write_u32(0x884000, 0x16002c2cu);
        i960_mmio_write_u32(0x884000, 0u);
        i960_mmio_write_u32(0x884000, 0u);
        i960_mmio_write_u32(0x884000, 0u);
        d0 = i960_mmio_read_u32(0x884000);
        d1 = i960_mmio_read_u32(0x884000);
        d2 = i960_mmio_read_u32(0x884000);
        i960_mmio_write_u32(0x884000, 0x16002c2cu);
        i960_mmio_write_u32(0x884000, 0u);
        i960_mmio_write_u32(0x884000,
                            i960_f64_to_u32(
                                i960_u32_to_f64(*(u32 *)(frame + 0x64))
                                + i960_u32_to_f64(depth2_bits) * k01));
        i960_mmio_write_u32(0x884000, 0u);
        t0 = i960_mmio_read_u32(0x884000);
        t1 = i960_mmio_read_u32(0x884000);
        t2 = i960_mmio_read_u32(0x884000);
        *(u32 *)(frame + 0x40) = d0;
        *(u32 *)(frame + 0x44) = d1;
        *(u32 *)(frame + 0x48) = d2;
        *(u32 *)(frame + 0x50) = t0;
        *(u32 *)(frame + 0x54) = t1;
        *(u32 *)(frame + 0x58) = t2;
        r5bits = i960_f64_to_u32(i960_u32_to_f64(t1) - i960_u32_to_f64(d1));
        if (i960_u32_to_f64(r5bits) < 0.0)
            r5bits = f_xor_sign(r5bits);
        fill_record_common(slot, nibble, r5bits, frame, obj_b, 0,
                           i960_rifl_read(0x33333333u, 0x3fd33333u));
    }

    /* @0x31F74 */
    i960_mmio_write_u32(0x884000, 0x10802121u);

bit_next:
    mode_nibbles >>= 4;
    bit_i++;
    if (bit_i <= 3u)
        goto bit_loop;

    i960_mmio_write_u32(0x884000, 0x10802121u);

done:
    fp = fp_save;
}
