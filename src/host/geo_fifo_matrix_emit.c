/* Semantic C from MAME disasm @ 0x4958 — edit by hand; not tier-3 byte-matched. */
/* source: disasm/maincpu/maincpu_004958_60.asm */
// @rom 0x4958 +0x60 geo_fifo_matrix_emit

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_memory.h"
#include "model2_rom.h"

/*
 * Push a headed word list to geo prg_fifo @ 0x804000.
 *   g0 = header / opcode word
 *   g1 = pair count (loop iterations; each iter emits 2 words)
 *   g2 = guest pointer to u32 payload (2*g1 words)
 * Also clears geo reg @ 0x800060.
 * Returns via bx(link) — link saved in g3.
 *
 * Note: bootstrap @ 0x4AC0 calls this after st 0x41000000 with a lum/scale
 * table @ 0x5A39C0 (pairs of u32 + 1.0f), not a 3x4 matrix.
 * That table lives in the workram ROM mirror (@ vaddr - 0x59F000). Prefer the
 * mirror read so dirty/zero workram cannot push ambient=0 (black car roofs
 * under bootstrap light (0,-1,·) which relies on texparam ambient).
 */
static u32 payload_word(u32 addr)
{
    if (addr >= WORKRAM_ROM_MIRROR)
        return model2_workram_mirror_u32(addr);
    return i960_ld_u32(I960_ABS, addr, 0);
}

void geo_fifo_matrix_emit(u32 header, u32 pair_count, u32 payload_vaddr)
{
    u32 n;
    uintptr_t link;
    u32 addr;
    u32 w;

    n = pair_count - 1u;
    link = g14;
    addr = payload_vaddr;

    g3 = link;
    g14 = 0;
    i960_mmio_write_u32(0x800060u, 0u);
    i960_mmio_write_u32(GEO_PRG_FIFO, header);
    i960_mmio_write_u32(GEO_PRG_FIFO, pair_count);

    /* cmpibe g5,g6 skips loop when pair_count==0 (n==0xffffffff) */
    if (n != 0xffffffffu) {
        do {
            w = payload_word(addr);
            i960_mmio_write_u32(GEO_PRG_FIFO, w);
            addr += 4u;
            n--;
            w = payload_word(addr);
            addr += 4u;
            i960_mmio_write_u32(GEO_PRG_FIFO, w);
        } while (n != 0xffffffffu);
    }

    g2 = addr;
    g14 = link;
}
