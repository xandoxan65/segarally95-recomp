/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_05cf50_7c.asm */
// @rom 0x5cf50 +0x80 libc_printf_dispatch

#include "i960_lift.h"
#include "i960_fp.h"
#include "cgm_format.h"
#include "model2_memory.h"
#include "model2_rom.h"
#include "i960_mem.h"

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret  callee r12,r5 */
/* abi: void * arg0=g0, u32 arg1=g1, u32 arg2=g2 → void * g0 */

extern void boot_tile_opcode_dispatch(u32 arg0);
extern void libc_printf_format_loop(void);

static u8 fmt_byte(uintptr_t cursor)
{
    u32 va = (u32)cursor;

    if (va >= CGM_STAGE_BASE && va < CGM_STAGE_BASE + 0x2000u)
        return i960_ld_u8(I960_WORKRAM, va, 0);
    if (va >= MAIN_DATA_A) {
        const u8 *rom = model2_rom_at(va);

        return rom ? rom[0] : 0;
    }
    if (va >= WORKRAM_BASE && va < WORKRAM_BASE + WORKRAM_SIZE)
        return model2_workram_mirror_u8(va);
    /* Host pointer only when outside guest maps (rare). */
    if (cursor > (uintptr_t)0x01000000u)
        return *(const u8 *)(uintptr_t)cursor;
    return 0;
}

void libc_printf_outer_loop(void)
{
    for (;;) {
        g0 = fmt_byte(r12);
        if ((unsigned char)g0 == 0)
            return;
        g11 = 31u + 6u;
        if ((unsigned char)g11 == (unsigned char)g0) {
            r9 = 0;
            r3 = 0;
            r15 = 0;
            r14 = 0;
            r7 = 0u - 1u;
            r13 = 0;
            cgm_printf_spec_open((u32)r5);
            libc_printf_format_loop();
            cgm_printf_spec_close();
            return;
        }
        boot_tile_opcode_dispatch((u32)g0);
        r11 = r11 + 1u;
        r12 = r12 + 1u;
    }
}

void * libc_printf_dispatch(void * arg0, u32 arg1, u32 arg2)
{
    uintptr_t fmt_va;
    uintptr_t sp_save = sp;
    u64 g8_save;

    (void)arg2;
    /*
     * ROM does lda 0x180(sp),sp for a callee frame. Host must restore — the
     * boot stack is only host_workram[0x2000]; unreclaimed +0x180 per call
     * walks off the end and SIGBUS on the next *(u64 *)(sp-0x10).
     */
    sp = sp + 0x180;
    g8_save = (u64)g8;
    r5 = (uintptr_t)arg1;
    /*
     * libc_printf often calls with an empty prototype; prefer guest VA in g0
     * (set by libc_printf) over a possibly-clobbered C arg0.
     */
    fmt_va = (u32)g0;
    if (fmt_va == 0 && arg0)
        fmt_va = (uintptr_t)arg0;
    r12 = fmt_va;
    g8 = (uintptr_t)(model2_workram + 0xfbf10);
    r11 = 0;
    g9 = 15u << 3;

    libc_printf_outer_loop();
    g0 = r11;
    g8 = (u32)g8_save;
    sp = sp_save;
    return (void *)(uintptr_t)g0;
}
