/* Host-only skip: attract → desert practice START.
 *
 * Car select @ 0x14820 writes 0x214354 from analog table 5 @ 0x5b3730:
 *   0x80 → idx 2 → 214354=0 (Celica AT)
 *   0xE0 → idx 4 → 214354=2 (Delta AT)
 * Confirm @ 0x15200: brake > 0xB0 and 214354==2 → 214354=3 (Delta MT).
 *
 * Course-select result is 0x20a8c4. --practice never runs that scene, so
 * store desert 0 there. Do not store 0 into 0x214354 after confirm — that
 * cell is still the car class (ROM 0x152DC has no such store).
 *
 * After START, analog rest is 0x80: fov_scale @ 0x395D8 is byte−0x80.
 */

#include "i960_host.h"
#include "i960_mem.h"
#include "model2_rom.h"

#include <stdio.h>

#define SKIP_STEER_DESERT    0x40u
#define SKIP_STEER_DELTA     0xE0u
#define SKIP_BRAKE_MT        0xC0u
#define SKIP_CAR_GATE_FRAMES 12u

static u32 s_car_gate_frames;

static void skip_force_analog(u8 steer, u8 brake)
{
    unsigned i;

    for (i = 0; i < 8u; i++)
        model2_io_analog_set(i, 0);
    model2_io_analog_set(MODEL2_IO_AN_STEER, steer);
    model2_io_analog_set(MODEL2_IO_AN_BRAKE, brake);
    i960_st_u8(I960_WORKRAM, 0x202050, 0, steer);
    i960_st_u8(I960_WORKRAM, 0x202051, 0, 0);
    i960_st_u8(I960_WORKRAM, 0x202052, 0, brake);
}

static void skip_force_delta_at_wheel(void)
{
    skip_force_analog(SKIP_STEER_DESERT, 0);
}

int i960_host_skip_practice_intro_locked(void)
{
    u32 inner;
    u32 scene;

    if (!i960_host_skip_practice())
        return 0;
    if (i960_ld_u32(I960_WORKRAM, 0x202098, 0) != 3u)
        return 0;
    inner = i960_ld_u32(I960_WORKRAM, 0x20209c, 0) & 15u;
    scene = i960_ld_u32(I960_WORKRAM, 0x2020ac, 0) & 15u;
    if (inner != 7u || scene < 4u)
        return 0;
    /* slot2 phase5 @ 0x1D0DC stores 1 when countdown ab54 > 85.625. */
    if ((i32)i960_ld_u32(I960_WORKRAM, 0x214120, 0) > 0)
        return 0;
    return 1;
}

static void skip_force_idle_wheel(void)
{
    unsigned i;

    for (i = 0; i < 8u; i++)
        model2_io_analog_set(i, 0);
    model2_io_analog_set(MODEL2_IO_AN_STEER, 0x80u);
    i960_st_u8(I960_WORKRAM, 0x202050, 0, 0x80u);
    i960_st_u8(I960_WORKRAM, 0x202051, 0, 0);
    i960_st_u8(I960_WORKRAM, 0x202052, 0, 0);
}

void i960_host_skip_practice_idle_wheel(void)
{
    if (!i960_host_skip_practice_intro_locked())
        return;
    skip_force_idle_wheel();
}

static void skip_force_practice_flags(void)
{
    i960_st_u32(I960_WORKRAM, 0x202230, 0, 1u);
    i960_st_u32(I960_WORKRAM, 0x20a8c4, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x2139cc, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x2139d0, 0, 1u);
}

u32 i960_host_race_course_index(void)
{
    if (i960_host_skip_practice())
        return i960_ld_u32(I960_WORKRAM, 0x20a8c4, 0);
    return i960_ld_u32(I960_WORKRAM, 0x214354, 0);
}

void i960_host_practice_force_delta_mt(void)
{
    /* Analog table 5 + 0x15200 brake bump write 214354. Do not stomp. */
}

