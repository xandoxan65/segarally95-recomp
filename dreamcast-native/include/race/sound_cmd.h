/* Sound command queue and text cursor (code/race/sound_cmd.c). */
#ifndef SR_RACE_SOUND_CMD_H
#define SR_RACE_SOUND_CMD_H
#include <stdint.h>

/* 0x261B0 / 0x26140 / 0x26090: queue a 3-byte sound command from table 0x5c3ce0 into the sound ring at 0x20b100
 * (drained to the sound board each vblank). `fill` replaces a third byte of 0xff (sr_sound_cmd_fill only). */
void sr_sound_cmd_fill(uint32_t index, uint32_t fill);
void sr_sound_cmd(uint32_t index);
/* 0x26E18 tile_cursor_seed: text cursor 0x20b1a4/a8 = x, 0x20b1ac = y. */
void sr_text_cursor(uint32_t x, uint32_t y);

/* Native builds: send the queued bytes to the sound board, one call per byte (code/hooks/sound_hooks.c). */
void sr_sound_drain(void);
void sr_hook_sound_byte(uint8_t byte);

#endif
