/* Stream 21 glyph command bytes to I/O board @ 0x01C00000. */
// @rom 0x1e20 +0x34 boot_io_glyph_stream

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_rom.h"

#define IO_BOARD_DATA     0x01C00000u
#define GLYPH_CMD_TABLE   0x005A0E00u
#define GLYPH_CMD_COUNT   21u

void boot_io_glyph_stream(u32 arg0, u32 arg1, u32 arg2)
{
    u32 cursor;
    u32 left;
    u8 byte;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    cursor = GLYPH_CMD_TABLE;
    left = GLYPH_CMD_COUNT;
    while (left > 0) {
        byte = model2_workram_mirror_u8(cursor);
        i960_st_u8(I960_ABS, IO_BOARD_DATA, 0, byte);
        i960_call_rom(0x1db8);
        left--;
        cursor++;
    }
}
