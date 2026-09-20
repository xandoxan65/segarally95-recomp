/* Coin / credit option defaults @ 0xA940 (bal entry @ 0xA948).
 * Seeds price @ 0x20202A = 2 ("CREDIT n/2") and related coin words.
 * source: disasm/maincpu/maincpu_00a940_80.asm */
// @rom 0xa940 +0x80 coin_option_defaults

#include "i960_lift.h"
#include "i960_mem.h"

void coin_option_defaults(u32 arg0, u32 arg1, u32 arg2)
{
    u8 flags;
    u32 g6;
    u32 g5;
    u32 g7;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0xA950–0xA984 — bal @0xA948 skips lda-return; g14 already 0 from caller. */
    i960_st_u16(I960_WORKRAM, 0x202028, 0, 1);
    i960_st_u16(I960_WORKRAM, 0x20202a, 0, 2);
    flags = i960_ld_u8(I960_WORKRAM, 0x202024, 0);
    i960_st_u16(I960_WORKRAM, 0x20202e, 0, 11);
    g6 = 1;
    g7 = 15u << 4; /* shlo 4,15 → 0xF0 */
    i960_st_u16(I960_WORKRAM, 0x20202c, 0, 0); /* stos g14 */
    flags = (u8)(flags & (u8)g7);
    g7 = 1;
    flags = (u8)(flags | 2u);
    g5 = 0x202032u;
    i960_st_u8(I960_WORKRAM, 0x202024, 0, flags);

    /* @0xA9A8–0xA9B8: two stos of 1 walking g5 backward. */
    for (;;) {
        g6 -= 1u;
        i960_st_u16(I960_WORKRAM, g5, 0, (u16)g7);
        if ((i32)g6 < 0)
            break;
        g5 -= 2u;
    }
}
