/* Car pose/contact integration @ 0x2CCE0.
 *
 * Practice-path lift: prologue, both nested calls, unconditional TGP/road
 * force core, published XYZ/angle integration, damping, and angle clamps.
 *
 * source: disasm/maincpu/maincpu_02cce0_196c.asm
 */
// @rom 0x2cce0 +0x196c game_start_race_obj_car_integrate

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "i960_host.h"

#include "lift_syms.h"

#include <stdio.h>
#include <string.h>

u32 game_start_race_obj_car_contact_solve(u32 arg0, void *arg1, void *arg2);
void game_start_race_obj_car_velocity_step(u32 arg0, u32 arg1, u32 arg2);

static u32 integrate_f32(double value)
{
    return (u32)i960_f64_to_u32(value);
}

static double integrate_f64(u32 bits)
{
    return i960_u32_to_f64(bits);
}

static u32 integrate_add(u32 a, u32 b)
{
    double s = integrate_f64(a) + integrate_f64(b);

    /* i960 invalid-op leaves the dest (IEEE would store NaN). */
    if (!isfinite(s))
        return a;
    return integrate_f32(s);
}

static u32 integrate_sub(u32 a, u32 b)
{
    double s = integrate_f64(a) - integrate_f64(b);

    if (!isfinite(s))
        return a;
    return integrate_f32(s);
}

static u32 integrate_mul(u32 a, u32 b)
{
    double s = integrate_f64(a) * integrate_f64(b);

    /* i960 invalid-op leaves the dest. */
    if (!isfinite(s))
        return a;
    return integrate_f32(s);
}

static u32 integrate_fneg(u32 bits)
{
    return bits ^ 0x80000000u;
}

static void integrate_frame_st(u8 *frame, u32 off, u32 value)
{
    memcpy(frame + off, &value, sizeof(value));
}

static u32 integrate_frame_ld(const u8 *frame, u32 off)
{
    u32 value;

    memcpy(&value, frame + off, sizeof(value));
    return value;
}

static void integrate_cmd(u32 marker)
{
    i960_mmio_write_u32(0x884000u, marker);
}

static void integrate_vec3(u32 marker, const u32 in[3], u32 out[3])
{
    integrate_cmd(marker);
    i960_mmio_write_u32(0x884000u, in[0]);
    i960_mmio_write_u32(0x884000u, in[1]);
    i960_mmio_write_u32(0x884000u, in[2]);
    out[0] = i960_mmio_read_u32(0x884000u);
    out[1] = i960_mmio_read_u32(0x884000u);
    out[2] = i960_mmio_read_u32(0x884000u);
}

/*
 * XZ clamp leaf @ 0x30380 — called only from index_refresh @ 0x30320.
 * 0x30380 is a `call` (`addo 16,sp`); `st 0x40(fp)` is the callee frame,
 * not integrate's predicted corners at caller fp+0x40. Force samples keep
 * predicted[] from the pose 0x2c burst (lda 0x40(r6)[fp] after 0x30320).
 * Transforms (x,0,z) through push/0x61 (load list slot *0x20a290)/0x2c; if |x'| > *(0x20cae0),
 * rewrites x/z at *xyz_va. source: maincpu_02f8a0_c50.asm @ 0x30380.
 */
static void integrate_xz_clamp(u32 xyz_va)
{
    u32 cursor;
    u32 x, z;
    u32 xp, yp, zp;
    u32 ax;
    u32 thresh;
    u32 scratch[3];

    cursor = i960_ld_u32(I960_WORKRAM, 0x20a290u, 0);
    integrate_cmd(0x10002020u);
    i960_st_u32(I960_ABS, cursor, 0, 0);
    integrate_cmd(0x30806161u);
    x = i960_ld_u32(I960_ABS, xyz_va, 0);
    z = i960_ld_u32(I960_ABS, xyz_va, 8);
    {
        u32 v[3] = { x, 0, z };
        u32 out[3];

        integrate_vec3(0x16002c2cu, v, out);
        xp = out[0];
        yp = out[1];
        zp = out[2];
    }
    scratch[0] = xp;
    scratch[1] = yp;
    scratch[2] = zp;
    i960_st_u32(I960_WORKRAM, 0x20a290u, 0, cursor + 4u);

    ax = xp;
    if (integrate_f64(xp) < 0.0)
        ax = xp ^ 0x80000000u;
    thresh = i960_ld_u32(I960_WORKRAM, 0x20cae0u, 0);
    if (integrate_f64(ax) > integrate_f64(thresh)) {
        u32 lim = thresh;

        if (integrate_f64(xp) < 0.0)
            lim = thresh ^ 0x80000000u;
        scratch[0] = lim;
        cursor = i960_ld_u32(I960_WORKRAM, 0x20a290u, 0);
        i960_st_u32(I960_ABS, cursor, 0, 16u);
        integrate_cmd(0x30806161u);
        {
            u32 out[3];

            integrate_vec3(0x16002c2cu, scratch, out);
            i960_st_u32(I960_ABS, xyz_va, 0, out[0]);
            scratch[1] = out[1];
            i960_st_u32(I960_ABS, xyz_va, 8, out[2]);
            i960_st_u32(I960_WORKRAM, 0x20a290u, 0, cursor + 4u);
        }
    }
    integrate_cmd(0x10802121u);
}

/*
 * Road-index refresh @ 0x30320.
 * ABI: g0=node, g1=predicted XYZ VA (fp+0x70), g2=&node+0x88.
 *
 * @0x3033C/0x3035C: ldis; cmpible 0,g4 → ret when g4 >= 0 (MAME: branch
 * if src1<=src2). Pack hit (index 0..n, including desert cell 0) keeps the
 * new index and unconstrained predicted XYZ. Pack miss stores 0xffff (−1):
 * fall through restores prior index, xz-clamps, packs again; still miss →
 * snap predicted to node+0x90/94/98 and restore index.
 *
 * Prior lift used packed<=0 as the miss gate (treated cell 0 as fail) and
 * snapped on index>0 — opposite of the ROM, which pulled cars onto garbage
 * span snaps (log: snap=1e8) during the desert zoom.
 */
