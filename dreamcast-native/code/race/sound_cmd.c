/* The sound command queue and the text cursor (0x26090 / 0x26140 / 0x261B0 / 0x26E18): gameplay queues 3-byte
 * commands for the sound board into a ring that the vblank drains. */
#include "race/sound_cmd.h"
#include "sr_mem.h"
#include <stdio.h>

#define SOUND_CMD_TABLE 0x5c3ce0u /* 3-byte commands */
#define SOUND_RING      0x20b100u /* 0x80-byte ring */
#define SOUND_PENDING   0x20b180u
#define SOUND_CURSOR    0x20b184u
#define SOUND_BUSY      0x20b188u
#define SOUND_STAGING   0x20b18cu

void sr_text_cursor(uint32_t x, uint32_t y) {
    sr_st32(0x20b1a4u, x);
    sr_st32(0x20b1a8u, x);
    sr_st32(0x20b1acu, y);
}

/* 0x26090: append the 3 bytes at `src` to the ring. A full ring (more than ~42 commands in one tick, never seen:
 * the ring drains each vblank) shows an error text script instead; only its view scale and cursor are ported. */
static void sound_ring_put(uint32_t src) {
    const uint32_t pending = sr_ld32(SOUND_PENDING);
    uint32_t cursor, i;
    if(pending + 3u > 0x80u) {
        static int reported;
        if(!reported++) printf("sr: sound ring full, error script (0x5c5070) not ported\n");
        sr_st32(0x20b1b0u, 8u << 7);
        sr_text_cursor(10, 10);
        return;
    }
    cursor = sr_ld32(SOUND_CURSOR);
    sr_st32(SOUND_BUSY, 1);
    for(i = 0; i < 3; i++) {
        sr_st8(SOUND_RING + cursor, sr_ld8(src + i));
        cursor = (cursor + 1u) & 0x7fu;
    }
    sr_st32(SOUND_CURSOR, cursor);
    sr_st32(SOUND_BUSY, 0);
    sr_st32(SOUND_PENDING, pending + 3u);
}

void sr_sound_cmd(uint32_t index) { sound_ring_put(SOUND_CMD_TABLE + index * 3u); }

void sr_sound_cmd_fill(uint32_t index, uint32_t fill) {
    const uint32_t src = SOUND_CMD_TABLE + index * 3u;
    const uint32_t b2 = sr_ld8(src + 2);
    sr_st8(SOUND_STAGING, sr_ld8(src));
    sr_st8(SOUND_STAGING + 1, sr_ld8(src + 1));
    sr_st8(SOUND_STAGING + 2, b2 == 0xffu ? fill : b2);
    sound_ring_put(SOUND_STAGING);
}

/* The timer interrupt (0x25C68) sends one ring byte to the sound board every 2 ms, so the ring is empty by the next
 * frame: hand the pending bytes over in order. */
void sr_sound_drain(void) {
    uint32_t pending = sr_ld32(SOUND_PENDING);
    const uint32_t cursor = sr_ld32(SOUND_CURSOR);
    while(pending) {
        sr_hook_sound_byte(sr_ld8(SOUND_RING + ((cursor - pending) & 0x7fu)));
        pending--;
    }
    sr_st32(SOUND_PENDING, 0);
}
