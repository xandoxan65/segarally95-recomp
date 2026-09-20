/* Race camera seed @ 0x1F1D0 — staged sentinel 0x5BE1D0; install-time fill
 * of the cam object at *0x2140cc from per-course float rows @ 0x5BE140, then
 * TGP length normalize + view-seed tail shared with the odd-sub path.
 *
 * cam_bind calls this only when installing the sentinel (disasm @ 0x20324).
 * logo_path may still have 0x2140cc == 0 (0x2AA90 runs after cam_boot) —
 * refuse unmapped cam writes like desert cam refuses a bad 0x213980.
 *
 * Frame: lda 0x130(sp),sp. Float quads land at 0x80(fp)..0x10C. Course index
 * uses lda (EA) into those rows: 0x40(fp)[course*9] + 0x40 → 0x80 row base.
 *
 * source: disasm/maincpu/maincpu_01f1d0_850.asm */
// @rom 0x1f1d0 +0x810 game_start_race_cam_seed

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "i960_host.h"
#include "model2_rom.h"

#include <stdio.h>
#include <string.h>

static u32 frame_u32(u8 *frame, u32 off)
{
    u32 v;

    memcpy(&v, frame + off, 4);
    return v;
}

static void frame_st32(u8 *frame, u32 off, u32 v)
{
    memcpy(frame + off, &v, 4);
}

static void frame_st64(u8 *frame, u32 off, u32 lo, u32 hi)
{
    frame_st32(frame, off, lo);
    frame_st32(frame, off + 4u, hi);
}

static u32 f_neg(u32 bits)
{
    return bits ^ 0x80000000u;
}

static u32 f32_bits(double v)
{
    return (u32)i960_f64_to_u32(v);
}

/* TGP 0x58 length → scale component by 1.6 (0x3fd99999_9999999a). */
static u32 tgp_scale_axis(u32 x, u32 y, u32 z, u32 *out_bits)
{
    double len;
    double scaled;

    i960_mmio_write_u32(0x884000, 0x2c005858u);
    i960_mmio_write_u32(0x884000, 0);
    i960_mmio_write_u32(0x884000, 0);
    i960_mmio_write_u32(0x884000, 0);
    i960_mmio_write_u32(0x884000, x);
    i960_mmio_write_u32(0x884000, y);
    i960_mmio_write_u32(0x884000, z);
    len = i960_u32_to_f64(i960_ld_u32(I960_MMIO, 0x884000, 0));
    if (len == 0.0)
        len = 1.0;
    scaled = i960_u32_to_f64(*out_bits) / len;
    scaled *= i960_rifl_read(0x9999999au, 0x3fd99999u);
    *out_bits = (u32)i960_f64_to_u32(scaled);
    return *out_bits;
}

