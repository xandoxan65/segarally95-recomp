/* Semantic C from MAME disasm @ 0x3ed20 — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_03ed20_168.asm */
// @rom 0x3ed20 +0x168 geo_attract_pen_angle_push

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_geo.h"
#include "model2_rom.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Entry via g0..g5 (set by geo_attract_pen_prg_push_b):
 *   g0 = table ptr, g1 = half-count, g2 = object*, g3 = object+0x18*,
 *   g4 = 0x20220c, g5 = pen_mode.
 * Returns object opcode 0x800100+slot and fills 0x214380 halfwords.
 *
 * Attract glyph_emit passes host fp-shadow object pointers (same class as
 * object_pen). Race desert_node may pass guest VAs. Truncating g2/g3 to u32
 * and i960_ld made attract cabin/glass see obj=(0,0,0) — wrong UV bias and
 * yaw for pen_mode tables @ 0x5ddb70 (Delta OBA 0x815430).
 */

static int ptr_is_host(uintptr_t p)
{
    if (p > 0xffffffffull)
        return 1;
    return model2_ram_mut((u32)p) == NULL;
}

static u32 ld_obj_u32(uintptr_t obj, u32 off)
{
    if (ptr_is_host(obj)) {
        u32 v;

        memcpy(&v, (const u8 *)obj + off, 4);
        return v;
    }
    return i960_ld_u32(I960_ABS, (u32)obj, off);
}

u32 geo_attract_pen_angle_push(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t obj;
    uintptr_t obj18;
    u32 wr_base;
    u32 pen_mode;
    u32 table_ptr;
    u32 half;
    u32 a, b, c, d;
    u32 obj_y;
    float f6, f7, fang, frem;
    double fp_angle;
    double fp_scale;
    i32 r8, r6;
    u32 opcode;
    u32 cursor;
    u32 limit = 0x7fdu;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    table_ptr = (u32)g0;
    half = (u32)g1;
    obj = (uintptr_t)g2;
    obj18 = (uintptr_t)g3;
    wr_base = (u32)g4;
    pen_mode = (u32)g5;

    a = i960_ld_u32(I960_WORKRAM, wr_base, 0);
    b = ld_obj_u32(obj, 0);
    c = i960_ld_u32(I960_WORKRAM, wr_base, 8);
    d = ld_obj_u32(obj, 8);
    obj_y = ld_obj_u32(obj, 4);
    /* Disasm: subr → g7=wr.x−obj.x, g6=wr.z−obj.z; atanr g6,g7,g6.
     * MAME atanr = atan2(src2, src1) → atan2(x_delta, z_delta). */
    f7 = (float)(i960_u32_to_f64(a) - i960_u32_to_f64(b));
    f6 = (float)(i960_u32_to_f64(c) - i960_u32_to_f64(d));
    fang = atan2f(f7, f6);
    fang += (float)i960_u32_to_f64(ld_obj_u32(obj18, 4));

    {
        static unsigned s_yaw;

        if (s_yaw < 4u) {
            lift_log(
                    "lift: pen_yaw atan2(dx=%.3g,dz=%.3g)=%.3g deg "
                    "focus=(%.3g,%.3g,%.3g) obj=(%.3g,%.3g,%.3g) host=%d\n",
                    f7, f6, fang * (180.f / 3.14159265f),
                    (float)i960_u32_to_f64(a),
                    (float)i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, wr_base, 4)),
                    (float)i960_u32_to_f64(c),
                    (float)i960_u32_to_f64(b),
                    (float)i960_u32_to_f64(obj_y),
                    (float)i960_u32_to_f64(d),
                    ptr_is_host(obj));
            s_yaw++;
        }
    }

    /* remr with 2π (0x40c90fdb). */
    frem = fmodf(fang, (float)i960_u32_to_f64(0x40c90fdbu));
    if (pen_mode == 10u)
        frem = (float)i960_u32_to_f64(
            (u32)i960_f64_to_u32((double)frem) ^ 0x80000000u);

    fp_angle = (double)frem;
    if (fp_angle < 0.0)
        fp_angle += i960_rifl_read(0x54442d18u, 0x401921fbu);

    {
        double dy = i960_u32_to_f64(obj_y) - i960_u32_to_f64(
            i960_ld_u32(I960_WORKRAM, wr_base, 4));
        double t = dy + i960_rifl_read(0x55555555u, 0x40555555u);
        double k = i960_rifl_read(0x6dc9c883u, 0x40845f30u);
        double eight = i960_rifl_read(0, 0x40200000u);

        fp_scale = fp_angle * k;
        t = t * eight;
        r8 = (i32)fp_scale;
        r6 = (i32)t;
    }

    cursor = i960_ld_u32(I960_WORKRAM, 0x214374, 0);
    half >>= 1;
    opcode = 0x800100u + cursor;
    if (cursor + half > limit)
        goto done;

    half--;
    if (half == 0xffffffffu)
        goto done;

    while (half != 0xffffffffu) {
        u16 s0 = (u16)i960_ld_u16(I960_WORKRAM, table_ptr, 0);
        u16 s1;
        u32 ia, ib;

        table_ptr += 2u;
        s1 = (u16)i960_ld_u16(I960_WORKRAM, table_ptr, 0);
        table_ptr += 2u;
        if (s1 != 0) {
            s1 = (u16)((u32)s1 + (u32)r8);
            s0 = (u16)((u32)s0 + (u32)r6);
        }
        ia = cursor;
        ib = cursor + 1u;
        half--;
        i960_st_u16(I960_WORKRAM, 0x214380u + (ia << 1), 0, s0);
        if (half == 0xffffffffu) {
            i960_st_u16(I960_WORKRAM, 0x214380u + (ib << 1), 0, s1);
            cursor = ib + 1u;
            break;
        }
        i960_st_u16(I960_WORKRAM, 0x214380u + (ib << 1), 0, s1);
        cursor = ib + 1u;
    }

    /*
     * Hardware: geo_fifo_emit @ 0x3eca8 uploads 0x214380 → geo_texture_data
     * (0x800040 + FIFO) at end of frame. Host PRG decode can run as soon as
     * pen_prg_push_b stq's the object — before emit — so mirror the staging
     * window into texture_ram now (same words emit would push).
     */
    {
        u32 start = opcode - 0x800100u;
        u32 n = cursor - start;
        u32 *buf;
        u32 i;

        if (n > 0u && n <= limit && (buf = (u32 *)malloc((size_t)n * sizeof(u32)))) {
            for (i = 0; i < n; i++)
                buf[i] = i960_ld_u16(I960_WORKRAM, 0x214380u + ((start + i) << 1), 0);
            model2_geo_upload_texture(opcode, n, buf);
            free(buf);
        }
    }

done:
    i960_st_u32(I960_WORKRAM, 0x214374, 0, cursor);
    return opcode;
}
