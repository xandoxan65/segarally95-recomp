/* CRX gamma LUT upload @ 0x37F0 (geo_renderer_init callee). */
// @rom 0x37f0 +0x88 geo_crx_lut_upload

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_rom.h"

u32 geo_lut_byte_pack(u32 index);

static u8 geo_lut_chain_byte(u32 index)
{
    return model2_workram_mirror_u8(0x7f00u + index);
}

void geo_crx_lut_upload(u32 dest_vaddr, u32 arg1, u32 arg2)
{
    u32 dest;
    u32 index;
    u32 word;
    u32 left;

    (void)arg1;
    (void)arg2;

    dest = dest_vaddr;
    index = 0;
    left = 0x2000u;
    while (left > 0) {
        g0 = index;
        word = geo_lut_byte_pack(index);
        index = geo_lut_chain_byte(index);
        g0 = index;
        word |= geo_lut_byte_pack(index) << 8;
        index = geo_lut_chain_byte(index);
        g0 = index;
        word |= geo_lut_byte_pack(index) << 16;
        index = geo_lut_chain_byte(index);
        g0 = index;
        word |= geo_lut_byte_pack(index) << 24;
        index = geo_lut_chain_byte(index);
        i960_st_u32(I960_ABS, dest, 0, word);
        dest += 4u;
        left--;
    }
}