static void integrate_index_refresh(u32 node, u8 *frame)
{
    u32 xyz_va = node + 0x14u;
    u32 index_va = node + 0x88u;
    u32 save_xyz[3];
    u32 save_index;
    i32 packed;
    static unsigned refresh_logs;

    save_xyz[0] = i960_ld_u32(I960_ABS, xyz_va, 0);
    save_xyz[1] = i960_ld_u32(I960_ABS, xyz_va, 4);
    save_xyz[2] = i960_ld_u32(I960_ABS, xyz_va, 8);
    i960_st_u32(I960_ABS, xyz_va, 0, integrate_frame_ld(frame, 0x70));
    i960_st_u32(I960_ABS, xyz_va, 4, integrate_frame_ld(frame, 0x74));
    i960_st_u32(I960_ABS, xyz_va, 8, integrate_frame_ld(frame, 0x78));

    /* @0x30330: ld (g2),r7 — save index word before pack. */
    save_index = i960_ld_u32(I960_ABS, index_va, 0);
    geo_view_slot_word_pack(xyz_va, index_va, 0);
    /* @0x3033C: ldis (r5),g4 */
    packed = (i32)(int16_t)i960_ld_u16(I960_ABS, index_va, 0);
    /* @0x30340: cmpible 0,g4 → ret when packed >= 0 (keep predicted). */
    if (packed >= 0) {
        if (refresh_logs < 8u) {
            lift_log(
                    "lift: index_refresh pack1 hit idx=%d "
                    "keep xyz=(%.3g,%.3g,%.3g)\n",
                    (int)packed,
                    integrate_f64(i960_ld_u32(I960_ABS, xyz_va, 0)),
                    integrate_f64(i960_ld_u32(I960_ABS, xyz_va, 4)),
                    integrate_f64(i960_ld_u32(I960_ABS, xyz_va, 8)));
            fflush(stderr);
            refresh_logs++;
        }
        goto done_pose;
    }

    if (refresh_logs < 8u) {
        lift_log(
                "lift: index_refresh pack1 miss idx=%d "
                "xyz=(%.3g,%.3g,%.3g)\n",
                (int)packed,
                integrate_f64(i960_ld_u32(I960_ABS, xyz_va, 0)),
                integrate_f64(i960_ld_u32(I960_ABS, xyz_va, 4)),
                integrate_f64(i960_ld_u32(I960_ABS, xyz_va, 8)));
        fflush(stderr);
        refresh_logs++;
    }

    /* @0x30344: miss → restore prior index, xz-clamp @ 0x30380. */
    i960_st_u32(I960_ABS, index_va, 0, save_index);
    integrate_xz_clamp(xyz_va);

    geo_view_slot_word_pack(xyz_va, index_va, 0);
    packed = (i32)(int16_t)i960_ld_u16(I960_ABS, index_va, 0);
    /* @0x3035C: cmpible 0,g4 → ret when pack2 hit. */
    if (packed >= 0) {
        if (refresh_logs < 8u) {
            lift_log(
                    "lift: index_refresh pack2 hit idx=%d\n",
                    (int)packed);
            fflush(stderr);
            refresh_logs++;
        }
        goto done_pose;
    }

    /* @0x30360–0x30378: still miss → snap to span XYZ, restore index. */
    i960_st_u32(I960_ABS, xyz_va, 0, i960_ld_u32(I960_ABS, node, 0x90));
    i960_st_u32(I960_ABS, xyz_va, 4, i960_ld_u32(I960_ABS, node, 0x94));
    i960_st_u32(I960_ABS, xyz_va, 8, i960_ld_u32(I960_ABS, node, 0x98));
    i960_st_u32(I960_ABS, index_va, 0, save_index);

    if (refresh_logs < 8u) {
        lift_log(
                "lift: index_refresh snap keep_idx=%d "
                "snap=(%.3g,%.3g,%.3g)\n",
                (int)(int16_t)(save_index & 0xffffu),
                integrate_f64(i960_ld_u32(I960_ABS, xyz_va, 0)),
                integrate_f64(i960_ld_u32(I960_ABS, xyz_va, 4)),
                integrate_f64(i960_ld_u32(I960_ABS, xyz_va, 8)));
        fflush(stderr);
        refresh_logs++;
    }

done_pose:
    integrate_frame_st(frame, 0x70, i960_ld_u32(I960_ABS, xyz_va, 0));
    integrate_frame_st(frame, 0x74, i960_ld_u32(I960_ABS, xyz_va, 4));
    integrate_frame_st(frame, 0x78, i960_ld_u32(I960_ABS, xyz_va, 8));
    /* Restore published pose at node+0x14 (proxy buffer only). */
    i960_st_u32(I960_ABS, xyz_va, 0, save_xyz[0]);
    i960_st_u32(I960_ABS, xyz_va, 4, save_xyz[1]);
    i960_st_u32(I960_ABS, xyz_va, 8, save_xyz[2]);
}

static u32 integrate_distance(const u32 v[3])
{
    integrate_cmd(0x2c005858u);
    i960_mmio_write_u32(0x884000u, 0);
    i960_mmio_write_u32(0x884000u, 0);
    i960_mmio_write_u32(0x884000u, 0);
    i960_mmio_write_u32(0x884000u, v[0]);
    i960_mmio_write_u32(0x884000u, v[1]);
    i960_mmio_write_u32(0x884000u, v[2]);
    return i960_mmio_read_u32(0x884000u);
}

static void integrate_pose_matrix(u32 xyz[3], u32 pitch, u32 yaw, u32 roll)
{
    integrate_cmd(0x12802525u);
    integrate_cmd(0x13802727u);
    i960_mmio_write_u32(0x884000u, xyz[0]);
    i960_mmio_write_u32(0x884000u, xyz[1]);
    i960_mmio_write_u32(0x884000u, xyz[2]);
    integrate_cmd(0x15002a2au);
    i960_mmio_write_u32(0x884000u, yaw);
    integrate_cmd(0x14802929u);
    i960_mmio_write_u32(0x884000u, pitch);
    integrate_cmd(0x15802b2bu);
    i960_mmio_write_u32(0x884000u, roll);
}

/*
 * Inline semantic lift of 0x30140. Callee `lda 0x40(sp),sp` then
 * `lda 0x40(fp),g1` → table_index_b. Sequential calls reuse the same
 * stack slot, so miss (store only out+0x1c=15) leaves the previous
 * slot/frame plane words. Do not memset this buffer.
 */
static u8 s_force_slot_fp[0xa0];

