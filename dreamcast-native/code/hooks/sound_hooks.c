/* Sound board bytes on the Dreamcast: dropped until the sound port exists (include/race/sound_cmd.h). */
#include "race/sound_cmd.h"

void sr_hook_sound_byte(uint8_t byte) { (void)byte; }
