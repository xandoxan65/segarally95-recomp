/* The I/O board as the native build presents it to sr_io_poll (include/race/io_poll.h): bytes idle at 0xff as on
 * the host reference, the H-gate in IN1 bits 4..6, the analog channels behind the mux port. */
#include "race/io_poll.h"
#include "sr_alloc.h"
#include <string.h>

static uint8_t s_board[0x100];
static uint8_t s_analog[8];
static uint8_t s_channel;

void sr_io_board_reset(void) {
    sr_fill(s_board, 0xff, sizeof(s_board));
    sr_fill(s_analog, 0, sizeof(s_analog));
    s_channel = 0;
}

void sr_io_board_set(uint32_t steer, uint32_t accel, uint32_t brake, uint32_t gear) {
    static const uint8_t gate[5] = {0, 0x50, 0x60, 0x10, 0x20}; /* bits pulled low for gears 1..4 */
    s_analog[0] = (uint8_t)steer;
    s_analog[1] = (uint8_t)accel;
    s_analog[2] = (uint8_t)brake;
    s_board[0x04] = (uint8_t)((s_board[0x04] | 0x70u) & ~(gear <= 4 ? gate[gear] : 0));
}

void sr_io_board_view(int held) {
    s_board[0x02] = (uint8_t)(held ? s_board[0x02] & ~0x20u : s_board[0x02] | 0x20u);
}

uint32_t sr_io_rd8(uint32_t off) {
    if(off == 0x1e) {
        const uint8_t v = s_analog[s_channel & 7];
        s_channel = (uint8_t)((s_channel + 1) & 7);
        return v;
    }
    return s_board[off & 0xff];
}

void sr_io_wr8(uint32_t off, uint32_t v) {
    if(off == 0x1e) s_channel = (uint8_t)(v & 7);
    else s_board[off & 0xff] = (uint8_t)v;
}
