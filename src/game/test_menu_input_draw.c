/* INPUT TEST row draw @ 0x61E0 — chute/shift/VR/analog status lines.
 * source: disasm/maincpu/maincpu_0061e0_220.asm */
// @rom 0x61e0 +0x204 test_menu_input_draw

#include "i960_lift.h"
#include "i960_mem.h"

#include "lift_syms.h"

static void printf_guest(u32 fmt_va)
{
    uintptr_t fp_save = fp;

    fp = 0;
    libc_printf((const char *)(uintptr_t)fmt_va, (u32)g1, (u32)g2);
    fp = fp_save;
}

static void on_off_line(u32 fmt_va, int on)
{
    g1 = on ? 0x5a5018u : 0x5a501cu;
    printf_guest(fmt_va);
}

void test_menu_input_draw(u32 arg0, u32 arg1, u32 arg2)
{
    u32 in0;
    u32 shift;

    /* Caller sets g0/g1 cursor; first insn is bal tile_cursor_seed. */
    tile_cursor_seed(arg0, arg1);
    (void)arg2;

    in0 = i960_ld_u8(I960_WORKRAM, 0x20205c, 0);
    on_off_line(0x5a5020u, (int)(in0 & 1u));
    on_off_line(0x5a5040u, (int)((in0 >> 1) & 1u));

    shift = i960_ld_u32(I960_WORKRAM, 0x202044, 0);
    on_off_line(0x5a5060u, shift == 1u);
    on_off_line(0x5a5080u, shift == 2u);
    on_off_line(0x5a50a0u, shift == 3u);
    on_off_line(0x5a50c0u, shift == 4u);

    on_off_line(0x5a50e0u, (int)((in0 >> 7) & 1u));
    on_off_line(0x5a5100u, (int)((in0 >> 4) & 1u));
    on_off_line(0x5a5120u, (int)((in0 >> 2) & 1u));
    on_off_line(0x5a5140u, (int)((in0 >> 3) & 1u));

    g1 = i960_ld_u8(I960_WORKRAM, 0x202050, 0) & 0xffu;
    printf_guest(0x5a5160u);
    g1 = i960_ld_u8(I960_WORKRAM, 0x202051, 0) & 0xffu;
    printf_guest(0x5a5180u);
    g1 = i960_ld_u8(I960_WORKRAM, 0x202052, 0) & 0xffu;
    printf_guest(0x5a51a0u);
    g1 = i960_ld_u8(I960_WORKRAM, 0x202048, 0) & 0xffu;
    printf_guest(0x5a51c0u);
}
