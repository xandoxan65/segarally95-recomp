/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_05ce18_a0.asm */
// @rom 0x5ce18 +0xa0 libc_scanf_setup

#include "i960_lift.h"
#include "i960_fp.h"
#include "model2_memory.h"
#include "model2_rom.h"

/* convention: kind=leaf_bx  args g0,g1,g2  link g14→g3 bx */
/* abi: void * arg0=g0, void * arg1=g1, u32 arg2=g2 → void * g0 */

static u8 scan_byte(const void *base, u32 off)
{
    const u8 *host = (const u8 *)base + off;
    u32 va = (u32)(uintptr_t)base + off;

    if (model2_main_data_rom &&
        (uintptr_t)host >= (uintptr_t)model2_main_data_rom &&
        (uintptr_t)host < (uintptr_t)model2_main_data_rom + model2_main_data_size)
        return host[0];

    if ((uintptr_t)base >= (uintptr_t)model2_workram &&
        (uintptr_t)base < (uintptr_t)model2_workram + WORKRAM_SIZE)
        return host[0];

    if (va >= MAIN_DATA_A) {
        const u8 *rom = model2_rom_at(va);

        return rom ? rom[0] : 0;
    }

    if (va >= WORKRAM_BASE && va < WORKRAM_BASE + WORKRAM_SIZE)
        return model2_workram_mirror_u8(va);

    return host[0];
}

void * libc_scanf_setup(void * arg0, void * arg1, u32 arg2)
{
    u32 s_off = 0;
    u32 p_off = 0;
    u8 s0;
    u8 p0;

    g3 = g14;
    g14 = 0;
    g2 = arg2 - 1u;
    g6 = (uintptr_t)arg0;
    g7 = (uintptr_t)arg1;
    if ((signed)g2 < 0)
        goto L_0005ce64;
    s0 = scan_byte(arg0, s_off);
    if (s0 == 0)
        goto L_0005ce64;
    p0 = scan_byte(arg1, p_off);
    if (p0 != s0)
        goto L_0005ce64;

    L_0005ce40:
        g2 = g2 - 1u;
        s_off = s_off + 1u;
        p_off = p_off + 1u;
        if ((signed)g2 < 0)
            goto L_0005ce64;
        s0 = scan_byte(arg0, s_off);
        if (s0 == 0)
            goto L_0005ce64;
        p0 = scan_byte(arg1, p_off);
        if (p0 == s0)
            goto L_0005ce40;

    L_0005ce64:
        if ((signed char)(u8)g2 > 0)
            goto L_0005ce70;
        g0 = 0;
        return (void *)(uintptr_t)g0;

    L_0005ce70:
        s0 = scan_byte(arg0, s_off);
        if (s0 != 0)
            goto L_0005ce88;
        p0 = scan_byte(arg1, p_off);
        if (p0 != 0)
            goto L_0005ce88;
        g0 = 0;
        return (void *)(uintptr_t)g0;

    L_0005ce88:
        s0 = scan_byte(arg0, s_off);
        if (s0 != 0)
            goto L_0005ce98;
        g0 = 0u - 1u;
        return (void *)(uintptr_t)g0;

    L_0005ce98:
        p0 = scan_byte(arg1, p_off);
        if (p0 != 0)
            goto L_0005cea8;
        g0 = 1;
        return (void *)(uintptr_t)g0;

    L_0005cea8:
        g0 = s0;
        g4 = p0;
        g0 = g0 - g4;
        return (void *)(uintptr_t)g0;
}
