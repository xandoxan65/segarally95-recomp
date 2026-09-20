/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_027008_200.asm @ 0x27130 */
// @rom 0x27130 +0x24 boot_tile_script_run

#include "i960_lift.h"
#include "i960_mem.h"
#include "i960_host.h"
#include "lift_syms.h"

void boot_tile_script_run(u32 script_ptr)
{
    u32 r4;
    u8 peek;

    /* @0x27130: ldob (g0),g4; cmpibe 0,g4,0x27150 */
    g0 = script_ptr;
    peek = (u8)i960_ld_u8(I960_ABS, (u32)g0, 0);
    if (peek == 0)
        return;

    /* @0x27134: mov g0,r4 */
    r4 = (u32)g0;

    /* @0x2713C–0x2714C: ldob (r4),g0; bal 0x27008; cmpibne 0,(r4),0x2713c */
    for (;;) {
        g0 = i960_ld_u8(I960_ABS, r4, 0);
        r4++;
        boot_tile_opcode_dispatch((u32)g0);
        peek = (u8)i960_ld_u8(I960_ABS, r4, 0);
        if (peek == 0)
            break;
    }

    i960_host_milestone_boot_notify_script(script_ptr);
}
