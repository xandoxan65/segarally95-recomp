/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_0000ccb0_c0.asm */
// @rom 0xccb0 +0xc0 comm_board_dispatch

#include "i960_lift.h"
#include "model2_rom.h"
#include "i960_mem.h"
#include "i960_host.h"

extern void comm_attract_reset(u32 arg0, u32 arg1, u32 arg2);
extern void comm_draw_setup(u32 arg0, u32 arg1, u32 arg2);

void comm_board_dispatch(u32 arg0, u32 arg1, u32 arg2)
{
    u32 mode5;
    u32 slot_hi;
    u32 slot_lo;
    u32 g5;
    u32 g6;
    u32 poll_count;
    u32 mode;

    g0 = (uintptr_t)arg0;
    g1 = (uintptr_t)arg1;
    g2 = (uintptr_t)arg2;

    g4 = i960_ld_u8(I960_ROM, 0x1a10000, 0);
    if ((unsigned char)g4 != 1) {
        poll_count = i960_ld_u32(I960_WORKRAM, 0x20a754, 0);
        i960_st_u32(I960_WORKRAM, 0x20a758, 0, 1);
        i960_st_u32(I960_WORKRAM, 0x20a754, 0, poll_count + 1);

        if (poll_count >= 10u)
            return;

        /* @0xCD94: ld 20A560; cmpibge 0 → skip st −1 when timer <= 0.
         * Fall through stores −1 when timer > 0 (cancel live session). */
        {
            u32 gate = i960_ld_u32(I960_WORKRAM, 0x20a560, 0);
            if ((int)gate > 0)
                i960_st_u32(I960_WORKRAM, 0x20a560, 0, (u32)-1);
        }

        mode = i960_ld_u32(I960_WORKRAM, 0x202098, 0);
        if (mode != 1 && mode != 2)
            return;

        i960_st_u32(I960_WORKRAM, 0x20a530, 0, (u32)g14);
        comm_attract_reset(0, 0, 0);
        comm_draw_setup(0, 0, 0);
        i960_st_u32(I960_WORKRAM, 0x202098, 0, (u32)g14);
        i960_st_u32(I960_WORKRAM, 0x20209c, 0, (u32)g14);
        return;
    }

    slot_hi = i960_ld_u8(I960_ROM, 0x1a10003, 0) & 0xffu;
    slot_lo = i960_ld_u8(I960_ROM, 0x1a10002, 0) & 0xffu;
    g5 = slot_hi;
    g4 = slot_lo;
    if (g5 >= g4)
        g4 = g5 - g4;
    else
        g4 = 0;
    if (g5 != 0)
        g4 = g4 % g5;
    g5 = g5 - 1;

    i960_st_u32(I960_WORKRAM, 0x20a584, 0, (u32)g5);
    mode5 = i960_ld_u32(I960_WORKRAM, 0x202098, 0);
    i960_st_u32(I960_WORKRAM, 0x20a754, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x20a758, 0, (u32)g14);
    i960_st_u8(I960_WORKRAM, 0x20a581, 0, (u8)slot_hi);
    i960_st_u8(I960_WORKRAM, 0x20a750, 0, (u8)slot_lo);
    i960_st_u32(I960_WORKRAM, 0x20a534, 0, (u32)g4);

    if (mode5 != 2) {
        if (mode5 != 3)
            return;
        g4 = i960_ld_u32(I960_WORKRAM, 0x20a530, 0);
        if (g4 != 3)
            return;
    }

    g5 = i960_ld_u8(I960_WORKRAM, 0x20a581, 0);
    if (g5 == 0)
        return;

    g4 = ((g5 << 3) - g5) << 6;
    g5 = (u32)(uintptr_t)model2_rom_at(0x1a121d8);
    if (!g5)
        return;
    g6 = g5 + g4;

    for (;;) {
        g4 = i960_ld_u32(I960_ABS, g5, 0);
        if (g4 == 1) {
            i960_st_u32(I960_WORKRAM, 0x20a530, 0, (u32)g14);
            comm_attract_reset(0, 0, 0);
            comm_draw_setup(0, 0, 0);
            i960_st_u32(I960_WORKRAM, 0x202098, 0, (u32)g14);
            i960_st_u32(I960_WORKRAM, 0x20209c, 0, (u32)g14);
            return;
        }
        g5 = i960_ld_u32(I960_ABS, g5, 0x1c0);
        if (g5 >= g6)
            return;
    }
}
