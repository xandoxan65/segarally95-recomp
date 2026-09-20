/* Semantic C — private host frame; guest/host outs via i960 or memcpy.
 * source: disasm/maincpu/maincpu_02acc0_210.asm */
// @rom 0x2acc0 +0x210 geo_view_table_index_b

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "model2_rom.h"
#include "i960_mem.h"

#include <stdio.h>
#include <string.h>

/* abi: void * arg0=object XYZ (g0), void * arg1=out struct (g1), u32 arg2=g2 */

static int ptr_is_host(uintptr_t p)
{
    if (p > 0xffffffffull)
        return 1;
    return model2_ram_mut((u32)p) == NULL;
}

static u32 ld_obj_u32(u32 va, u32 off, u8 *host)
{
    if (host != NULL) {
        u32 v;

        memcpy(&v, host + off, 4);
        return v;
    }
    return i960_ld_u32(I960_ABS, va, off);
}

static void st_out_u32(u32 va, u32 off, u32 v, u8 *host)
{
    if (host != NULL) {
        memcpy(host + off, &v, 4);
        return;
    }
    i960_st_u32(I960_ABS, va, off, v);
}

static u32 frame_ld(u8 *frame, u32 off)
{
    u32 v;

    memcpy(&v, frame + off, 4);
    return v;
}

static void frame_st(u8 *frame, u32 off, u32 v)
{
    memcpy(frame + off, &v, 4);
}

