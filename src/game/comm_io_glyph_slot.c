/* Comm-board glyph slot emit @ 0x2080 (comm_board_present_glyph_loop callee). */
// @rom 0x2080 +0xc0 comm_io_glyph_slot

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_rom.h"

#define IO_BOARD_DATA  0x01C00000u
#define GLYPH_HDR_BASE 0x005A1070u

static void comm_io_emit_byte(u8 byte)
{
    i960_st_u8(I960_ABS, IO_BOARD_DATA, 0, byte);
    i960_call_rom(0x1db8);
}

void comm_io_glyph_slot(u32 slot_index, u32 glyph_word, u32 arg2)
{
    u32 cursor;
    u32 left;
    u32 round;
    u32 masked;
    u8 byte;

    (void)arg2;

    cursor = GLYPH_HDR_BASE;
    left = 10u;
    while (left > 0) {
        byte = model2_workram_mirror_u8(cursor);
        comm_io_emit_byte(byte);
        left--;
        cursor++;
    }

    round = 5u;
    while (round > 0) {
        masked = (slot_index & 0x1fu) + 1u;
        byte = model2_workram_mirror_u8(masked + 0x51u);
        comm_io_emit_byte(byte);
        byte = model2_workram_mirror_u8(masked + 0xd1u);
        comm_io_emit_byte(byte);
        slot_index <<= 1;
        round--;
    }

    round = 15u;
    while (round > 0) {
        masked = ((glyph_word >> 10) & 0x1fu) + 1u;
        byte = model2_workram_mirror_u8(masked + 0x51u);
        comm_io_emit_byte(byte);
        byte = model2_workram_mirror_u8(masked + 0xd1u);
        comm_io_emit_byte(byte);
        glyph_word <<= 1;
        round--;
    }

    comm_io_emit_byte(1u);
}
