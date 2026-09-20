/* Semantic C — guest immediates only (lda → gN, not host pointers).
 * source: disasm/maincpu/maincpu_008f28_1c4.asm */
// @rom 0x8f28 +0x1c4 backup_sram_sync

#include "i960_lift.h"
#include "model2_rom.h"
#include "i960_mem.h"

/* convention: kind=leaf_bx  args g0,g1,g2  link g14→g7 bx */
/* abi: u32 arg0=g0, u32 arg1=g1, u32 arg2=g2 → void */

void backup_sram_sync(u32 arg0, u32 arg1, u32 arg2)
{
    u32 mode;
    u32 placement;
    u32 poly_lo;
    u32 saved_q;
    u32 geo_count;
    u32 bkp_cc;
    u32 bkp_dc;
    u32 bkp_e0;

    g0 = (uintptr_t)arg0;
    g1 = (uintptr_t)arg1;
    g2 = (uintptr_t)arg2;
    g7 = g14;
    g14 = 0;

    /*
     * @0x8F38–0x8F3C: cmpibl 3,mode → 0x90c0 when mode > 3;
     *                  cmpibg 2,mode → 0x90c0 when mode < 2.
     * Modes 2/3 run the body; every path still falls into 0x90c0 and
     * rebinds 0x20a290 ← 0x918000 (bufferram road/GEO write list base).
     * Prior lift `if (mode >= 3) return` skipped that rebind during race
     * (mode 3), so table_index_a wrote past mapped bufferram → 0x07/0x52
     * failed → chase freefall / swinging rates.
     */
    mode = i960_ld_u32(I960_WORKRAM, 0x202098, 0);
    if ((i32)mode > 3 || (i32)mode < 2)
        goto reset_write_ptr;

    saved_q = (u32)i960_ld_u64(I960_ROM, 0x1d001c0, 0);
    placement = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
    if (g1 < placement)
        g1 = placement;

    poly_lo = i960_ld_u16(I960_MMIO, 0x10800000, 0) & 0xffffu;
    if ((arg2 & 0xffffu) < poly_lo) {
        /* lda 0xffff0000 — mask immediate, not a host pointer. */
        g2 = (arg2 & 0xffff0000u) | poly_lo;
    }

    i960_st_u64(I960_ROM, 0x1d001c0, 0, (u64)saved_q);

    geo_count = i960_ld_u32(I960_WORKRAM, 0x2020a0, 0);
    bkp_cc = i960_ld_u32(I960_ROM, 0x1d001cc, 0);
    g0 = (u32)i960_ld_u64(I960_ROM, 0x1d001d0, 0);
    if (bkp_cc < geo_count) {
        i960_st_u32(I960_ROM, 0x1d001cc, 0, geo_count);
    } else if (g0 <= geo_count && g0 == 0) {
        g0 = geo_count;
    }
    i960_st_u32(I960_ROM, 0x1d00148, 0, 1);

    /* lda 0x2000404 — TGP immediate, not i960_vaddr_ptr. */
    i960_mmio_write_u32(0x884000, 0x02000404u);
    g4 = i960_ld_u32(I960_MMIO, 0x884000, 0);
    g5 = g4 & 0xffffu;
    if (g1 < g5)
        g1 = g5;
    else if (g2 != 0 && g2 > (g4 & 0xffffu))
        g2 = g4 & 0xffffu;

    i960_st_u64(I960_ROM, 0x1d001d0, 0, (u64)g0);

    geo_count = i960_ld_u32(I960_WORKRAM, 0x20a284, 0);
    bkp_dc = i960_ld_u32(I960_ROM, 0x1d001dc, 0);
    if (bkp_dc < geo_count)
        i960_st_u32(I960_ROM, 0x1d001dc, 0, geo_count);
    else {
        bkp_e0 = i960_ld_u32(I960_ROM, 0x1d001e0, 0);
        if (bkp_e0 <= geo_count && bkp_e0 != 0)
            i960_st_u32(I960_ROM, 0x1d001e0, 0, geo_count);
    }

reset_write_ptr:
    /* @0x90C0: lda 0x918000 / st 0x20a290 — guest bufferram base. */
    i960_st_u32(I960_WORKRAM, 0x20a290, 0, 0x00918000u);
    i960_mmio_write_u32(0x884000, 0x01800303u);
    i960_mmio_write_u32(0x884000, (u32)g14);
    (void)g7;
}
