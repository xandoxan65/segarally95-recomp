/* INPUT TEST @ 0x6400 — menu slot 1.
 * source: disasm/maincpu/maincpu_006400_300.asm */
// @rom 0x6400 +0xf8 test_menu_input

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>

static void printf_guest(u32 fmt_va)
{
    uintptr_t fp_save = fp;

    fp = 0;
    libc_printf((const char *)(uintptr_t)fmt_va, (u32)g1, (u32)g2);
    fp = fp_save;
}

/* g0!=0: enter; g0==0: frame. Return 1 → back to main test menu. */
u32 test_menu_input(u32 arg0, u32 arg1, u32 arg2)
{
    u32 flags;
    u32 bits;

    (void)arg1;
    (void)arg2;

    if (arg0 != 0) {
        i960_st_u32(I960_WORKRAM, 0x20a2d0, 0, 0);
        i960_st_u8(I960_WORKRAM, 0x202049, 0, 1u);
    }

    tile_cursor_seed(25u, 9u);
    printf_guest(0x5a53f0u);
    g0 = 22;
    g1 = 13;
    test_menu_input_draw((u32)g0, (u32)g1, (u32)g2);
    g0 = 1;
    geo_scene_row_print((u32)g0, (u32)g1, (u32)g2);

    /* TEST edge → arm exit latch */
    flags = i960_ld_u32(I960_WORKRAM, 0x202074, 0);
    if (flags & 1u)
        i960_st_u32(I960_WORKRAM, 0x20a2d0, 0, 1u);
    else {
        flags = i960_ld_u8(I960_WORKRAM, 0x202064, 0);
        if ((flags >> 2) & 1u)
            i960_st_u32(I960_WORKRAM, 0x20a2d0, 0, 1u);
    }

    /* START (or service-start path) with latch → leave */
    flags = i960_ld_u32(I960_WORKRAM, 0x202078, 0);
    if (flags & 1u)
        goto check_latch;
    flags = i960_ld_u8(I960_WORKRAM, 0x202068, 0);
    if ((flags >> 2) & 1u)
        goto check_latch;
    goto check_combo;

check_latch:
    if (i960_ld_u32(I960_WORKRAM, 0x20a2d0, 0) != 0) {
        g0 = 1;
        return 1;
    }

check_combo:
    /* Packed IN0/IN1 nibble at 0x20205c/5d — any set → leave */
    flags = i960_ld_u8(I960_WORKRAM, 0x20205c, 0);
    if (!((flags >> 4) & 1u)) {
        g0 = 0;
        return 0;
    }
    bits = ((flags >> 5) & 1u)
         | ((flags >> 5) & 2u)
         | ((flags >> 5) & 4u);
    bits |= (i960_ld_u8(I960_WORKRAM, 0x20205d, 0) & 1u) << 3;
    if (bits != 0) {
        g0 = 1;
        return 1;
    }
    g0 = 0;
    return 0;
}
