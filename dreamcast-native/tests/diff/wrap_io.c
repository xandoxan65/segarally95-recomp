/* I/O poll port (code/race/io_poll.c) against the lifted one, on the host's board. */
#include "wrap_common.h"
#include "race/io_poll.h"
#include "i960_mem.h"
#include "i960_host.h"

uint32_t sr_io_rd8(uint32_t off) { return i960_ld_u8(I960_ABS, 0x01c00000u + off, 0); }

void sr_io_wr8(uint32_t off, uint32_t v) { i960_st_u8(I960_ABS, 0x01c00000u + off, 0, (u8)v); }

/* The host's game_io_poll also applies the scripted inputs before the sample and holds the --practice flags
 * after it; those are host scaffolding around the same native poll. */
void lifted_game_io_poll(u32 a0, u32 a1, u32 a2);
void game_io_poll(u32 a0, u32 a1, u32 a2) {
    u8 analog[8];
    unsigned i;
    /* The board is not part of the rewound state, and the --practice hold rewrites the analog channels. */
    for(i = 0; i < 8; i++) analog[i] = model2_io_analog_get(i);
    SR_DIFF_BEGIN();
    lifted_game_io_poll(a0, a1, a2);
    SR_DIFF_NATIVE();
    for(i = 0; i < 8; i++) model2_io_analog_set(i, analog[i]);
    i960_host_race_inputs_apply();
    sr_io_poll();
    i960_host_skip_practice_hold_inputs();
    SR_DIFF_END("game_io_poll");
}

/* The diff build never drains the ring natively (the lifted vblank does). */
void sr_hook_sound_byte(uint8_t byte) { (void)byte; }
