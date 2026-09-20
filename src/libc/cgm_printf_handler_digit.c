/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_05d304_8.asm */
// @rom 0x5d304 +0x8 cgm_printf_handler_digit

#include "i960_lift.h"
#include "i960_fp.h"
#include "cgm_format.h"
#include "model2_memory.h"
#include "model2_rom.h"

extern void libc_printf_format_loop(void);

static u8 fmt_byte(uintptr_t cursor)
{
    u32 va = (u32)cursor;

    if (va >= MAIN_DATA_A) {
        const u8 *rom = model2_rom_at(va);

        return rom ? rom[0] : 0;
    }
    if (va >= WORKRAM_BASE && va < WORKRAM_BASE + WORKRAM_SIZE)
        return model2_workram_mirror_u8(va);
    return *(const u8 *)(uintptr_t)cursor;
}

void cgm_printf_handler_digit(u32 arg0, u32 arg1, u32 arg2)
{
    u8 digit_ch;

    (void)arg0;
    (void)arg1;
    (void)arg2;
    digit_ch = fmt_byte(r12);
    cgm_printf_digit(digit_ch);
    libc_printf_format_loop();
}
