/* Car road-contact solver @ 0x2F8A0.
 *
 * Four AABB corners → local (fp+0xa0) + world (fp+0x40). Bit7 walls run
 * N×local, then (cross)×N → normalize → 0x59(local) scale into per-corner
 * force at fp+0x70+i*12. Post-loop reduces XZ force + yaw torque into
 * node+0x20/28/48 and predicted target X/Z.
 *
 * source: disasm/maincpu/maincpu_02f8a0_c50.asm */
// @rom 0x2f8a0 +0x894 game_start_race_obj_car_contact_solve

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "i960_host.h"
#include "lift_syms.h"

#include <stdio.h>
#include <string.h>

static u32 contact_ld(const u8 *p, u32 off)
{
    u32 value;

    memcpy(&value, p + off, 4);
    return value;
}

static void contact_st(u8 *p, u32 off, u32 value)
{
    memcpy(p + off, &value, 4);
}

static u32 contact_f32(double value)
{
    return (u32)i960_f64_to_u32(value);
}

static double contact_f64(u32 value)
{
    return i960_u32_to_f64(value);
}

static u32 contact_mul(u32 a, u32 b)
{
    return contact_f32(contact_f64(a) * contact_f64(b));
}

static void contact_cross_5b(u32 ay, u32 bz, u32 az, u32 by, u32 bx, u32 ax,
                             u32 out[3])
{
    i960_mmio_write_u32(0x884000u, 0x2d805b5bu);
    i960_mmio_write_u32(0x884000u, ay);
    i960_mmio_write_u32(0x884000u, bz);
    i960_mmio_write_u32(0x884000u, az);
    i960_mmio_write_u32(0x884000u, by);
    i960_mmio_write_u32(0x884000u, bx);
    i960_mmio_write_u32(0x884000u, ax);
    out[0] = i960_mmio_read_u32(0x884000u);
    out[1] = i960_mmio_read_u32(0x884000u);
    out[2] = i960_mmio_read_u32(0x884000u);
}

