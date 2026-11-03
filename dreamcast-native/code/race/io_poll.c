/* The 315-5649 I/O board poll (0x26B8), as the race reaches it through game_io_poll (0x24C0). */
#include "race/io_poll.h"
#include "sr_mem.h"

void sr_io_poll(void) {
    uint32_t prev, cur, word, t, i;
    uint8_t lo;

    sr_io_wr8(0x1e, 0);
    for(i = 0; i < 8; i++) sr_st8(0x202050u + i, sr_io_rd8(0x1e));
    sr_io_wr8(0x10, 0x4e);
    sr_io_wr8(0x00, 0);
    lo = (uint8_t)sr_io_rd8(0x0c);
    sr_st8(0x202058u, lo);
    sr_st8(0x202059u, lo);
    sr_st8(0x20205au, lo);

    /* Switches, active low on the board: IN2 << 16 | IN1 << 8 | IN0, inverted, bits 4..7 of IN0 swapped in pairs. */
    prev = sr_ld32(0x20205cu);
    sr_st32(0x202060u, prev);
    word = ~((sr_io_rd8(0x06) << 16) | (sr_io_rd8(0x04) << 8) | sr_io_rd8(0x02)) & 0x00ffffffu;
    sr_st32(0x20205cu, word);
    lo = (uint8_t)word;
    t = (word >> 2) & 16u;
    lo = (uint8_t)((lo & ~16u) | t);
    t = (word >> 2) & 32u;
    lo = (uint8_t)((lo & ~32u) | t);
    t = (word << 2) & 64u;
    lo = (uint8_t)((lo & ~64u) | t);
    t = (word << 2) & 128u;
    lo = (uint8_t)((lo & 0x7fu) | t);
    sr_st8(0x20205cu, lo);
    cur = sr_ld32(0x20205cu);
    sr_st32(0x202064u, cur & ~prev);
    sr_st32(0x202068u, prev & ~cur);
    sr_io_wr8(0x00, 1);

    prev = sr_ld32(0x20206cu);
    sr_st32(0x202070u, prev);
    cur = ~(sr_io_rd8(0x02) >> 6) & 3u;
    sr_st32(0x20206cu, cur);
    sr_st32(0x202074u, cur & ~prev);
    sr_st32(0x202078u, prev & ~cur);

    /* The H-gate's switches index the gear table 0x5a14b0. */
    sr_st32(0x202044u, sr_rom_mirror32(0x5a14b0u + ((sr_ld32(0x20205cu) >> 12) & 7u)) & 0xffu);
}