static u32 integrate_force_slot(
    u32 point[3], u32 scalar_va, u32 meta_va, u32 scale,
    u32 force_out[3], u32 *sample_y, u16 *counter, u32 slot_index)
{
    u8 *sample = s_force_slot_fp;
    u32 plane_x;
    u32 plane_y;
    u32 plane_z;
    u32 plane_w;
    u32 flags;
    u32 depth;
    double response;
    double capped_depth;
    double spring;
    double damping;
    double global_scale = integrate_f64(
        i960_ld_u32(I960_WORKRAM, 0x214124u, 0));
    double depth_scale;
    double damp_scale;

    /*
     * Disasm @ 0x3019C: lda 0x40(fp),g1 → table_index_b out.
     * +0x0c/10/14 normal, +0x18 height, +0x1c flag (fp+0x4c..0x5c).
     */
    geo_view_table_index_b(point, sample + 0x40u, 0);
    plane_x = integrate_frame_ld(sample, 0x50);
    plane_y = integrate_frame_ld(sample, 0x54);
    plane_z = integrate_frame_ld(sample, 0x58);
    plane_w = integrate_frame_ld(sample, 0x4c);
    flags = sample[0x5c];
    depth = plane_z;
    *sample_y = depth;
    /* @0x301B0 / 0x301DC: st height then y+height before the chkbit gate. */
    i960_st_u32(I960_ABS, meta_va, 0, integrate_add(point[1], depth));
    *counter = (u16)(*counter
        | (((u16)(i960_ld_u32(I960_WORKRAM, 0x2141dcu, 0) & 15u))
           << (slot_index * 4u)));

    {
        static unsigned force_slot_logs;
        int apply;
        double h = integrate_f64(depth);
        double ny = integrate_f64(plane_x);

        /*
         * @0x301D8–0x301EC: chkbit 7 → fail; height < 0 → fail.
         * Miss flag 15 has bit7 clear, so leftover height≥0 still springs.
         */
        apply = ((flags & 0x80u) == 0u) && (h >= 0.0);

        if (force_slot_logs < 24u || (force_slot_logs % 60u) == 0u
            || (flags & 0x80u) != 0u || flags == 15u
            || (apply && h > 2.0)) {
            double car_x = 0.0;
            double car_z = 0.0;

            if (slot_index < 4u) {
                u32 node = meta_va - 0xb4u - slot_index * 4u;

                car_x = integrate_f64(i960_ld_u32(I960_ABS, node, 0x14));
                car_z = integrate_f64(i960_ld_u32(I960_ABS, node, 0x1c));
            }
            lift_log(
                    "lift: force_slot i=%u depth=%.4g flag=%#x apply=%d "
                    "Ny=%.3g probe=(%.4g,%.4g,%.4g) carXZ=(%.4g,%.4g)\n",
                    (unsigned)slot_index, integrate_f64(depth),
                    (unsigned)(flags & 0xffu), apply,
                    ny,
                    integrate_f64(point[0]), integrate_f64(point[1]),
                    integrate_f64(point[2]),
                    car_x, car_z);
            fflush(stderr);
        }
        force_slot_logs++;

        if (!apply) {
            i960_st_u32(I960_ABS, scalar_va, 0, 0);
            force_out[0] = 0;
            force_out[1] = 0;
            force_out[2] = 0;
            return 0;
        }
    }

    depth_scale = slot_index <= 1u
                ? integrate_f64(i960_ld_u32(
                      I960_WORKRAM, 0x2141d4u, 0))
                : integrate_f64(i960_ld_u32(
                      I960_WORKRAM, 0x2141d8u, 0));
    damp_scale = slot_index <= 1u
               ? integrate_f64(i960_ld_u32(
                     I960_WORKRAM, 0x2141dcu, 0))
               : integrate_f64(i960_ld_u32(
                     I960_WORKRAM, 0x2141e0u, 0));
    capped_depth = integrate_f64(depth);
    spring = capped_depth * depth_scale * 100.0 * global_scale;
    if (capped_depth > 0.2) {
        spring += (capped_depth - 0.2) * integrate_f64(scale);
        capped_depth = 0.2;
    }
    damping = (integrate_f64(i960_ld_u32(I960_ABS, scalar_va, 0))
               - capped_depth)
            * damp_scale * global_scale * 3600.0;
    response = spring - damping;
    if (response < 0.0)
        response = 0.0;

    i960_st_u32(I960_ABS, scalar_va, 0,
                integrate_f32(capped_depth));
    force_out[0] = integrate_mul(integrate_f32(response), plane_w);
    force_out[1] = integrate_mul(integrate_f32(response), plane_x);
    force_out[2] = integrate_mul(integrate_f32(response), plane_y);
    return 1;
}

static u32 integrate_angle_clamp(u32 bits)
{
    u32 wrapped = geo_view_float_clamp(bits, 0, 0);
    double angle = integrate_f64(wrapped);
    const double limit = integrate_f64(0x3ec90fdbu);

    if (fabs(angle) > limit)
        angle = signbit(angle) ? -limit : limit;
    return integrate_f32(angle);
}

/*
 * Unconditional car force/matrix core @0x2CE58–0x2E344.
 *
 * The ROM executes this immediately after 0x2F8A0.  It constructs predicted
 * and current corner points, samples four road slots, derives support vectors,
 * measures horizontal velocity, and rotates linear/angular force terms through
 * the car matrix.  All TGP traffic below follows the disassembly order.
 */