u32 game_start_race_obj_car_contact_solve(u32 arg0, void *arg1, void *arg2)
{
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    u8 frame[0x1d0];
    u8 *target = (u8 *)arg1;
    u8 *angles = (u8 *)arg2;
    u32 node = arg0;
    u32 course;
    i32 index;
    u32 max_penetration = 0;
    u32 sum_x = 0;
    u32 sum_z = 0;
    u32 yaw_acc = 0;
    u32 bit7_mask = 0;
    static unsigned contact_logs;
    static unsigned contact_summary;

    if (node == 0u || target == NULL || angles == NULL)
        return 0;
    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    sp += 0x1b0u;

    /* @0x2F8FC–0x2F94C: four 0x42 table rows from 0x5D2E30. */
    for (u32 i = 0; i < 4u; ++i) {
        i960_mmio_write_u32(0x884000u, 0x21004242u);
        i960_mmio_write_u32(0x884000u, i);
        i960_mmio_write_u32(0x884000u,
            i960_ld_u32(I960_WORKRAM, 0x5d2e30u + i * 12u, 0));
        i960_mmio_write_u32(0x884000u,
            i960_ld_u32(I960_WORKRAM, 0x5d2e34u + i * 12u, 0));
        i960_mmio_write_u32(0x884000u,
            i960_ld_u32(I960_WORKRAM, 0x5d2e38u + i * 12u, 0));
    }

    /*
     * @0x2F950–0x2FBF8: rotation-only pose. Locals stay at fp+0xa0+i*12;
     * world probes (local + predicted) at fp+0x40+i*12.
     */
    /*
     * @0x2F950–0x2FBF8: rotation-only pose. g14 must be 0 for the first
     * 0x48 index (ROM st g14); a stale callee link selects the wrong vslot.
     */
    g14 = 0;
    i960_mmio_write_u32(0x884000u, 0x10002020u);
    i960_mmio_write_u32(0x884000u, 0x12802525u);
    i960_mmio_write_u32(0x884000u, 0x15002a2au);
    i960_mmio_write_u32(0x884000u, contact_ld(angles, 4));
    i960_mmio_write_u32(0x884000u, 0x14802929u);
    i960_mmio_write_u32(0x884000u, contact_ld(angles, 0));
    i960_mmio_write_u32(0x884000u, 0x15802b2bu);
    i960_mmio_write_u32(0x884000u, contact_ld(angles, 8));
    {
        u32 pred_x = contact_ld(target, 0);
        u32 pred_y = contact_ld(target, 4);
        u32 pred_z = contact_ld(target, 8);

        for (u32 i = 0; i < 4u; ++i) {
            u32 lx, ly, lz;
            u32 off = i * 12u;

            i960_mmio_write_u32(0x884000u, 0x24004848u);
            i960_mmio_write_u32(0x884000u, i);
            i960_mmio_write_u32(0x884000u, 0x21804343u);
            i960_mmio_write_u32(0x884000u, i);
            lx = i960_mmio_read_u32(0x884000u);
            ly = i960_mmio_read_u32(0x884000u);
            lz = i960_mmio_read_u32(0x884000u);
            contact_st(frame, 0xa0u + off, lx);
            contact_st(frame, 0xa4u + off, ly);
            contact_st(frame, 0xa8u + off, lz);
            contact_st(frame, 0x40u + off,
                       contact_f32(contact_f64(lx) + contact_f64(pred_x)));
            contact_st(frame, 0x44u + off,
                       contact_f32(contact_f64(ly) + contact_f64(pred_y)));
            contact_st(frame, 0x48u + off,
                       contact_f32(contact_f64(lz) + contact_f64(pred_z)));
        }
    }
    i960_mmio_write_u32(0x884000u, 0x10802121u);

    /* @0x2FC0C–0x2FF38: per-corner bit7 wall impulse into fp+0x70+i*12. */
    course = i960_host_race_course_index();
    index = (i32)(int16_t)i960_ld_u16(I960_ABS, node, 0x88);
    for (u32 i = 0; i < 4u; ++i) {
        u8 sample[0xe0];
        u32 probe[3];
        u32 local[3];
        u32 nx, ny, nz;
        u32 flag;
        u32 penetration;
        u32 cross1[3];
        u32 cross2[3];
        u32 unit[3];
        u32 dot;
        u32 scale;
        u32 mass;
        u32 iy;
        u32 force_off = 0x70u + i * 12u;
        u32 off = i * 12u;

        memset(sample, 0, sizeof(sample));
        probe[0] = contact_ld(frame, 0x40u + off);
        probe[1] = contact_ld(frame, 0x44u + off);
        probe[2] = contact_ld(frame, 0x48u + off);
        local[0] = contact_ld(frame, 0xa0u + off);
        local[1] = contact_ld(frame, 0xa4u + off);
        local[2] = contact_ld(frame, 0xa8u + off);
        g3 = 0;
        geo_view_table_index_a((void *)(uintptr_t)course, (u32)index, probe);
        geo_view_table_index_b(probe, sample, 0);

        flag = sample[0x1c];
        penetration = contact_ld(sample, 0x18);
        if ((flag & 0x80u) != 0u)
            bit7_mask |= 1u << i;
        if ((flag & 0x80u) == 0u || !(contact_f64(penetration) > 0.0)) {
            /* @0x2FF14: clear this corner's force triple. */
            contact_st(frame, force_off, 0);
            contact_st(frame, force_off + 4u, 0);
            contact_st(frame, force_off + 8u, 0);
            continue;
        }

        nx = contact_ld(sample, 0x0c);
        ny = contact_ld(sample, 0x10);
        nz = contact_ld(sample, 0x14);

        /*
         * @0x2FCB4–0x2FD08: 0x5b with A=N, B=local
         * FIFO (Ay,Bz,Az,By,Bx,Ax) = (Ny,lz,Nz,ly,lx,Nx).
         */
        contact_cross_5b(ny, local[2], nz, local[1], local[0], nx, cross1);

        /*
         * @0x2FD34–0x2FDAC: second 0x5b A=cross1, B=N
         * (Ay,Bz,Az,By,Bx,Ax) = (c1y,Nz,c1z,Ny,Nx,c1x).
         */
        contact_cross_5b(cross1[1], nz, cross1[2], ny, nx, cross1[0], cross2);

        /* @0x2FDB0–0x2FE04: bare 0x2f normalize. */
        i960_mmio_write_u32(0x884000u, 0x17802f2fu);
        i960_mmio_write_u32(0x884000u, cross2[0]);
        i960_mmio_write_u32(0x884000u, cross2[1]);
        i960_mmio_write_u32(0x884000u, cross2[2]);
        unit[0] = i960_mmio_read_u32(0x884000u);
        unit[1] = i960_mmio_read_u32(0x884000u);
        unit[2] = i960_mmio_read_u32(0x884000u);

        /* @0x2FE08–0x2FE68: 0x59 unit·local. */
        i960_mmio_write_u32(0x884000u, 0x2c805959u);
        i960_mmio_write_u32(0x884000u, unit[0]);
        i960_mmio_write_u32(0x884000u, local[0]);
        i960_mmio_write_u32(0x884000u, unit[1]);
        i960_mmio_write_u32(0x884000u, local[1]);
        i960_mmio_write_u32(0x884000u, unit[2]);
        i960_mmio_write_u32(0x884000u, local[2]);
        dot = i960_mmio_read_u32(0x884000u);

        /*
         * @0x2FE64–0x2FEB8 (mulrl 0.5 × mass×iy, divrl by denom):
         *   denom = mass*dot*dot + iy
         *   scale = (0.5 * mass * iy) / denom
         * then force = scale * penetration * N.
         * Prior lift used (0.5*iy)/denom — mass≈1000 (road_attach) made
         * wall impulses ~1000× too weak so the car drove through fences.
         */
        mass = i960_ld_u32(I960_ABS, node, 0x50);
        iy = i960_ld_u32(I960_ABS, node, 0x58);
        {
            u32 m_d = contact_mul(mass, dot);
            u32 m_d2 = contact_mul(m_d, dot);
            u32 denom = contact_f32(contact_f64(m_d2) + contact_f64(iy));
            u32 half_m_iy = contact_f32(
                contact_f64(mass) * contact_f64(iy) * 0.5);

            if (contact_f64(denom) == 0.0)
                scale = 0;
            else
                scale = contact_f32(
                    contact_f64(half_m_iy) / contact_f64(denom));
        }
        /* @0x2FEB4–0x2FEF8: force = scale * penetration * N (not unit). */
        scale = contact_mul(scale, penetration);
        contact_st(frame, force_off, contact_mul(scale, nx));
        contact_st(frame, force_off + 4u, contact_mul(scale, ny));
        contact_st(frame, force_off + 8u, contact_mul(scale, nz));

        if (contact_f64(penetration) > contact_f64(max_penetration))
            max_penetration = penetration;

        if (contact_logs < 24u) {
            lift_log(
                    "lift: contact_solve i=%u pen=%.4g N=(%.3g,%.3g,%.3g) "
                    "F=(%.3g,%.3g,%.3g) scale=%.4g\n",
                    (unsigned)i, contact_f64(penetration),
                    contact_f64(nx), contact_f64(ny), contact_f64(nz),
                    contact_f64(contact_ld(frame, force_off)),
                    contact_f64(contact_ld(frame, force_off + 4u)),
                    contact_f64(contact_ld(frame, force_off + 8u)),
                    contact_f64(scale));
            fflush(stderr);
            contact_logs++;
        }
    }

    /* @0x2FF3C: no positive penetration → ret 0. */
    if (!(contact_f64(max_penetration) > 0.0)) {
        if (contact_summary < 16u
            || (bit7_mask != 0u && (contact_summary % 30u) == 0u)) {
            lift_log(
                    "lift: contact_solve no-impulse idx=%d bit7=%#x "
                    "pred_y=%.4g\n",
                    (int)index, (unsigned)bit7_mask,
                    contact_f64(contact_ld(target, 4)));
            fflush(stderr);
        }
        contact_summary++;
        fp = fp_save;
        sp = sp_save;
        return 0;
    }

    /*
     * @0x2FF58–0x300AC: per-corner horizontal 0x5b (local.xz × force.xz)
     * accumulates |torque| into yaw_acc and force X/Z into sum_*.
     */
    for (u32 i = 0; i < 4u; ++i) {
        u32 off = i * 12u;
        u32 fx = contact_ld(frame, 0x70u + off);
        u32 fy = contact_ld(frame, 0x74u + off);
        u32 fz = contact_ld(frame, 0x78u + off);
        u32 lx = contact_ld(frame, 0xa0u + off);
        u32 lz = contact_ld(frame, 0xa8u + off);
        u32 cross[3];
        u32 len;

        (void)fy;
        /* Zero local.y / force.y lanes as ROM does before the reduce 0x5b. */
        contact_cross_5b(0, fz, lz, 0, fx, lx, cross);

        i960_mmio_write_u32(0x884000u, 0x2c005858u);
        i960_mmio_write_u32(0x884000u, 0);
        i960_mmio_write_u32(0x884000u, 0);
        i960_mmio_write_u32(0x884000u, 0);
        i960_mmio_write_u32(0x884000u, cross[0]);
        i960_mmio_write_u32(0x884000u, cross[1]);
        i960_mmio_write_u32(0x884000u, cross[2]);
        len = i960_mmio_read_u32(0x884000u);
        if (contact_f64(cross[1]) < 0.0)
            len ^= 0x80000000u;
        yaw_acc = contact_f32(contact_f64(yaw_acc) - contact_f64(len));
        sum_x = contact_f32(contact_f64(sum_x) + contact_f64(fx));
        sum_z = contact_f32(contact_f64(sum_z) + contact_f64(fz));
    }

    /*
     * @0x300B0–0x30118: pending_x += sum_x/mass, pending_z += sum_z/mass,
     * yaw_rate += yaw_acc/inertia. Integrate entry then applies 0.5×pending
     * to vel+pos and clears. No same-frame hard-sep / vel kill — those
     * host paths stacked on the mass-scaled force and launched the car
     * (log: pen=0.195 F≈88 → unrecovered rebound).
     */
    {
        double mass = contact_f64(i960_ld_u32(I960_ABS, node, 0x50));
        double inertia = contact_f64(i960_ld_u32(I960_ABS, node, 0x58));

        if (mass != 0.0) {
            u32 dx = contact_f32(contact_f64(sum_x) / mass);
            u32 dz = contact_f32(contact_f64(sum_z) / mass);

            i960_st_u32(I960_ABS, node, 0x20,
                contact_f32(contact_f64(i960_ld_u32(I960_ABS, node, 0x20))
                            + contact_f64(dx)));
            i960_st_u32(I960_ABS, node, 0x28,
                contact_f32(contact_f64(i960_ld_u32(I960_ABS, node, 0x28))
                            + contact_f64(dz)));
        }
        if (inertia != 0.0) {
            u32 dyaw = contact_f32(contact_f64(yaw_acc) / inertia);

            i960_st_u32(I960_ABS, node, 0x48,
                contact_f32(contact_f64(i960_ld_u32(I960_ABS, node, 0x48))
                            + contact_f64(dyaw)));
        }
        /* @0x300E8–0x30118: target.xz = node.pos + node.vel (unmodified). */
        contact_st(target, 0, contact_f32(
            contact_f64(i960_ld_u32(I960_ABS, node, 0x14))
            + contact_f64(i960_ld_u32(I960_ABS, node, 0x2c))));
        contact_st(target, 8, contact_f32(
            contact_f64(i960_ld_u32(I960_ABS, node, 0x1c))
            + contact_f64(i960_ld_u32(I960_ABS, node, 0x34))));
    }

    {
        static unsigned contact_hit_logs;

        /* Always log first 64, then every 16th — crash tunnels were invisible
         * after the early-run cap while force_slot y<-2 flooded the terminal. */
        if (contact_hit_logs < 64u || (contact_hit_logs % 16u) == 0u) {
            lift_log(
                    "lift: contact_solve hit pen=%.4g sumXZ=(%.3g,%.3g) "
                    "yaw=%.3g\n",
                    contact_f64(max_penetration),
                    contact_f64(sum_x), contact_f64(sum_z),
                    contact_f64(yaw_acc));
            fflush(stderr);
        }
        contact_hit_logs++;
    }

    fp = fp_save;
    sp = sp_save;
    return max_penetration;
}
