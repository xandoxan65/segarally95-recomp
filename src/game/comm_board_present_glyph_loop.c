/* Comm-board present glyph loop @ 0x21C0 (comm_board_idle_gate callee). */
// @rom 0x21c0 +0x34 comm_board_present_glyph_loop

#include "i960_lift.h"
#include "i960_mem.h"

void comm_board_present_glyph_loop(u32 arg0, u32 arg1, u32 arg2)
{
    u32 stream;
    u32 slot;
    u32 left;
    u16 glyph;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    i960_call_rom(0x1e20);
    stream = 0x00202010u;
    slot = 0;
    left = 18u;
    while (left > 0) {
        glyph = (u16)i960_ld_u16(I960_WORKRAM, stream, 0);
        g0 = slot;
        g1 = glyph;
        i960_call_rom(0x2080);
        slot++;
        stream += 2u;
        left--;
    }
    i960_call_rom(0x1e80);
}
