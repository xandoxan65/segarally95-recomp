/* Cabinet controls (code/race/io_poll.c): the 315-5649 I/O board as the game polls it once a tick. */
#ifndef SR_RACE_IO_POLL_H
#define SR_RACE_IO_POLL_H
#include <stdint.h>

/* Board port access (0x01c00000 + offset). Reading the analog mux port 0x1e returns the selected channel and
 * advances it; writing it selects a channel. Native builds: code/io_board.c; the differential checks: the host's
 * board (tests/diff/wrap_io.c). */
uint32_t sr_io_rd8(uint32_t off);
void sr_io_wr8(uint32_t off, uint32_t v);

/* 0x26B8: sample the 8 analog channels to 0x202050.. (steer, accel, brake, ...), the switch ports to 0x20205c
 * with press/release edges at 0x202064/68 and start edges at 0x202074/78, and the gear lever to 0x202044. */
void sr_io_poll(void);

/* Native builds: what the board reads (code/io_board.c). steer 0x80 is centre; gear 0 neutral, 1..4. */
void sr_io_board_reset(void);
void sr_io_board_set(uint32_t steer, uint32_t accel, uint32_t brake, uint32_t gear);
/* The VIEW CHANGE button, held or not (IN0 bit 5; the poll's press edge 0x202064 bit 7 toggles the camera). */
void sr_io_board_view(int held);

#endif
