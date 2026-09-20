/* Coin / IN0 sense @ 0x2240 — gate @ 0x202090.
 * Present: 315-5649 body @ 0x2378 reads Port B IN0, debounce into
 * 0x202043 (held active-high), 0x202041 press edges, 0x202040 release edges.
 * AB10 (@ 0xAF00→AC30) consumes release edges for COIN1 bit0.
 * source: disasm/maincpu/maincpu_002240_c0.asm, maincpu_002378_140.asm */
// @rom 0x2240 +0x28 game_io_coin_sense

#include "i960_lift.h"
#include "i960_mem.h"

static void game_io_coin_sense_315_5649(void)
{
    u8 prev;
    u8 cur;
    u8 out;
    u32 w80;
    u32 w4c;

    i960_st_u8(I960_ABS, 0x01c00010u, 0, 0x4e);

    prev = (u8)i960_ld_u8(I960_WORKRAM, 0x202043, 0);
    i960_st_u8(I960_WORKRAM, 0x202042, 0, prev);

    cur = (u8)i960_ld_u8(I960_ABS, 0x01c00002u, 0);
    cur = (u8)~cur;
    i960_st_u8(I960_WORKRAM, 0x202043, 0, cur);

    /* Press = curr & ~prev; release = prev & ~curr. */
    i960_st_u8(I960_WORKRAM, 0x202041, 0, (u8)(cur & (u8)~prev));
    i960_st_u8(I960_WORKRAM, 0x202040, 0, (u8)(prev & (u8)~cur));

    out = (u8)i960_ld_u8(I960_WORKRAM, 0x20204c, 0);
    out = (u8)(out & ~(1u << 6));
    i960_st_u8(I960_WORKRAM, 0x20204c, 0, out);

    out = (u8)i960_ld_u8(I960_WORKRAM, 0x20204c, 0);
    out = (u8)((out & ~(1u << 4)) | ((out >> 2) & 16u));
    i960_st_u8(I960_WORKRAM, 0x20204c, 0, out);

    out = (u8)i960_ld_u8(I960_WORKRAM, 0x20204c, 0);
    out = (u8)((out & ~(1u << 3)) | ((out >> 1) & 8u));
    i960_st_u8(I960_WORKRAM, 0x20204c, 0, out);

    w80 = i960_ld_u32(I960_WORKRAM, 0x202080, 0) & 3u;
    w4c = i960_ld_u32(I960_WORKRAM, 0x20204c, 0) & 0xfcu;
    i960_st_u8(I960_ABS, 0x01c0000au, 0, (u8)(w80 + w4c));

    i960_st_u8(I960_WORKRAM, 0x202048, 0,
               (u8)i960_ld_u8(I960_ABS, 0x01c00006u, 0));
    i960_st_u8(I960_ABS, 0x01c00008u, 0,
               (u8)i960_ld_u8(I960_WORKRAM, 0x202049, 0));
}

void game_io_coin_sense(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* Same gate as game_io_poll: host keyboard uses 315-5649 IN0 @ 0x2378. */
    game_io_coin_sense_315_5649();
}
