/* Texture block MMIO stream @ 0x48E8 (geo_reg_flip_wait callee). */
// @rom 0x48e8 +0x58 geo_texture_addr_write

#include "i960_lift.h"
#include "i960_mem.h"

void geo_texture_addr_write(u32 src_vaddr, u32 block_index, u32 block_count)
{
    u32 cursor;
    u32 words_left;
    u32 word;

    g14 = 0;
    block_index &= 0xffffu;
    i960_mmio_write_u32(0x800140, (u32)g14);
    i960_mmio_write_u32(0x804000, block_index);
    i960_mmio_write_u32(0x804000, block_count);
    cursor = src_vaddr;
    words_left = block_count;
    while (words_left > 0) {
        word = i960_ld_u32(I960_ABS, cursor, 0);
        cursor += 4u;
        i960_mmio_write_u32(0x804000, word);
        words_left--;
    }
}
