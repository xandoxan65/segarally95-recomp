/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_02a050_a0.asm */
// @rom 0x2a050 +0xa0 cgm_leading_colorbase

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_rom.h"
#include "model2_memory.h"
#include "cgm_format.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* abi: u32 slot_base=g0, u32 * stream_ctrl=g1, u32 leading_count=g2 */

extern void palram_colorbase_upload(u32 slot, u32 merge_index, u32 arg2);

static int stream_is_workram(u32 vaddr)
{
    return vaddr >= WORKRAM_BASE && vaddr < WORKRAM_BASE + WORKRAM_SIZE;
}

static u16 rom_u16(u32 vaddr)
{
    const u8 *p;

    if (stream_is_workram(vaddr))
        return i960_ld_u16(I960_WORKRAM, vaddr, 0);
    p = model2_rom_at(vaddr);
    if (!p)
        return 0;
    return (u16)p[0] | ((u16)p[1] << 8);
}

static u64 rom_u64(u32 vaddr)
{
    const u8 *p;
    u64 v = 0;

    if (stream_is_workram(vaddr)) {
        u32 lo = i960_ld_u32(I960_WORKRAM, vaddr, 0);
        u32 hi = i960_ld_u32(I960_WORKRAM, vaddr, 4);

        return (u64)lo | ((u64)hi << 32);
    }
    p = model2_rom_at(vaddr);
    if (p)
        memcpy(&v, p, 8);
    return v;
}

void cgm_leading_colorbase(void * arg0, void * arg1, u32 arg2)
{
    u32 *stream_ctrl = (u32 *)arg1;
    u32 slot_base = (u32)(uintptr_t)arg0;
    u32 leading = arg2 & 0xffffu;
    u32 pair_count;
    u32 scratch_base;
    u32 stream;
    u32 slot_index;
    u32 n;

    stream = stream_ctrl ? *stream_ctrl : 0;

    for (slot_index = 0; slot_index < leading; slot_index++) {
        u32 slot = slot_base + slot_index;

        /* @0x2A07C movl r4,g0: g0=slot, g1=current 32-byte palette header. */
        palram_colorbase_upload(slot, stream, 0u);
        /* @0x2A084 is an address increment, not a link dereference. */
        stream += 0x22u;
        scratch_base = 0x01080000u + (slot << 12);
        pair_count = rom_u16(stream);
        if (getenv("I960_TRACE_SPLASH")) {
            fprintf(stderr,
                    "lift: cgm leading slot=%u stream=%#x pairs=%u\n",
                    (unsigned)slot, (unsigned)stream, (unsigned)pair_count);
        }
        stream += 2u;
        if (pair_count != 0) {
            u32 src_a = stream;
            u32 src_b = stream + 16u;
            u32 dst_a = scratch_base;
            u32 dst_b = scratch_base + 0x10u;

            for (n = 0; n < pair_count; n++) {
                /*
                 * @0x2A0B0/@0x2A0B8 and @0x2A0B4/@0x2A0C4 are ldq/stq:
                 * each transfer is 16 bytes, not one 8-byte i960 ``long``.
                 */
                i960_st_u64(I960_ABS, dst_a, 0, rom_u64(src_a));
                i960_st_u64(I960_ABS, dst_a, 8, rom_u64(src_a + 8u));
                i960_st_u64(I960_ABS, dst_b, 0, rom_u64(src_b));
                i960_st_u64(I960_ABS, dst_b, 8, rom_u64(src_b + 8u));
                src_a += 0x20u;
                src_b += 0x20u;
                dst_a += 0x20u;
                dst_b += 0x20u;
            }
            stream = src_a;
        }
    }

    if (stream_ctrl)
        *stream_ctrl = stream;
}