void geo_view_table_index_b(void *arg0, void *arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    u8 frame[0xa0];
    u32 obj_va = (u32)(uintptr_t)arg0;
    u32 out_va = (u32)(uintptr_t)arg1;
    u8 *obj_host = ptr_is_host((uintptr_t)arg0) ? (u8 *)arg0 : NULL;
    u8 *out_host = ptr_is_host((uintptr_t)arg1) ? (u8 *)arg1 : NULL;
    u32 flag;
    u32 r4, r5, r6, r7;
    u32 r8, r9, r10, r11;
    u32 g4v, g5v, g6v, g7v;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    sp = sp + 0x50;

    /* TGP 0x53 with (arg2 << 1); 16 float readbacks + flag. */
    i960_mmio_write_u32(0x884000, 0x29805353u);
    i960_mmio_write_u32(0x884000, arg2 << 1);

    frame_st(frame, 0x60, i960_mmio_read_u32(0x884000));
    frame_st(frame, 0x64, i960_mmio_read_u32(0x884000));
    frame_st(frame, 0x68, i960_mmio_read_u32(0x884000));
    frame_st(frame, 0x6c, i960_mmio_read_u32(0x884000));
    frame_st(frame, 0x70, i960_mmio_read_u32(0x884000));
    frame_st(frame, 0x74, i960_mmio_read_u32(0x884000));
    frame_st(frame, 0x78, i960_mmio_read_u32(0x884000));
    frame_st(frame, 0x7c, i960_mmio_read_u32(0x884000));
    frame_st(frame, 0x80, i960_mmio_read_u32(0x884000));
    frame_st(frame, 0x84, i960_mmio_read_u32(0x884000));
    {
        u32 w88 = i960_mmio_read_u32(0x884000);
        u32 w8c = i960_mmio_read_u32(0x884000);

        frame_st(frame, 0x88, w88);
        frame_st(frame, 0x50, i960_mmio_read_u32(0x884000));
        frame_st(frame, 0x54, i960_mmio_read_u32(0x884000));
        frame_st(frame, 0x58, i960_mmio_read_u32(0x884000));
        frame_st(frame, 0x8c, w8c);
        frame_st(frame, 0x5c, i960_mmio_read_u32(0x884000));
    }
    flag = i960_mmio_read_u32(0x884000);

    {
        static int flag_logged;

        if (flag_logged < 8) {
            lift_log(
                    "lift: table_index_b 0x53 raw=%#x stored_slot=%#x out=%#x\n",
                    (unsigned)flag, (unsigned)arg2, (unsigned)out_va);
            fflush(stderr);
            flag_logged++;
        }
    }

    if (flag == 0u) {
        /*
         * Disasm @ 0x2AEB8–0x2AEC0: mov 15 → st out+0x1c; ret.
         * Preserve out+0x0c..0x18 exactly as the i960 does: they hold the
         * last valid road plane used by the following support-force pass.
         */
        st_out_u32(out_va, 0x1c, 15u, out_host);
        goto done;
    }

    {
        u32 hi = flag >> 16;
        u32 masked = hi & 0x8fu;
        u32 bit7 = masked & 0x80u;

        st_out_u32(out_va, 0x1c, masked, out_host);
        if (bit7 != 0u)
            goto path_bit7;
    }

    /* @0x2ADCC: ldq 0x50(fp) → r4..r7; plane vs object. */
    r4 = frame_ld(frame, 0x50);
    r5 = frame_ld(frame, 0x54);
    r6 = frame_ld(frame, 0x58);
    r7 = frame_ld(frame, 0x5c);
    g4v = r4 ^ (1u << 31); /* notbit 31 */
    g5v = ld_obj_u32(obj_va, 8, obj_host);
    g6v = ld_obj_u32(obj_va, 0, obj_host);
    g5v = (u32)i960_f64_to_u32(i960_u32_to_f64(r6) * i960_u32_to_f64(g5v));
    g4v = (u32)i960_f64_to_u32(i960_u32_to_f64(g4v) * i960_u32_to_f64(g6v));
    g4v = (u32)i960_f64_to_u32(i960_u32_to_f64(g4v) - i960_u32_to_f64(g5v));
    g4v = (u32)i960_f64_to_u32(i960_u32_to_f64(g4v) - i960_u32_to_f64(r7));
    g4v = (u32)i960_f64_to_u32(i960_u32_to_f64(g4v) / i960_u32_to_f64(r5));
    g5v = ld_obj_u32(obj_va, 4, obj_host);
    g4v = (u32)i960_f64_to_u32(i960_u32_to_f64(g4v) - i960_u32_to_f64(g5v));
    st_out_u32(out_va, 0x18, g4v, out_host);

    /*
     * TGP 0x2f normalize of (r4,r5,r6); write XYZ to out+0xc.
     * Disasm @ 0x2AE04–0x2AE68: bare 0x2f then ret — no 0x20/0x21/0x25.
     */
    i960_mmio_write_u32(0x884000, 0x17802f2fu);
    frame_st(frame, 0x40, r4);
    frame_st(frame, 0x44, r5);
    frame_st(frame, 0x48, r6);
    i960_mmio_write_u32(0x884000, frame_ld(frame, 0x40));
    i960_mmio_write_u32(0x884000, frame_ld(frame, 0x44));
    i960_mmio_write_u32(0x884000, frame_ld(frame, 0x48));
    st_out_u32(out_va, 0x0c, i960_mmio_read_u32(0x884000), out_host);
    st_out_u32(out_va, 0x10, i960_mmio_read_u32(0x884000), out_host);
    st_out_u32(out_va, 0x14, i960_mmio_read_u32(0x884000), out_host);
    goto done;

path_bit7:
    /* @0x2AE6C: ldq 0x50 → r8..r11; ldq 0x60 → r4..r7. */
    r8 = frame_ld(frame, 0x50);
    r9 = frame_ld(frame, 0x54);
    r10 = frame_ld(frame, 0x58);
    r11 = frame_ld(frame, 0x5c);
    r4 = frame_ld(frame, 0x60);
    r5 = frame_ld(frame, 0x64);
    r6 = frame_ld(frame, 0x68);
    r7 = frame_ld(frame, 0x6c);
    st_out_u32(out_va, 0x0c, r8, out_host);
    st_out_u32(out_va, 0x10, r9, out_host);
    st_out_u32(out_va, 0x14, r10, out_host);

    g7v = ld_obj_u32(obj_va, 8, obj_host);
    g5v = frame_ld(frame, 0x74);
    g6v = (u32)i960_f64_to_u32(i960_u32_to_f64(r6) - i960_u32_to_f64(g7v));
    g5v = (u32)i960_f64_to_u32(i960_u32_to_f64(g5v) - i960_u32_to_f64(g7v));
    {
        u32 obj_x = ld_obj_u32(obj_va, 0, obj_host);

        g4v = (u32)i960_f64_to_u32(i960_u32_to_f64(r4) - i960_u32_to_f64(obj_x));
        g7v = (u32)i960_f64_to_u32(i960_u32_to_f64(r7) - i960_u32_to_f64(obj_x));
    }
    g4v = (u32)i960_f64_to_u32(i960_u32_to_f64(g4v) * i960_u32_to_f64(g5v));
    g6v = (u32)i960_f64_to_u32(i960_u32_to_f64(g6v) * i960_u32_to_f64(g7v));
    g4v = (u32)i960_f64_to_u32(i960_u32_to_f64(g4v) - i960_u32_to_f64(g6v));
    g4v = g4v & ~(1u << 31); /* clrbit 31 */
    g4v = (u32)i960_f64_to_u32(i960_u32_to_f64(g4v) / i960_u32_to_f64(r11));
    st_out_u32(out_va, 0x18, g4v, out_host);

done:
    sp = sp_save;
    fp = fp_save;
}