void i960_host_skip_practice_hold_inputs(void)
{
    u32 mode;
    u32 inner;
    u32 scene;

    if (!i960_host_skip_practice())
        return;

    mode = i960_ld_u32(I960_WORKRAM, 0x202098, 0);
    if (mode == 2u) {
        skip_force_analog(SKIP_STEER_DESERT, 0);
        return;
    }
    if (mode != 3u)
        return;

    skip_force_practice_flags();
    inner = i960_ld_u32(I960_WORKRAM, 0x20209c, 0) & 15u;
    scene = i960_ld_u32(I960_WORKRAM, 0x2020ac, 0) & 15u;
    if (scene >= 4u && inner == 7u) {
        /* record_index uses 0x5db310 vs 0x5db318 from this flag. */
        i960_st_u32(I960_WORKRAM, 0x2020a4, 0, 0);
        if ((i32)i960_ld_u32(I960_WORKRAM, 0x214120, 0) <= 0)
            skip_force_idle_wheel();
        return;
    }
    /* Scene 2 (car_display) + 3 (car_select): analog after I/O sample. */
    if (scene == 2u || scene == 3u) {
        skip_force_analog(SKIP_STEER_DELTA, SKIP_BRAKE_MT);
        if (scene == 3u)
            s_car_gate_frames++;
        if (scene == 2u || s_car_gate_frames <= SKIP_CAR_GATE_FRAMES)
            i960_st_u32(I960_WORKRAM, 0x20a8b8, 0, 61u);
        else
            i960_st_u32(I960_WORKRAM, 0x20a8b8, 0, 0);
        return;
    }
    skip_force_analog(SKIP_STEER_DESERT, 0);
}

void i960_host_skip_practice_tick(void)
{
    u32 mode;
    u32 inner;
    u32 scene;
    u8 flags;
    static int s_logged;
    static int s_started;
    static int s_selects_done;

    if (!i960_host_skip_practice())
        return;

    mode = i960_ld_u32(I960_WORKRAM, 0x202098, 0);

    /* Boot copyright hold @ wait_phase_b (0x1AB84 uses 0x2021f4). */
    if (mode == 0u) {
        if (i960_ld_u32(I960_WORKRAM, 0x20aa20, 0) == 2u)
            i960_st_u32(I960_WORKRAM, 0x2021f4, 0, 0);
        model2_io_in0_set_mask(MODEL2_IO_IN0_START1, 0);
        return;
    }

    /* Attract: one playable credit, or hold START under free-play. */
    if (mode == 2u) {
        skip_force_delta_at_wheel();
        flags = (u8)i960_ld_u8(I960_WORKRAM, 0x202024, 0);
        if ((flags & 8u) != 0) {
            model2_io_in0_set_mask(MODEL2_IO_IN0_START1, 1);
        } else {
            u16 unit = (u16)i960_ld_u16(I960_WORKRAM, 0x202028, 0);
            u16 plays = (u16)i960_ld_u16(I960_ABS, 0x01d00022u, 0);

            if (unit == 0)
                unit = 1;
            if (plays < unit)
                i960_st_u16(I960_ABS, 0x01d00022u, 0, unit);
            model2_io_in0_set_mask(MODEL2_IO_IN0_START1, 0);
        }
        if (!s_logged) {
            fprintf(stderr,
                    "lift: skip-practice — waiting for game-start "
                    "(desert / Delta MT)\n");
            s_logged = 1;
        }
        return;
    }

    model2_io_in0_set_mask(MODEL2_IO_IN0_START1, 0);

    if (mode != 3u)
        return;

    inner = i960_ld_u32(I960_WORKRAM, 0x20209c, 0) & 15u;
    scene = i960_ld_u32(I960_WORKRAM, 0x2020ac, 0) & 15u;

    if (scene >= 4u && inner == 7u) {
        skip_force_practice_flags();
        i960_st_u32(I960_WORKRAM, 0x2020a4, 0, 0);
        if (!s_selects_done) {
            fprintf(stderr,
                    "lift: skip-practice — desert START "
                    "(course 20a8c4=%u car 214354=%u lookup=%u)\n",
                    (unsigned)i960_ld_u32(I960_WORKRAM, 0x20a8c4, 0),
                    (unsigned)i960_ld_u32(I960_WORKRAM, 0x214354, 0),
                    (unsigned)i960_ld_u32(I960_WORKRAM, 0x20a8bc, 0));
            s_selects_done = 1;
            i960_st_u32(I960_WORKRAM, 0x2142c0, 0, 0);
        }
        i960_host_skip_practice_idle_wheel();
        return;
    }

    skip_force_practice_flags();
    if (scene == 2u || scene == 3u) {
        skip_force_analog(SKIP_STEER_DELTA, SKIP_BRAKE_MT);
        if (scene == 3u)
            s_car_gate_frames++;
        if (scene == 2u || s_car_gate_frames <= SKIP_CAR_GATE_FRAMES)
            i960_st_u32(I960_WORKRAM, 0x20a8b8, 0, 61u);
        else
            i960_st_u32(I960_WORKRAM, 0x20a8b8, 0, 0);
    } else {
        skip_force_analog(SKIP_STEER_DESERT, 0);
        i960_st_u32(I960_WORKRAM, 0x20a8b8, 0, 0);
    }

    if (!s_started) {
        fprintf(stderr,
                "lift: skip-practice — practice + desert + Delta MT analog\n");
        s_started = 1;
    }
}
