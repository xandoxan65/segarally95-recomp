/* Mode table XOR/hash fetch @ 0x297E0 (boot_mode_index_step callee). */
// @rom 0x297e0 +0x60 game_mode_table_fetch

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_rom.h"

u32 game_mode_table_fetch(u32 table_ptr, u32 row_count)
{
    u32 cursor;
    u32 rows_left;
    u32 state;
    u32 byte;
    u32 nibble;
    u32 word;
    const u8 *tbl;

    cursor = table_ptr;
    state = 0xffffu;
    if (row_count == 0) {
        g0 = 0;
        return 0;
    }

    rows_left = row_count - 1u;
    for (;;) {
        byte = model2_workram_mirror_u8(cursor);
        nibble = (state >> 8) & 0xffu;
        nibble ^= byte;
        nibble &= 0xffu;
        tbl = model2_rom_at(0x005c8350u + (nibble * 4u));
        word = tbl ? *(const u32 *)tbl : 0u;
        state = (state << 8) ^ word;
        if (rows_left == (u32)-1)
            break;
        rows_left--;
        cursor++;
    }

    g0 = state & ~0xffffu;
    return (u32)g0;
}
