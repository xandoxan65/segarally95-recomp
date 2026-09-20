/* Extra glyph command stream @ 0x1E80 (comm_board_present_glyph_loop tail). */
// @rom 0x1e80 +0x34 boot_io_glyph_finish

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_rom.h"

#define IO_BOARD_DATA     0x01C00000u
#define GLYPH_TAIL_TABLE  0x005A0E60u
#define GLYPH_TAIL_COUNT  21u

void boot_io_glyph_finish(u32 arg0, u32 arg1, u32 arg2)
{
    u32 cursor;
    u32 left;
    u8 byte;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    cursor = GLYPH_TAIL_TABLE;
    left = GLYPH_TAIL_COUNT;
    while (left > 0) {
        byte = model2_workram_mirror_u8(cursor);
        i960_st_u8(I960_ABS, IO_BOARD_DATA, 0, byte);
        i960_call_rom(0x1db8);
        left--;
        cursor++;
    }
}