static u32 integrate_force_core(u32 node, u32 bank, u8 *frame)
{
    static const u32 corner[4][3] = {
        { 0xbf228f5cu, 0xbecccccdu, 0x3fa00000u },
        { 0x3f228f5cu, 0xbecccccdu, 0x3fa00000u },
        { 0xbf28f5c3u, 0xbecccccdu, 0xbfa00000u },
        { 0x3f28f5c3u, 0xbecccccdu, 0xbfa00000u },
    };
    u32 predicted[4][3];
    u32 current[4][3];
    u32 force[4][3];
    u32 xyz[3];
    u32 velocity[3];
    u32 unit[3];
    u32 contact_count = 0;
    u32 course;
    i32 road_index;
    u16 slot_counter = 0;
    double side_sum = 0.0;
    double length_sum = 0.0;

    /*
     * Callers keep g14==0; force-core FIFO zeros and the ROM st g14 clears of
     * fp+0xf0/100 at 0x2CF40 rely on that. Reassert at entry.
     */
    g14 = 0;
    memset(force, 0, sizeof(force));

    /* @0x2CE58–0x2D070: predicted pose and four transformed corners. */
    integrate_cmd(0x10002020u);
    xyz[0] = integrate_frame_ld(frame, 0x70);
    xyz[1] = integrate_frame_ld(frame, 0x74);
    xyz[2] = integrate_frame_ld(frame, 0x78);
    integrate_pose_matrix(xyz,
                          integrate_frame_ld(frame, 0xe0),
                          integrate_frame_ld(frame, 0xe4),
                          integrate_frame_ld(frame, 0xe8));
    for (u32 i = 0; i < 4u; ++i) {
        integrate_vec3(0x16002c2cu, corner[i], predicted[i]);
        for (u32 lane = 0; lane < 3u; ++lane)
            integrate_frame_st(frame, 0x40u + i * 12u + lane * 4u,
                               predicted[i][lane]);
    }

    /* @0x2D078–0x2D274: current pose, rear corners, and predicted deltas. */
    xyz[0] = i960_ld_u32(I960_ABS, node, 0x14);
    xyz[1] = i960_ld_u32(I960_ABS, node, 0x18);
    xyz[2] = i960_ld_u32(I960_ABS, node, 0x1c);
    integrate_pose_matrix(xyz,
                          i960_ld_u32(I960_ABS, node, 0x38),
                          i960_ld_u32(I960_ABS, node, 0x3c),
                          i960_ld_u32(I960_ABS, node, 0x40));
    integrate_vec3(0x16002c2cu, corner[2], current[2]);
    integrate_vec3(0x16002c2cu, corner[3], current[3]);
    for (u32 i = 2; i < 4u; ++i) {
        for (u32 lane = 0; lane < 3u; ++lane) {
            u32 delta = integrate_sub(predicted[i][lane], current[i][lane]);

            integrate_frame_st(frame, 0xb0u + (i - 2u) * 12u + lane * 4u,
                               delta);
        }
    }
    integrate_cmd(0x10802121u);

    /*
     * @0x2D294: call 0x30320 — see integrate_index_refresh.
     * Pack ABI: ROM g1 is caller fp+0x70. Host copies that into node+0x14
     * so slot_word_pack has a guest VA, then restores. 0x30380 scratch is
     * the clamp callee frame, not these predicted corners.
     */
    integrate_index_refresh(node, frame);

    /*
     * @0x2D2B8–0x2D34C: four 0x2ABC0/0x30140 iterations. geo_view_table_index_a/b
     * are the already-lifted TGP table operators used by those callees.
     */
    course = i960_host_race_course_index();
    road_index = (i32)(int16_t)i960_ld_u16(I960_ABS, node, 0x88);
    for (u32 i = 0; i < 4u; ++i) {
        u32 sample_y;
        u32 scale = integrate_f32(
            integrate_f64(i960_ld_u32(I960_ABS, node, 0x50)) * 0.25);
        u32 hit;

        g3 = 0;
        geo_view_table_index_a((void *)(uintptr_t)course,
                               (u32)road_index,
                               (void *)predicted[i]);
        hit = integrate_force_slot(
            predicted[i], node + 0x64u + i * 4u,
            node + 0xb4u + i * 4u, scale, force[i], &sample_y,
            &slot_counter, i);
        integrate_frame_st(frame, 0x80u + i * 12u, force[i][0]);
        integrate_frame_st(frame, 0x84u + i * 12u, force[i][1]);
        integrate_frame_st(frame, 0x88u + i * 12u, force[i][2]);
        integrate_frame_st(frame, 0x110u + i * 4u, sample_y);
        contact_count = (contact_count << 1) + hit;
    }
    integrate_frame_st(frame, 0x1a0, contact_count);
    i960_st_u16(I960_ABS, node, 0x12, slot_counter);

    /* @0x2D354–0x2D510: forward vector and four corner side products. */
    integrate_cmd(0x10002020u);
    integrate_cmd(0x12802525u);
    integrate_cmd(0x15002a2au);
    i960_mmio_write_u32(0x884000u, i960_ld_u32(I960_ABS, node, 0x3c));
    {
        const u32 forward[3] = { 0, 0, 0x3f800000u };

        integrate_vec3(0x16002c2cu, forward, unit);
    }
    integrate_frame_st(frame, 0x130, unit[0]);
    integrate_frame_st(frame, 0x134, unit[1]);
    integrate_frame_st(frame, 0x138, unit[2]);
    /*
     * @0x2D3FC–0x2D510: exactly two iterations (g0=0,12) over the
     * rear-corner predicted-current deltas at fp+0xb0/fp+0xbc.
     * Using four absolute world-space corner positions here creates a large
     * false side torque while the car is stationary and winds yaw away from
     * its road-seeded pi orientation.
     */
    for (u32 i = 0; i < 2u; ++i) {
        u32 source[3] = {
            integrate_frame_ld(frame, 0xb0u + i * 12u),
            integrate_frame_ld(frame, 0xb4u + i * 12u),
            integrate_frame_ld(frame, 0xb8u + i * 12u),
        };
        u32 length = integrate_distance(source);

        if (integrate_f64(length)
            > i960_rifl_read(0xd9d7bdbbu, 0x3ddb7cdfu)) {
            u32 corner_unit[3];

            integrate_vec3(0x17802f2fu, source, corner_unit);
            length_sum += integrate_f64(length);
            side_sum += integrate_f64(unit[2]) * integrate_f64(corner_unit[0])
                      - integrate_f64(unit[0]) * integrate_f64(corner_unit[2]);
        }
    }

    /*
     * @0x2D514–0x2D6CC: velocity magnitude, heading error, d0/d4 publish.
     * ROM keeps the 0x2D354 push through 0x2DA40 — velocity 0x2f is inside
     * that frame. Host 0x2f also orients R; popping before this call left
     * look-along R on the parent view so later 0x05 T compounded to 1e29/NaN.
     */
    velocity[0] = i960_ld_u32(I960_ABS, node, 0x2c);
    velocity[1] = i960_ld_u32(I960_ABS, node, 0x30);
    velocity[2] = i960_ld_u32(I960_ABS, node, 0x34);
    {
        u32 speed = integrate_distance(velocity);

        if (integrate_f64(speed)
            > i960_rifl_read(0x47ae147bu, 0x3f847ae1u)) {
            double cross;
            double angle;
            u32 old_d0;

            integrate_vec3(0x17802f2fu, velocity, unit);
            cross = integrate_f64(integrate_frame_ld(frame, 0x138))
                        * integrate_f64(unit[0])
                  - integrate_f64(integrate_frame_ld(frame, 0x130))
                        * integrate_f64(unit[2]);
            if (fabs(cross) > 1.0)
                angle = 0.0;
            else
                angle = asin(cross);
            if (fabs(angle) > 1.5707963267948966)
                angle = signbit(angle) ? -1.5707963267948966
                                       : 1.5707963267948966;
            old_d0 = i960_ld_u32(I960_ABS, node, 0xd0);
            i960_st_u32(I960_ABS, node, 0xd0, integrate_f32(angle));
            i960_st_u32(I960_ABS, node, 0xd4,
                        integrate_sub(integrate_f32(angle), old_d0));
        } else {
            i960_st_u32(I960_ABS, node, 0xd0, 0);
            i960_st_u32(I960_ABS, node, 0xd4, 0);
        }
    }

    /*
     * @0x2D6CC–0x2DA40: side_sum → fp+0x104, first lateral TGP pass, then
     * pop. fp+0xf0/f4/f8 and fp+0x100/104/108 were zeroed with g14 at
     * 0x2CF40 (host frame memset). This block is the only pre-pop writer of
     * fp+0x104; 0x2DA50+ continues from that state.
     */
    {
        double scale = integrate_f64(i960_ld_u32(I960_WORKRAM, 0x214124u, 0));
        double coeff = integrate_f64(i960_ld_u32(I960_WORKRAM, 0x2141c4u, 0));
        double side_pi = side_sum * i960_rifl_read(0x54442d18u, 0x400921fbu);
        u32 idx = i960_ld_u32(I960_ABS, node, 0xe4);
        /* r12 = g1 = span_base (car_frame mov r6,g1). ldob 0xc(r12)[e4*16]. */
        u32 lane = (u32)i960_ld_u8(I960_ABS, bank, 0xcu + idx * 16u);
        u32 table;
        u32 sin_arg;
        u32 sine;
        double gain;
        double product;
        double abs_side_pi;
        u32 dx, dz;
        u32 lat[3];
        u32 lat_n[3];
        u32 dist;
        double mass_half;
        u32 yaw_n;

        lane = (lane >> 2) & 28u;
        table = i960_ld_u32(I960_WORKRAM, 0x5cabe0u + lane, 0);
        gain = scale * 1000.0 * coeff * integrate_f64(table);
        abs_side_pi = fabs(side_pi);
        if (abs_side_pi > i960_rifl_read(0x54442d18u, 0x3ff921fbu))
            sin_arg = 0x3fc90fdbu;
        else if (side_pi < 0.0)
            sin_arg = integrate_f32(-side_pi);
        else
            sin_arg = integrate_f32(side_pi);
        integrate_cmd(0x0a801515u);
        i960_mmio_write_u32(0x884000u, sin_arg);
        sine = i960_mmio_read_u32(0x884000u);
        product = gain * integrate_f64(sine);
        if (side_sum >= 0.0)
            product = -product;
        {
            double abs_side = fabs(side_sum);
            double mixed = length_sum * abs_side;
            double floor = integrate_f64(0x3ca3d70au);

            /*
             * @0x2D818: cmpr mixed, 0.02; bge skip;
             *   divr 0.02, mixed → mixed/0.02; mulr into product.
             * Tiny mixed scales yaw torque down. The host previously inverted
             * this to 0.02/mixed, which exploded yaw to 1e23 then NaN 0x2a.
             */
            if (mixed < floor)
                product *= mixed / floor;
        }
        integrate_frame_st(
            frame, 0x104,
            integrate_f32(integrate_f64(integrate_frame_ld(frame, 0x104))
                          + product * 1.25));

        /* @0x2D860–0x2DA3C: lateral velocity through yaw, adjust fp+0x104. */
        dx = integrate_sub(i960_ld_u32(I960_ABS, node, 0x2c),
                           i960_ld_u32(I960_ABS, node, 0xc4));
        dz = integrate_sub(i960_ld_u32(I960_ABS, node, 0x34),
                           i960_ld_u32(I960_ABS, node, 0xcc));
        integrate_frame_st(frame, 0x150, dx);
        integrate_frame_st(frame, 0x154, 0);
        integrate_frame_st(frame, 0x158, dz);
        {
            u32 v[3] = { dx, 0, dz };

            dist = integrate_distance(v);
            integrate_vec3(0x17802f2fu, v, lat_n);
        }
        integrate_frame_st(frame, 0x150, lat_n[0]);
        integrate_frame_st(frame, 0x154, lat_n[1]);
        integrate_frame_st(frame, 0x158, lat_n[2]);
        mass_half = integrate_f64(i960_ld_u32(I960_ABS, node, 0x50)) * 0.5
                  * integrate_f64(dist);
        lat[0] = integrate_f32(mass_half * integrate_f64(lat_n[0]));
        lat[1] = lat_n[1];
        lat[2] = integrate_f32(mass_half * integrate_f64(lat_n[2]));
        integrate_frame_st(frame, 0x150, lat[0]);
        integrate_frame_st(frame, 0x158, lat[2]);
        integrate_cmd(0x12802525u);
        yaw_n = integrate_fneg(i960_ld_u32(I960_ABS, node, 0x3c));
        integrate_cmd(0x15002a2au);
        i960_mmio_write_u32(0x884000u, yaw_n);
        integrate_vec3(0x16002c2cu, lat, lat_n);
        integrate_frame_st(frame, 0x150, lat_n[0]);
        integrate_frame_st(frame, 0x154, lat_n[1]);
        integrate_frame_st(frame, 0x158, lat_n[2]);
        integrate_frame_st(
            frame, 0x104,
            integrate_f32(integrate_f64(integrate_frame_ld(frame, 0x104))
                          - integrate_f64(lat_n[0]) * 1.25));
    }
    /* ROM pop @ 0x2DA40. */
    integrate_cmd(0x10802121u);

    /*
     * @0x2DA50–0x2E174: second lateral pass into fp+0x100/108, sample_y
     * gates into fp+0x108, then push/0x25/negated roll/pitch/yaw and 0x2c of
     * the four force corners at fp+0x80.. into torque/linear accumulators.
     */
    {
        u32 dx, dz;
        u32 lat[3];
        u32 lat_n[3];
        u32 yaw_n;
        u32 mass;
        double k02 = i960_rifl_read(0x9999999au, 0x3fc99999u);
        double k125 = 1.25;
        double sample_scale;
        u32 tf[4][3];
        u32 raw_x[4], raw_y[4], raw_z[4];
        double t100, t104, t108;
        double f0, f1, f2;
        double c0y_c1y, c2y_c3y;
        double c0x_c1x, c2x_c3x;
        double dy01, dy23;
        double dz01, dz23;
        double k0635 = i960_rifl_read(0x851eb852u, 0x3fe451ebu);
        double k066 = i960_rifl_read(0x51eb851fu, 0x3fe51eb8u);

        integrate_cmd(0x10002020u);
        dx = integrate_sub(i960_ld_u32(I960_ABS, node, 0x2c),
                           i960_ld_u32(I960_ABS, node, 0xc4));
        dz = integrate_sub(i960_ld_u32(I960_ABS, node, 0x34),
                           i960_ld_u32(I960_ABS, node, 0xcc));
        integrate_frame_st(frame, 0x150, dx);
        integrate_frame_st(frame, 0x158, dz);
        integrate_cmd(0x12802525u);
        yaw_n = integrate_fneg(i960_ld_u32(I960_ABS, node, 0x3c));
        integrate_cmd(0x15002a2au);
        i960_mmio_write_u32(0x884000u, yaw_n);
        lat[0] = dx;
        lat[1] = 0;
        lat[2] = dz;
        integrate_vec3(0x16002c2cu, lat, lat_n);
        integrate_frame_st(frame, 0x150, lat_n[0]);
        integrate_frame_st(frame, 0x154, lat_n[1]);
        integrate_frame_st(frame, 0x158, lat_n[2]);
        mass = i960_ld_u32(I960_ABS, node, 0x50);
        t100 = integrate_f64(integrate_frame_ld(frame, 0x100))
             + integrate_f64(lat_n[2]) * integrate_f64(mass) * k02;
        t108 = integrate_f64(integrate_frame_ld(frame, 0x108))
             - integrate_f64(lat_n[0]) * integrate_f64(mass) * k02;
        integrate_frame_st(frame, 0x100, integrate_f32(t100));
        integrate_frame_st(frame, 0x108, integrate_f32(t108));

        sample_scale =
            integrate_f64(i960_ld_u32(I960_WORKRAM, 0x2141d4u, 0)) * 100.0
            * integrate_f64(i960_ld_u32(I960_WORKRAM, 0x214124u, 0)) * k125;
        if (integrate_f64(i960_ld_u32(I960_ABS, node, 0x64)) == 0.0
            && integrate_f64(i960_ld_u32(I960_ABS, node, 0x68)) > 0.0) {
            t108 += integrate_f64(integrate_frame_ld(frame, 0x110))
                  * sample_scale;
            integrate_frame_st(frame, 0x108, integrate_f32(t108));
        }
        if (integrate_f64(i960_ld_u32(I960_ABS, node, 0x68)) == 0.0
            && integrate_f64(i960_ld_u32(I960_ABS, node, 0x64)) > 0.0) {
            t108 = integrate_f64(integrate_frame_ld(frame, 0x108))
                 - integrate_f64(integrate_frame_ld(frame, 0x114))
                       * sample_scale;
            integrate_frame_st(frame, 0x108, integrate_f32(t108));
        }

        integrate_cmd(0x10802121u);
        integrate_cmd(0x10002020u);
        integrate_cmd(0x12802525u);
        integrate_cmd(0x15802b2bu);
        i960_mmio_write_u32(
            0x884000u, integrate_fneg(i960_ld_u32(I960_ABS, node, 0x40)));
        integrate_cmd(0x14802929u);
        i960_mmio_write_u32(
            0x884000u, integrate_fneg(i960_ld_u32(I960_ABS, node, 0x38)));
        integrate_cmd(0x15002a2au);
        i960_mmio_write_u32(
            0x884000u, integrate_fneg(i960_ld_u32(I960_ABS, node, 0x3c)));
        for (u32 i = 0; i < 4u; ++i) {
            raw_x[i] = integrate_frame_ld(frame, 0x80u + i * 12u);
            raw_y[i] = integrate_frame_ld(frame, 0x84u + i * 12u);
            raw_z[i] = integrate_frame_ld(frame, 0x88u + i * 12u);
            lat[0] = raw_x[i];
            lat[1] = raw_y[i];
            lat[2] = raw_z[i];
            integrate_vec3(0x16002c2cu, lat, tf[i]);
            integrate_frame_st(frame, 0x160u + i * 12u, tf[i][0]);
            integrate_frame_st(frame, 0x164u + i * 12u, tf[i][1]);
            integrate_frame_st(frame, 0x168u + i * 12u, tf[i][2]);
        }

        t100 = integrate_f64(integrate_frame_ld(frame, 0x100));
        t104 = integrate_f64(integrate_frame_ld(frame, 0x104));
        t108 = integrate_f64(integrate_frame_ld(frame, 0x108));
        f0 = integrate_f64(integrate_frame_ld(frame, 0xf0));
        f1 = integrate_f64(integrate_frame_ld(frame, 0xf4));
        f2 = integrate_f64(integrate_frame_ld(frame, 0xf8));

        /*
         * Disasm @ 0x2DEBC–0x2E174 (subrl = src2−src1):
         *   t100 += 1.25*((c0y+c1y)-(c2y+c3y)) + 0.2*Σcz
         *   t104 += 1.25*((c2x+c3x)-(c0x+c1x)) + 0.635*(c0z-c1z) + 0.66*(c2z-c3z)
         *   t108 += 0.635*(c0y-c1y) + 0.66*(c2y-c3y) - 0.2*Σcx
         * Linear fp+0xf0/f4/f8 accumulates the raw (pre-0x2c) corner forces.
         */
        c0y_c1y = integrate_f64(tf[0][1]) + integrate_f64(tf[1][1]);
        c2y_c3y = integrate_f64(tf[2][1]) + integrate_f64(tf[3][1]);
        c0x_c1x = integrate_f64(tf[0][0]) + integrate_f64(tf[1][0]);
        c2x_c3x = integrate_f64(tf[2][0]) + integrate_f64(tf[3][0]);
        dy01 = integrate_f64(tf[0][1]) - integrate_f64(tf[1][1]);
        dy23 = integrate_f64(tf[2][1]) - integrate_f64(tf[3][1]);
        dz01 = integrate_f64(tf[0][2]) - integrate_f64(tf[1][2]);
        dz23 = integrate_f64(tf[2][2]) - integrate_f64(tf[3][2]);

        t100 += (c0y_c1y - c2y_c3y) * k125
              + (integrate_f64(tf[0][2]) + integrate_f64(tf[1][2])
                 + integrate_f64(tf[2][2]) + integrate_f64(tf[3][2]))
                    * k02;
        t104 += (c2x_c3x - c0x_c1x) * k125 + dz01 * k0635 + dz23 * k066;
        t108 += dy01 * k0635 + dy23 * k066
              - (integrate_f64(tf[0][0]) + integrate_f64(tf[1][0])
                 + integrate_f64(tf[2][0]) + integrate_f64(tf[3][0]))
                    * k02;

        f0 += integrate_f64(raw_x[0]) + integrate_f64(raw_x[1])
            + integrate_f64(raw_x[2]) + integrate_f64(raw_x[3]);
        f1 += integrate_f64(raw_y[0]) + integrate_f64(raw_y[1])
            + integrate_f64(raw_y[2]) + integrate_f64(raw_y[3]);
        f2 += integrate_f64(raw_z[0]) + integrate_f64(raw_z[1])
            + integrate_f64(raw_z[2]) + integrate_f64(raw_z[3]);

        integrate_frame_st(frame, 0xf0, integrate_f32(f0));
        integrate_frame_st(frame, 0xf4, integrate_f32(f1));
        integrate_frame_st(frame, 0xf8, integrate_f32(f2));
        integrate_frame_st(frame, 0x100, integrate_f32(t100));
        integrate_frame_st(frame, 0x104, integrate_f32(t104));
        integrate_frame_st(frame, 0x108, integrate_f32(t108));
        integrate_cmd(0x10802121u);
    }

    /*
     * @0x2E178–0x2E1D0: integrate torque through the three inertia terms.
     * The i960 does fp+0x100/104/108 divided by node+0x54/58/5c before
     * adding the result to angular velocity node+0x44/48/4c.
     */
    {
        static const u32 angular_off[3] = { 0x44u, 0x48u, 0x4cu };
        static const u32 torque_off[3] = { 0x100u, 0x104u, 0x108u };
        static const u32 inertia_off[3] = { 0x54u, 0x58u, 0x5cu };

        for (u32 i = 0; i < 3u; ++i) {
            u32 angular = i960_ld_u32(I960_ABS, node, angular_off[i]);
            double inertia = integrate_f64(
                i960_ld_u32(I960_ABS, node, inertia_off[i]));
            double impulse =
                integrate_f64(integrate_frame_ld(frame, torque_off[i]))
                / inertia;
            double next = integrate_f64(angular) + impulse;

            i960_st_u32(I960_ABS, node, angular_off[i],
                        integrate_f32(next));
        }

        /*
         * @0x2E1D4–0x2E344: when support is not the all-clear value 15,
         * ease roll and pitch angles only if their angular velocity has the
         * same sign. r14 is node+0x40 (roll); the second block uses
         * node+0x38 (pitch). Yaw and angular-velocity fields are not eased.
         */
        if (contact_count != 15u) {
            static const u32 angle_off[2] = { 0x40u, 0x38u };
            static const u32 rate_off[2] = { 0x4cu, 0x44u };

            for (u32 i = 0; i < 2u; ++i) {
                double angle =
                    integrate_f64(i960_ld_u32(I960_ABS, node, angle_off[i]));
                double rate =
                    integrate_f64(i960_ld_u32(I960_ABS, node, rate_off[i]));

                if ((angle < 0.0) == (rate < 0.0)) {
                    angle -= angle * angle * (angle < 0.0 ? -0.3 : 0.3);
                    i960_st_u32(I960_ABS, node, angle_off[i],
                                integrate_f32(angle));
                }
            }
        }
    }

    return contact_count;
}