void game_start_race_cam_seed(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    u8 frame[0x150];
    u32 course;
    u32 cam;
    u32 row;
    u32 i;
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    if (!logged) {
        lift_log( "lift: race_cam_seed\n");
        fflush(stderr);
        logged = 1;
    }

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    sp = sp + 0x130;

    /* @0x1F1E4–0x1F254: nine float quads → 0x80(fp)..0x10C. */
    for (i = 0; i < 9u; i++) {
        u32 base = 0x005be140u + i * 0x10u;
        u32 dst = 0x80u + i * 0x10u;

        frame_st64(frame, dst,
                   i960_ld_u32(I960_WORKRAM, base, 0),
                   i960_ld_u32(I960_WORKRAM, base, 4));
        frame_st64(frame, dst + 8u,
                   i960_ld_u32(I960_WORKRAM, base, 8),
                   i960_ld_u32(I960_WORKRAM, base, 12));
    }

    /* Odd 0x2020a8 skips the course-row copy / normalize. */
    if ((i960_ld_u8(I960_WORKRAM, 0x2020a8, 0) & 1u) != 0u)
        goto seed_tail;

    course = i960_host_race_course_index();
    cam = i960_ld_u32(I960_WORKRAM, 0x2140cc, 0);
    if (cam == 0u
        || (model2_ram_mut(cam) == NULL && model2_rom_at(cam) == NULL)) {
        fp = fp_save;
        sp = sp_save;
        return;
    }

    /* course*9 word index into the float rows at 0x80(fp). */
    row = course * 9u;

    /* @0x1F25C–0x1F284: row[0..2] → cam+0x14 / +0x1c.
     * lda 0x40(fp)[row] + ldl 0x40(ea) → load from 0x80(fp)+row*4. */
    {
        u32 ea = 0x40u + row * 4u;
        u32 lo = frame_u32(frame, ea + 0x40u);
        u32 hi = frame_u32(frame, ea + 0x44u);
        u32 z = frame_u32(frame, ea + 0x48u);

        i960_st_u32(I960_ABS, cam, 0x14, lo);
        i960_st_u32(I960_ABS, cam, 0x18, hi);
        i960_st_u32(I960_ABS, cam, 0x1c, z);
    }

    /* @0x1F288–0x1F2AC: row[3..5] → cam+0x38 / +0x40. */
    {
        u32 ea = 0x8cu + row * 4u;
        u32 lo = frame_u32(frame, ea);
        u32 hi = frame_u32(frame, ea + 4u);
        u32 z = frame_u32(frame, ea + 8u);

        i960_st_u32(I960_ABS, cam, 0x38, lo);
        i960_st_u32(I960_ABS, cam, 0x3c, hi);
        i960_st_u32(I960_ABS, cam, 0x40, z);
    }

    /* @0x1F2B0–0x1F2E0: row[6..8] → cam+0x2c / +0x30 / +0x34. */
    {
        u32 ea = 0x98u + row * 4u;
        u32 lo = frame_u32(frame, ea);
        u32 hi = frame_u32(frame, ea + 4u);
        u32 z = frame_u32(frame, ea + 8u);

        i960_st_u32(I960_ABS, cam, 0x2c, lo);
        i960_st_u32(I960_ABS, cam, 0x30, hi);
        i960_st_u32(I960_ABS, cam, 0x34, z);
        frame_st64(frame, 0x130, lo, hi);
    }

    /* @0x1F2E4–0x1F460: TGP 0x58 normalize each axis × 1.6. */
    {
        u32 x = i960_ld_u32(I960_ABS, cam, 0x2c);
        u32 y = i960_ld_u32(I960_ABS, cam, 0x30);
        u32 z = i960_ld_u32(I960_ABS, cam, 0x34);

        tgp_scale_axis(x, y, z, &x);
        i960_st_u32(I960_ABS, cam, 0x2c, x);
        tgp_scale_axis(x, y, z, &y);
        i960_st_u32(I960_ABS, cam, 0x30, y);
        tgp_scale_axis(x, y, z, &z);
        i960_st_u32(I960_ABS, cam, 0x34, z);
    }

    /* @0x1F460–0x1F4AC: seed 0x5c7890/98 into cam+0x20..+0x4c; write 0x3e0f5c29
     * into cam+0x64 and three linked +4 nodes. */
    {
        u32 p = cam;
        u32 n;

        i960_st_u32(I960_ABS, cam, 0x28, i960_ld_u32(I960_WORKRAM, 0x5c7898, 0));
        i960_st_u32(I960_ABS, cam, 0x20, i960_ld_u32(I960_WORKRAM, 0x5c7890, 0));
        i960_st_u32(I960_ABS, cam, 0x24, i960_ld_u32(I960_WORKRAM, 0x5c7890, 4));
        i960_st_u32(I960_ABS, cam, 0x44, i960_ld_u32(I960_WORKRAM, 0x5c7890, 0));
        i960_st_u32(I960_ABS, cam, 0x48, i960_ld_u32(I960_WORKRAM, 0x5c7890, 4));
        i960_st_u32(I960_ABS, cam, 0x4c, i960_ld_u32(I960_WORKRAM, 0x5c7898, 0));
        i960_st_u32(I960_ABS, cam, 0x60, 0);
        i960_st_u32(I960_ABS, cam, 0x74, 0);
        for (n = 0; n < 4u; n++) {
            i960_st_u32(I960_ABS, p, 0x64, 0x3e0f5c29u);
            p = i960_ld_u32(I960_ABS, p, 4);
            if (p == 0u)
                break;
        }
    }

seed_tail:
    /*
     * @0x1F4B0–0x1F9E0: shared view-seed tail (odd 0x2020a8 jumps here).
     * Clear bit7 on object list, compose install-time view from t@0x20ab58
     * (yaw/pitch via TGP sin + matrix load 0x24), set eye @ 0x20220c/210/214,
     * bump t by 0.01 into 0x20ab58, gate object_pen via 0x2139d8=1.
     * source: disasm/maincpu/maincpu_01f4b0_550.asm
     */
    {
        u32 count = i960_ld_u32(I960_WORKRAM, 0x213978, 0);
        u32 cam;
        u32 t_bits;
        double t_val;
        double two_t;
        double pitch_d;
        u32 yaw_f;
        u32 two_t_f;
        u32 sin_bits;
        u32 pitch_f;
        u32 i;
        u32 px, py, pz;
        u32 eye_x, eye_y, eye_z;
        u32 mtx[12];
        u32 pen_nibble;

        if ((i32)count >= 1) {
            u32 list = 0x00213984u;
            u32 n;

            for (n = 1u; (i32)n < (i32)count; n++) {
                u32 obj = i960_ld_u32(I960_WORKRAM, list, 0);
                u8 b;

                list += 4u;
                if (obj == 0u)
                    continue;
                b = i960_ld_u8(I960_ABS, obj + 0x50u, 0);
                i960_st_u8(I960_ABS, obj + 0x50u, 0, (u8)(b & 0x7fu));
            }
        }

        t_bits = i960_ld_u32(I960_WORKRAM, 0x20ab58, 0);
        t_val = i960_u32_to_f64(t_bits);
        two_t = t_val + t_val;
        /* float(-(t + π)) as yaw for 0x2a; float(2t) as sin input. */
        yaw_f = f32_bits(-(t_val + i960_rifl_read(0x54442d18u, 0x400921fbu)));
        two_t_f = f32_bits(two_t);

        /* @0x1F528–0x1F570: push, yaw(-(t+π)), sin(2t). */
        i960_mmio_write_u32(0x884000, 0x10002020u);
        i960_mmio_write_u32(0x884000, 0x15002a2au);
        i960_mmio_write_u32(0x884000, yaw_f);
        i960_mmio_write_u32(0x884000, 0x0a801515u);
        i960_mmio_write_u32(0x884000, two_t_f);
        sin_bits = i960_ld_u32(I960_MMIO, 0x884000, 0);

        /* pitch = -((sin * 0.1) + ~0.15915) */
        pitch_d = i960_u32_to_f64(sin_bits);
        pitch_d *= i960_rifl_read(0x9999999au, 0x3fb99999u);
        pitch_d += i960_rifl_read(0x769cf0e0u, 0x3fc41b2fu);
        pitch_f = f32_bits(-pitch_d);

        /* @0x1F5B8–0x1F684: pitch + T(0,0,4) + xform seed point; pop. */
        frame_st64(frame, 0x110,
                   i960_ld_u32(I960_WORKRAM, 0x5c7890, 0),
                   i960_ld_u32(I960_WORKRAM, 0x5c7890, 4));
        frame_st32(frame, 0x118, i960_ld_u32(I960_WORKRAM, 0x5c7898, 0));

        i960_mmio_write_u32(0x884000, 0x14802929u);
        i960_mmio_write_u32(0x884000, pitch_f);
        i960_mmio_write_u32(0x884000, 0x13802727u);
        i960_mmio_write_u32(0x884000, 0u);
        i960_mmio_write_u32(0x884000, 0u);
        i960_mmio_write_u32(0x884000, 0x40600000u); /* 4.0f */
        i960_mmio_write_u32(0x884000, 0x16002c2cu);
        i960_mmio_write_u32(0x884000, frame_u32(frame, 0x110));
        i960_mmio_write_u32(0x884000, frame_u32(frame, 0x114));
        i960_mmio_write_u32(0x884000, frame_u32(frame, 0x118));
        frame_st32(frame, 0x120, i960_ld_u32(I960_MMIO, 0x884000, 0));
        frame_st32(frame, 0x124, i960_ld_u32(I960_MMIO, 0x884000, 0));
        frame_st32(frame, 0x128, i960_ld_u32(I960_MMIO, 0x884000, 0));
        i960_mmio_write_u32(0x884000, 0x10802121u);

        cam = i960_ld_u32(I960_WORKRAM, 0x2140cc, 0);
        if (cam == 0u
            || (model2_ram_mut(cam) == NULL && model2_rom_at(cam) == NULL)) {
            i960_st_u32(I960_WORKRAM, 0x2139d8, 0, 1u);
            fp = fp_save;
            sp = sp_save;
            return;
        }

        /* eye.x = cam.x − pt.x */
        px = frame_u32(frame, 0x120);
        py = frame_u32(frame, 0x124);
        pz = frame_u32(frame, 0x128);
        eye_x = f32_bits(i960_u32_to_f64(i960_ld_u32(I960_ABS, cam, 0x14))
                         - i960_u32_to_f64(px));
        i960_st_u32(I960_WORKRAM, 0x20220c, 0, eye_x);

        /* eye.y = cam.y − pt.y; if cam+0x10 nibble == 0, add 0.7 */
        pen_nibble = i960_ld_u8(I960_ABS, cam, 0x10) & 0x0fu;
        eye_y = f32_bits(i960_u32_to_f64(i960_ld_u32(I960_ABS, cam, 0x18))
                         - i960_u32_to_f64(py));
        if (pen_nibble == 0u)
            eye_y = f32_bits(i960_u32_to_f64(eye_y)
                             + i960_rifl_read(0x66666666u, 0x3fe66666u));
        i960_st_u32(I960_WORKRAM, 0x202210, 0, eye_y);

        /* @0x1F700–0x1F820: sin(2t) → pitch/yaw compose + xform (−√½,−√½,+√½).
         * eye.z uses first-xform z (g2 from ldq @ 0x120); 0x2c feed goes to PRG. */
        {
            u32 ang2 = f32_bits(two_t);
            u32 sin2;
            double a;

            i960_mmio_write_u32(0x884000, 0x0a801515u);
            i960_mmio_write_u32(0x884000, ang2);
            sin2 = i960_ld_u32(I960_MMIO, 0x884000, 0);
            a = i960_u32_to_f64(sin2);
            a *= i960_rifl_read(0x9999999au, 0x3fb99999u);
            a += i960_rifl_read(0x769cf0e0u, 0x3fc41b2fu);
            i960_mmio_write_u32(0x884000, 0x14802929u);
            i960_mmio_write_u32(0x884000, f32_bits(a));
            i960_mmio_write_u32(0x884000, 0x15002a2au);
            i960_mmio_write_u32(0x884000,
                                f32_bits(t_val + i960_rifl_read(0x54442d18u, 0x400921fbu)));
            i960_mmio_write_u32(0x884000, 0x16002c2cu);
            i960_mmio_write_u32(0x884000, 0xbf34fdf4u);
            i960_mmio_write_u32(0x884000, 0xbf34fdf4u);
            i960_mmio_write_u32(0x884000, 0x3f34fdf4u);
        }

        eye_z = f32_bits(i960_u32_to_f64(i960_ld_u32(I960_ABS, cam, 0x1c))
                         - i960_u32_to_f64(pz));
        i960_st_u32(I960_WORKRAM, 0x202214, 0, eye_z);

        /* Store xform readback @ 0x70(fp) then copy to 0x140 for PRG push. */
        frame_st32(frame, 0x70, i960_ld_u32(I960_MMIO, 0x884000, 0));
        frame_st32(frame, 0x74, i960_ld_u32(I960_MMIO, 0x884000, 0));
        frame_st32(frame, 0x78, i960_ld_u32(I960_MMIO, 0x884000, 0));
        frame_st64(frame, 0x140, frame_u32(frame, 0x70), frame_u32(frame, 0x74));
        frame_st32(frame, 0x148, frame_u32(frame, 0x78));

        /* GEO reg poke + three PRG floats (install-time seed marker). */
        i960_mmio_write_u32(0x8000a0, 0u);
        i960_mmio_write_u32(0x804000, frame_u32(frame, 0x140));
        i960_mmio_write_u32(0x804000, frame_u32(frame, 0x144));
        i960_mmio_write_u32(0x804000, frame_u32(frame, 0x148));

        /* @0x1F870–0x1F910: 0x26 matrix → 0x40(fp)..0x6c. */
        i960_mmio_write_u32(0x884000, 0x13002626u);
        for (i = 0; i < 12u; i++) {
            mtx[i] = i960_ld_u32(I960_MMIO, 0x884000, 0);
            frame_st32(frame, 0x40u + i * 4u, mtx[i]);
        }

        /* @0x1F914–0x1F9C8: I + load matrix 0x24 + 0x27(−eye); bump t. */
        i960_mmio_write_u32(0x884000, 0x12802525u);
        i960_mmio_write_u32(0x884000, 0x12002424u);
        for (i = 0; i < 12u; i++)
            i960_mmio_write_u32(0x884000, frame_u32(frame, 0x40u + i * 4u));
        i960_mmio_write_u32(0x884000, 0x13802727u);
        i960_mmio_write_u32(0x884000, f_neg(i960_ld_u32(I960_WORKRAM, 0x20220c, 0)));
        i960_mmio_write_u32(0x884000, f_neg(i960_ld_u32(I960_WORKRAM, 0x202210, 0)));
        i960_mmio_write_u32(0x884000, f_neg(i960_ld_u32(I960_WORKRAM, 0x202214, 0)));

        i960_st_u32(I960_WORKRAM, 0x2139d8, 0, 1u);
        i960_st_u32(I960_WORKRAM, 0x20ab58, 0,
                    f32_bits(t_val + i960_rifl_read(0x47ae147bu, 0x3f847ae1u)));
        {
            static int eye_logged;

            if (!eye_logged) {
                lift_log(
                        "lift: race_cam_seed cam=%#x eye=(%.3g,%.3g,%.3g) t=%.3g\n",
                        cam,
                        i960_u32_to_f64(eye_x),
                        i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, 0x202210, 0)),
                        i960_u32_to_f64(eye_z),
                        t_val + 0.01);
                fflush(stderr);
                eye_logged = 1;
            }
        }
    }

    fp = fp_save;
    sp = sp_save;
}
