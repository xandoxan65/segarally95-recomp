/* Car velocity-direction / speed leaf @ 0x30E00.
 *
 * Builds a car-local probe through the TGP, normalizes the horizontal
 * direction, applies the selected 0x20cae4/e8 speed response, smooths the
 * old/new direction pair, and publishes node+0x2c/0x34 and node+0xb0.
 *
 * source: disasm/maincpu/maincpu_030e00_400.asm
 */
// @rom 0x30e00 +0x3d8 game_start_race_obj_car_velocity_step

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_hw.h"

#include <string.h>

static u32 velocity_f32(double value)
{
    return (u32)i960_f64_to_u32(value);
}

static double velocity_f64(u32 bits)
{
    return i960_u32_to_f64(bits);
}

static u32 velocity_add(u32 a, u32 b)
{
    return velocity_f32(velocity_f64(a) + velocity_f64(b));
}

static u32 velocity_sub(u32 a, u32 b)
{
    return velocity_f32(velocity_f64(a) - velocity_f64(b));
}

static u32 velocity_mul(u32 a, u32 b)
{
    return velocity_f32(velocity_f64(a) * velocity_f64(b));
}

static void velocity_cmd(u32 marker)
{
    i960_mmio_write_u32(0x884000u, marker);
}

static void velocity_vec3(u32 marker, const u32 in[3], u32 out[3])
{
    velocity_cmd(marker);
    i960_mmio_write_u32(0x884000u, in[0]);
    i960_mmio_write_u32(0x884000u, in[1]);
    i960_mmio_write_u32(0x884000u, in[2]);
    out[0] = i960_mmio_read_u32(0x884000u);
    out[1] = i960_mmio_read_u32(0x884000u);
    out[2] = i960_mmio_read_u32(0x884000u);
}

void game_start_race_obj_car_velocity_step(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    u8 frame[0x60];
    u32 node = arg0;
    u32 probe[3];
    u32 unit[3];
    u32 old_velocity[3];
    u32 old_unit[3];
    u32 speed;
    u32 response;
    u32 distance;
    u32 blend;

    (void)arg1;
    (void)arg2;
    if (node == 0u)
        return;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    sp += 0x30u;

    /* @0x30E0C: inactive scale pair takes the epilogue directly. */
    if (!(velocity_f64(velocity_add(
              i960_ld_u32(I960_ABS, node, 0x64),
              i960_ld_u32(I960_ABS, node, 0x68))) > 0.0))
        goto out;

    /* @0x30E2C–0x30F3C: car pose, then local probe (0,-0.4,1.25). */
    velocity_cmd(0x10002020u);
    velocity_cmd(0x12802525u);
    velocity_cmd(0x13802727u);
    i960_mmio_write_u32(0x884000u, i960_ld_u32(I960_ABS, node, 0x14));
    i960_mmio_write_u32(0x884000u, i960_ld_u32(I960_ABS, node, 0x18));
    i960_mmio_write_u32(0x884000u, i960_ld_u32(I960_ABS, node, 0x1c));
    velocity_cmd(0x15002a2au);
    i960_mmio_write_u32(0x884000u, i960_ld_u32(I960_ABS, node, 0x3c));
    velocity_cmd(0x14802929u);
    i960_mmio_write_u32(0x884000u, i960_ld_u32(I960_ABS, node, 0x38));
    velocity_cmd(0x15802b2bu);
    i960_mmio_write_u32(0x884000u, i960_ld_u32(I960_ABS, node, 0x40));
    {
        const u32 local[3] = { 0, 0xbecccccd, 0x3fa00000u };

        velocity_vec3(0x16002c2cu, local, probe);
    }
    velocity_cmd(0x10802121u);

    /* @0x30F40–0x30FA0: horizontal probe delta and unit direction. */
    {
        u32 delta[3] = {
            velocity_sub(i960_ld_u32(I960_ABS, node, 0x90), probe[0]),
            0,
            velocity_sub(i960_ld_u32(I960_ABS, node, 0x98), probe[2]),
        };

        i960_st_u32(I960_ABS, node, 0x9c, delta[0]);
        i960_st_u32(I960_ABS, node, 0xa0, 0);
        i960_st_u32(I960_ABS, node, 0xa4, delta[2]);
        velocity_vec3(0x17802f2fu, delta, unit);
        i960_st_u32(I960_ABS, node, 0x9c, unit[0]);
        i960_st_u32(I960_ABS, node, 0xa0, unit[1]);
        i960_st_u32(I960_ABS, node, 0xa4, unit[2]);
    }

    /* @0x30FA0–0x310B0: speed target and 0x58 magnitude response. */
    speed = velocity_f32(
        velocity_f64(i960_ld_u32(I960_ABS, node, 0xac))
        * i960_rifl_read(0xd714cf45u, 0x3f72f684u));
    old_velocity[0] = i960_ld_u32(I960_ABS, node, 0x2c);
    old_velocity[1] = 0;
    old_velocity[2] = i960_ld_u32(I960_ABS, node, 0x34);
    if (velocity_f64(speed)
        <= velocity_f64(i960_ld_u32(I960_ABS, node, 0xb0))) {
        u8 flags = i960_ld_u8(I960_ABS, node, 0x10);

        i960_st_u8(I960_ABS, node, 0x10, (u8)(flags & ~(1u << 4)));
        response = i960_ld_u32(I960_WORKRAM, 0x20cae4u, 0);
    } else {
        u8 flags = i960_ld_u8(I960_ABS, node, 0x10);

        i960_st_u8(I960_ABS, node, 0x10, (u8)(flags | (1u << 4)));
        response = i960_ld_u32(I960_WORKRAM, 0x20cae8u, 0);
    }
    velocity_cmd(0x2c005858u);
    i960_mmio_write_u32(0x884000u, 0);
    i960_mmio_write_u32(0x884000u, 0);
    i960_mmio_write_u32(0x884000u, 0);
    i960_mmio_write_u32(0x884000u, old_velocity[0]);
    i960_mmio_write_u32(0x884000u, old_velocity[1]);
    i960_mmio_write_u32(0x884000u, old_velocity[2]);
    distance = i960_mmio_read_u32(0x884000u);
    speed = velocity_sub(speed, velocity_mul(response,
                                             velocity_sub(speed, distance)));
    i960_st_u32(I960_ABS, node, 0xb0, speed);

    /* @0x310B4–0x311C8: direction blend, renormalize, publish X/Z. */
    velocity_vec3(0x17802f2fu, old_velocity, old_unit);
    {
        double dot = velocity_f64(i960_ld_u32(I960_ABS, node, 0x9c))
                         * velocity_f64(old_unit[0])
                   + velocity_f64(i960_ld_u32(I960_ABS, node, 0xa4))
                         * velocity_f64(old_unit[2]);
        double amount = 1.0 - dot;

        if (amount > 1.0)
            amount = 1.0;
        blend = velocity_f32(amount);
        old_unit[0] = velocity_add(
            old_unit[0],
            velocity_mul(velocity_sub(
                i960_ld_u32(I960_ABS, node, 0x9c), old_unit[0]), blend));
        old_unit[2] = velocity_add(
            old_unit[2],
            velocity_mul(velocity_sub(
                i960_ld_u32(I960_ABS, node, 0xa4), old_unit[2]), blend));
    }
    velocity_vec3(0x17802f2fu, old_unit, unit);
    i960_st_u32(I960_ABS, node, 0x2c, velocity_mul(unit[0], speed));
    i960_st_u32(I960_ABS, node, 0x34, velocity_mul(unit[2], speed));

out:
    fp = fp_save;
    sp = sp_save;
}