void game_start_race_obj_car_integrate(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    u8 frame[0x1c0];
    u32 node = arg0;
    u32 vx;
    u32 vy;
    u32 vz;
    u32 contact_result;
    u32 contact_count;
    static int logged;

    (void)arg2;
    if (node == 0u)
        return;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    /* @0x2CCE0: lda 0x1a0(sp),sp; host fp temps extend through 0x1b8. */
    sp += 0x1a0u;

    if (!logged) {
        lift_log( "lift: car_integrate node=%#x\n", node);
        fflush(stderr);
        logged = 1;
    }

    /*
     * @0x2CCF4–0x2CDBC: halve pending X/Z impulses, accumulate both velocity
     * and XYZ, then clear pending +0x20/+0x24/+0x28.
     */
    vx = integrate_mul(i960_ld_u32(I960_ABS, node, 0x20), 0x3f000000u);
    vy = 0;
    vz = integrate_mul(i960_ld_u32(I960_ABS, node, 0x28), 0x3f000000u);
    i960_st_u32(I960_ABS, node, 0x20, vx);
    i960_st_u32(I960_ABS, node, 0x24, 0);
    i960_st_u32(I960_ABS, node, 0x28, vz);

    i960_st_u32(I960_ABS, node, 0x2c,
                integrate_add(i960_ld_u32(I960_ABS, node, 0x2c), vx));
    i960_st_u32(I960_ABS, node, 0x34,
                integrate_add(i960_ld_u32(I960_ABS, node, 0x34), vz));
    i960_st_u32(I960_ABS, node, 0x14,
                integrate_add(i960_ld_u32(I960_ABS, node, 0x14), vx));
    i960_st_u32(I960_ABS, node, 0x18,
                integrate_add(i960_ld_u32(I960_ABS, node, 0x18), vy));
    i960_st_u32(I960_ABS, node, 0x1c,
                integrate_add(i960_ld_u32(I960_ABS, node, 0x1c), vz));
    i960_st_u32(I960_ABS, node, 0x20, 0);
    i960_st_u32(I960_ABS, node, 0x24, 0);
    i960_st_u32(I960_ABS, node, 0x28, 0);

    /* @0x2CDC0–0x2CDD4: ldl pair + scalar snapshot, then 0x30E00. */
    i960_st_u32(I960_ABS, node, 0xc4,
                i960_ld_u32(I960_ABS, node, 0x2c));
    i960_st_u32(I960_ABS, node, 0xc8,
                i960_ld_u32(I960_ABS, node, 0x30));
    i960_st_u32(I960_ABS, node, 0xcc,
                i960_ld_u32(I960_ABS, node, 0x34));
    game_start_race_obj_car_velocity_step(node, 0, 0);

    /*
     * @0x2CDD8–0x2CE50: prepare unconstrained XYZ and angle targets in the
     * private host frame.  stl/ldl semantics are represented as adjacent
     * scalar stores, never as a single host pointer-sized value.
     */
    integrate_frame_st(frame, 0x70,
        integrate_add(i960_ld_u32(I960_ABS, node, 0x14),
                      i960_ld_u32(I960_ABS, node, 0x2c)));
    integrate_frame_st(frame, 0x74,
        integrate_add(i960_ld_u32(I960_ABS, node, 0x18),
                      i960_ld_u32(I960_ABS, node, 0x30)));
    integrate_frame_st(frame, 0x78,
        integrate_add(i960_ld_u32(I960_ABS, node, 0x1c),
                      i960_ld_u32(I960_ABS, node, 0x34)));
    integrate_frame_st(frame, 0xe0,
        integrate_add(i960_ld_u32(I960_ABS, node, 0x38),
                      i960_ld_u32(I960_ABS, node, 0x44)));
    integrate_frame_st(frame, 0xe4,
        integrate_add(i960_ld_u32(I960_ABS, node, 0x3c),
                      i960_ld_u32(I960_ABS, node, 0x48)));
    integrate_frame_st(frame, 0xe8,
        integrate_add(i960_ld_u32(I960_ABS, node, 0x40),
                      i960_ld_u32(I960_ABS, node, 0x4c)));
    contact_result = game_start_race_obj_car_contact_solve(
        node, frame + 0x70, frame + 0xe0);
    integrate_frame_st(frame, 0x190, contact_result);
    contact_count = integrate_force_core(node, arg1, frame);

    /* @0x2E348–0x2E374: damp angular Y velocity from d4/global coefficients. */
    {
        u32 angular_y = i960_ld_u32(I960_ABS, node, 0x48);
        u32 correction = integrate_add(
            integrate_mul(i960_ld_u32(I960_WORKRAM, 0x2141ccu, 0),
                          i960_ld_u32(I960_ABS, node, 0xd4)),
            integrate_mul(i960_ld_u32(I960_WORKRAM, 0x2141d0u, 0),
                          angular_y));

        i960_st_u32(I960_ABS, node, 0x48,
                    integrate_sub(angular_y, correction));
    }

    /* @0x2E378–0x2E3B0: publish Euler angles from angular velocities. */
    i960_st_u32(I960_ABS, node, 0x38,
                integrate_add(i960_ld_u32(I960_ABS, node, 0x38),
                              i960_ld_u32(I960_ABS, node, 0x44)));
    i960_st_u32(I960_ABS, node, 0x3c,
                integrate_add(i960_ld_u32(I960_ABS, node, 0x3c),
                              i960_ld_u32(I960_ABS, node, 0x48)));
    i960_st_u32(I960_ABS, node, 0x40,
                integrate_add(i960_ld_u32(I960_ABS, node, 0x40),
                              i960_ld_u32(I960_ABS, node, 0x4c)));

    /* @0x2E3B4–0x2E470: force/mass into velocity, then publish XYZ. */
    {
        double mass = integrate_f64(i960_ld_u32(I960_ABS, node, 0x50));

        if (mass != 0.0) {
            i960_st_u32(I960_ABS, node, 0x2c,
                integrate_add(i960_ld_u32(I960_ABS, node, 0x2c),
                    integrate_f32(integrate_f64(
                        integrate_frame_ld(frame, 0xf0)) / mass)));
            i960_st_u32(I960_ABS, node, 0x30,
                integrate_add(i960_ld_u32(I960_ABS, node, 0x30),
                    integrate_f32(integrate_f64(
                        integrate_frame_ld(frame, 0xf4)) / mass
                        - integrate_f64(
                            i960_ld_u32(I960_WORKRAM, 0x214124u, 0)))));
            i960_st_u32(I960_ABS, node, 0x34,
                integrate_add(i960_ld_u32(I960_ABS, node, 0x34),
                    integrate_f32(integrate_f64(
                        integrate_frame_ld(frame, 0xf8)) / mass)));
        }
    }
    i960_st_u32(I960_ABS, node, 0x14,
                integrate_add(i960_ld_u32(I960_ABS, node, 0x14),
                              i960_ld_u32(I960_ABS, node, 0x2c)));
    i960_st_u32(I960_ABS, node, 0x18,
                integrate_add(i960_ld_u32(I960_ABS, node, 0x18),
                              i960_ld_u32(I960_ABS, node, 0x30)));
    i960_st_u32(I960_ABS, node, 0x1c,
                integrate_add(i960_ld_u32(I960_ABS, node, 0x1c),
                              i960_ld_u32(I960_ABS, node, 0x34)));

    /* @0x2E474–0x2E55C: contact-result dependent horizontal damping. */
    if (integrate_f64(contact_result) > 0.0) {
        double factor =
            i960_rifl_read(0x8b439581u, 0x3fefe76cu)
            - integrate_f64(contact_result)
                  * integrate_f64(
                      i960_ld_u32(I960_WORKRAM, 0x2141f0u, 0));

        if (factor < 0.0)
            factor = 0.0;
        i960_st_u32(I960_ABS, node, 0x2c,
                    integrate_f32(integrate_f64(
                        i960_ld_u32(I960_ABS, node, 0x2c)) * factor));
        i960_st_u32(I960_ABS, node, 0x30,
                    integrate_f32(integrate_f64(
                        i960_ld_u32(I960_ABS, node, 0x30)) * factor));
        i960_st_u32(I960_ABS, node, 0x34,
                    integrate_f32(integrate_f64(
                        i960_ld_u32(I960_ABS, node, 0x34)) * factor));
    } else {
        double factor = contact_count == 0u
                      ? i960_rifl_read(0x074a771du, 0x3fefffebu)
                      : integrate_f64(i960_ld_u32(
                            I960_WORKRAM, 0x2141ecu, 0));

        i960_st_u32(I960_ABS, node, 0x2c,
                    integrate_f32(integrate_f64(
                        i960_ld_u32(I960_ABS, node, 0x2c)) * factor));
        i960_st_u32(I960_ABS, node, 0x34,
                    integrate_f32(integrate_f64(
                        i960_ld_u32(I960_ABS, node, 0x34)) * factor));
    }

    /* @0x2E560–0x2E638: wrap all angles and cap pitch/roll at ±pi/8. */
    i960_st_u32(I960_ABS, node, 0x38,
                integrate_angle_clamp(i960_ld_u32(I960_ABS, node, 0x38)));
    i960_st_u32(I960_ABS, node, 0x3c,
                geo_view_float_clamp(i960_ld_u32(I960_ABS, node, 0x3c), 0, 0));
    i960_st_u32(I960_ABS, node, 0x40,
                integrate_angle_clamp(i960_ld_u32(I960_ABS, node, 0x40)));

    fp = fp_save;
    sp = sp_save;
}
