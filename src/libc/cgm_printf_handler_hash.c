/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_05d1c8_8.asm */
// @rom 0x5d1c8 +0x8 cgm_printf_handler_hash

#include "i960_lift.h"
#include "i960_fp.h"
#include "cgm_format.h"

extern void libc_printf_format_loop(void);

void cgm_printf_handler_hash(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;
    cgm_printf_hash();
    libc_printf_format_loop();
}
