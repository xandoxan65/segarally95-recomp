/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_05cfb8_14.asm */
// @rom 0x5cfb8 +0x14 libc_printf_format_loop

#include "i960_lift.h"
#include "i960_fp.h"
#include "cgm_format.h"
#include "i960_host_invoke.h"
#include "i960_host_staging.h"
#include "model2_memory.h"
#include "model2_rom.h"
#include "i960_mem.h"

extern void boot_tile_opcode_dispatch(u32 arg0);
extern void libc_printf_outer_loop(void);

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
    return *(const u8 *)(uintptr_t)cursor;
}

#define CGM_FORMAT_JUMP_TABLE  0x005fbfd0u
#define CGM_HANDLER_NOP_WR     0x005fc9b4u
#define CGM_HANDLER_NOP_ROM    0x0005d9b4u

void libc_printf_format_loop(void)
{
    for (;;) {
        u32 handler_wr;
        u32 handler_rom;

        r12 = r12 + 1u;
        g4 = fmt_byte(r12);
        if ((signed char)(u8)g4 > (signed char)(u8)g9)
            goto L_0005d9b4;
        handler_wr = i960_ld_u32(I960_WORKRAM, CGM_FORMAT_JUMP_TABLE,
                                 (u32)((u8)g4 << 2));
        handler_rom = i960_host_resolve_call_target(handler_wr);
        if (handler_wr == CGM_HANDLER_NOP_WR || handler_rom == CGM_HANDLER_NOP_ROM)
            goto L_0005d9b4;
        if (i960_host_invoke_lifted(handler_rom))
            continue;
        if (handler_rom != 0u)
            i960_call_rom(handler_rom);
    }

    L_0005d9b4:
        g0 = fmt_byte(r12);
        r11 = r11 + 1u;
        boot_tile_opcode_dispatch((u32)g0);
        r12 = r12 + 1u;
        libc_printf_outer_loop();
}
