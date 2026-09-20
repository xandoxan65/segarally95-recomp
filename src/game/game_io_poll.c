/* Cabinet I/O poll @ 0x24C0 — gate @ 0x202090 selects path.
 * Present (nonzero): 315-5649 body @ 0x26B8 (MAME Port B IN0 @ 0x01C00002).
 * Absent: comm-board path @ 0x24E8 (not lifted — no cabinet).
 * source: disasm/maincpu/maincpu_0026b8_260.asm */
// @rom 0x24c0 +0x28 game_io_poll

#include "i960_lift.h"
#include "i960_mem.h"
#include "i960_host.h"
#include "model2_rom.h"

static void game_io_poll_315_5649(void)
{
    u32 prev;
    u32 cur;
    u32 word;
    u8 lo;
    u32 t;
    u32 idx;

    /* @0x26C0–0x26E4: sample 0x01C0001E into 8-byte scratch @ 0x202050. */
    {
        u32 dst = 0x202050u;
        u32 end = dst + 7u;

        i960_st_u8(I960_ABS, 0x01c0001eu, 0, 0);
        for (;;) {
            i960_st_u8(I960_ABS, dst, 0, (u8)i960_ld_u8(I960_ABS, 0x01c0001eu, 0));
            if (dst >= end)
                break;
            dst += 1u;
        }
    }

    i960_st_u8(I960_ABS, 0x01c00010u, 0, 0x4e);
    i960_st_u8(I960_ABS, 0x01c00000u, 0, 0);
    lo = (u8)i960_ld_u8(I960_ABS, 0x01c0000cu, 0);
    i960_st_u8(I960_WORKRAM, 0x202058, 0, lo);
    i960_st_u8(I960_WORKRAM, 0x202059, 0, lo);
    i960_st_u8(I960_WORKRAM, 0x20205a, 0, lo);

    prev = i960_ld_u32(I960_WORKRAM, 0x20205c, 0);
    i960_st_u32(I960_WORKRAM, 0x202060, 0, prev);

    /* Pack IN2<<16 | IN1<<8 | IN0 (ports @ 0x01C00006/04/02). */
    word = ((u32)i960_ld_u8(I960_ABS, 0x01c00006u, 0) & 0xffu) << 16;
    word |= ((u32)i960_ld_u8(I960_ABS, 0x01c00004u, 0) & 0xffu) << 8;
    word |= (u32)i960_ld_u8(I960_ABS, 0x01c00002u, 0) & 0xffu;
    /* Active-high: andnot raw, 0xffffff → ~raw & 0xffffff (MAME i960). */
    word = (~word) & 0x00ffffffu;
    i960_st_u32(I960_WORKRAM, 0x20205c, 0, word);

    /* Remap low byte: bit4←bit6, bit5←bit7, bit6←bit4, bit7←bit5. */
    lo = (u8)(word & 0xffu);
    t = (word >> 2) & 16u;
    lo = (u8)((lo & ~(1u << 4)) | t);
    t = (word >> 2) & 32u;
    lo = (u8)((lo & ~(1u << 5)) | t);
    t = (word << 2) & 64u;
    lo = (u8)((lo & ~(1u << 6)) | t);
    t = (word << 2) & 128u;
    lo = (u8)((lo & 0x7fu) | t);
    i960_st_u8(I960_WORKRAM, 0x20205c, 0, lo);
    cur = i960_ld_u32(I960_WORKRAM, 0x20205c, 0);

    /* Press / release edges vs previous sample @ 0x202060. */
    i960_st_u32(I960_WORKRAM, 0x202064, 0, cur & ~prev);
    i960_st_u32(I960_WORKRAM, 0x202068, 0, prev & ~cur);

    i960_st_u8(I960_ABS, 0x01c00000u, 0, 1);

    /* Start path: IN0>>6, MAME notand 3,g4 → (~g4)&3 (active-high START1). */
    prev = i960_ld_u32(I960_WORKRAM, 0x20206c, 0);
    i960_st_u32(I960_WORKRAM, 0x202070, 0, prev);
    cur = (u32)i960_ld_u8(I960_ABS, 0x01c00002u, 0) & 0xffu;
    cur = (~(cur >> 6)) & 3u;
    i960_st_u32(I960_WORKRAM, 0x20206c, 0, cur);
    i960_st_u32(I960_WORKRAM, 0x202074, 0, cur & ~prev);
    i960_st_u32(I960_WORKRAM, 0x202078, 0, prev & ~cur);

    idx = (i960_ld_u32(I960_WORKRAM, 0x20205c, 0) >> 12) & 7u;
    i960_st_u32(I960_WORKRAM, 0x202044, 0,
                model2_workram_mirror_u8(0x005a14b0u + idx));
}

void game_io_poll(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    /*
     * ROM gate @ 0x202090: nonzero → 0x26B8 (315-5649 IN0 @ 0x01C00002),
     * zero → 0x24E8 (comm-board extended regs). Host has no comm loopback at
     * 0x01C00202 so probe leaves gate 0 — but keyboard coin/start are Port B
     * IN0, so always take the 315-5649 body.
     */
    game_io_poll_315_5649();

    /*
     * --practice analog after the 315-5649 sample. Holding first was wiped
     * by the 0x01C0001E copy into 0x202050 (center → Celica AT lookup).
     */
    i960_host_skip_practice_hold_inputs();
}
