/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_027008_128.asm */
// @rom 0x27008 +0x128 boot_tile_opcode_dispatch

#include "i960_lift.h"
#include "i960_mem.h"

#define TILE_DESC_BUS  0x01000000u /* @0x27110 stos … 0x1000000[g5*2] */

/* @0x270CC: descriptor halfword on staging bus + col advance. */
static void tile_desc_bus_emit(u32 opcode_word)
{
    u32 row;
    u32 col;
    u32 pal_bits;
    u32 char_byte;
    u32 entry;
    u32 map_index;

    row = i960_ld_u32(I960_WORKRAM, 0x20b1ac, 0);
    pal_bits = i960_ld_u16(I960_WORKRAM, 0x20b1b0, 0);
    char_byte = opcode_word & 0xffu;
    col = i960_ld_u32(I960_WORKRAM, 0x20b1a8, 0);
    entry = 0x8000u | (pal_bits & 0xffffu) | char_byte;
    map_index = (row << 6) + col;
    i960_st_u16(I960_ABS, TILE_DESC_BUS, map_index * 2u, (u16)entry);
    col = i960_ld_u32(I960_WORKRAM, 0x20b1a8, 0);
    /* @0x2710C addo 31,29,g1; @0x27118 cmpibg — advance col while col <= 60. */
    if (col <= 60u)
        i960_st_u32(I960_WORKRAM, 0x20b1a8, 0, col + 1u);
}

void boot_tile_opcode_dispatch(u32 opcode_word)
{
    u32 op;

    g0 = (uintptr_t)opcode_word;
    op = opcode_word & 0xffu;

    if (op > 31u) {
        tile_desc_bus_emit(opcode_word);
        return;
    }

    if (op == 8u) {
        u32 col = i960_ld_u32(I960_WORKRAM, 0x20b1a8, 0);

        /* @0x270AC: col = max(0, col-1). */
        if ((i32)col > 0)
            col--;
        i960_st_u32(I960_WORKRAM, 0x20b1a8, 0, col);
        return;
    }

    if (op == 9u) {
        u32 col;
        u32 row;
        u32 tag;

        /* @0x27034: tag @ 0x20B1B0 (lift had lda 0x8(g5) — cursor struct +8). */
        tag = i960_ld_u16(I960_WORKRAM, 0x20b1b0, 0);
        col = tag & 0xfffffff8u;
        if (col <= 61u) {
            row = i960_ld_u32(I960_WORKRAM, 0x20b1ac, 0);
            if (row <= 46u)
                row++;
            i960_st_u32(I960_WORKRAM, 0x20b1ac, 0, row);
        }
        i960_st_u32(I960_WORKRAM, 0x20b1a8, 0, col);
        return;
    }

    if (op == 10u) {
        u32 anchor_col = i960_ld_u32(I960_WORKRAM, 0x20b1a4, 0);
        u32 row = i960_ld_u32(I960_WORKRAM, 0x20b1ac, 0);

        /* @0x2707C: reset col; increment row while row <= 46. */
        i960_st_u32(I960_WORKRAM, 0x20b1a8, 0, anchor_col);
        if (row <= 46u)
            row++;
        i960_st_u32(I960_WORKRAM, 0x20b1ac, 0, row);
        return;
    }
}
