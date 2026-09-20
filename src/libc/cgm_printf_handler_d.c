/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_05d3dc_338.asm */
// @rom 0x5d3dc +0x338 cgm_printf_handler_d

#include "i960_lift.h"
#include "i960_fp.h"
#include "cgm_format.h"

extern void libc_printf_format_loop(void);

void cgm_printf_handler_d(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;
    cgm_printf_d();
    libc_printf_format_loop();
}
