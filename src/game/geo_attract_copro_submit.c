/* Semantic C from MAME disasm @ 0x31fc0 — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_031fc0_190.asm */
// @rom 0x31fc0 +0x190 geo_attract_copro_submit

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"
#include "model2_rom.h"

/*
 * Walk pending object indices in the 0x20d600[] queue (object_extra pushes
 * indices, not pointers). Disasm:
 *   lda 0x20d600[r7*4],r6   — slot *address*
 *   ld  (r6),r5             — object index
 * Dual cursors r6 (walk) and r8 (compaction source) both step by −4; on a
 * successful consume, `ld (r8)` / `st (r6)` may copy an older slot when skips
 * left r8 behind.
 */

void geo_attract_copro_submit(u32 arg0, u32 arg1, u32 arg2)
{
    u32 remain;
    i32 slot_i;
    u32 r6;
    u32 r8_cur;
    u32 obj_ix;
    u32 ix;
    u32 rec_va;
    u32 wr_slot;
    u32 wr_ptr;
    u32 tag;
    u32 g2w, g3w, g4w, g5w, g6w, g7w, g0w, g1w;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x31FC0–0x31FD0 */
    remain = i960_ld_u32(I960_WORKRAM, 0x20d7e0, 0);
    if ((i32)remain <= 0)
        return;
    slot_i = (i32)remain - 1;
    if (slot_i < 0)
        return;

    /* @0x31FD4: lda 0x20d600[r7*4],r6 — address, not load. */
    r6 = 0x20d600u + ((u32)slot_i << 2);
    r8_cur = r6;
    r9 = (uintptr_t)i960_vaddr_ptr(0x2a805555);
    r10 = (uintptr_t)i960_vaddr_ptr(0x3fc00000);
    r11 = (uintptr_t)i960_vaddr_ptr(0x3f800000);
    r12 = 0xff;

    do {
        /* @0x31FFC: ld (r6),r5 */
        obj_ix = i960_ld_u32(I960_WORKRAM, r6, 0);
        g2w = i960_ld_u32(I960_WORKRAM, 0x20220c, 0);
        wr_slot = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
        g5w = i960_ld_u32(I960_MMIO, 0x802008, 0);
        /* @0x32018–0x3201C: lda (r5)[r5*8]; lda 0x20cd90[g4*4],r4 */
        ix = obj_ix + (obj_ix << 3);
        rec_va = 0x20cd90u + (ix << 2);
        g7w = i960_ld_u32(I960_WORKRAM, rec_va, 4);
        g4w = i960_ld_u32(I960_WORKRAM, rec_va, 8);
        g0w = i960_ld_u32(I960_WORKRAM, rec_va, 0xc);
        g1w = i960_ld_u32(I960_WORKRAM, rec_va, 0x1c);
        i960_st_u32(I960_WORKRAM, wr_slot, 0, g5w);
        i960_mmio_write_u32(0x884000, (u32)r9);
        g3w = i960_ld_u32(I960_WORKRAM, 0x202214, 0);
        i960_mmio_write_u32(0x884000, g7w);
        i960_mmio_write_u32(0x884000, g4w);
        i960_mmio_write_u32(0x884000, g0w);
        i960_mmio_write_u32(0x884000, g2w);
        i960_mmio_write_u32(0x884000, g7w);
        i960_mmio_write_u32(0x884000, g3w);
        i960_mmio_write_u32(0x884000, g0w);
        i960_mmio_write_u32(0x884000, (u32)r10);
        i960_mmio_write_u32(0x884000, g1w);
        i960_mmio_write_u32(0x884000, (u32)r11);
        /* @0x3209C: lda 0x34(g5); st → 0x801008 */
        wr_ptr = g5w + 0x34u;
        i960_mmio_write_u32(0x801008, wr_ptr);
        g0w = i960_ld_u32(I960_WORKRAM, rec_va, 0);
        wr_slot += 4u;
        i960_st_u32(I960_WORKRAM, 0x20a290, 0, wr_slot);
        /* @0x320B8–0x320BC: mov r5,g1; call object */
        g0 = geo_attract_copro_object(g0w, obj_ix, 0);
        if (g0 == 0)
            goto next_slot;

        /* @0x320C4–0x320FC: tag from record byte0 */
        tag = i960_ld_u8(I960_WORKRAM, rec_va, 0);
        tag &= 3u;
        tag &= (u8)r12;
        if (tag != 0) {
            g4w = i960_ld_u32(I960_WORKRAM, 0x20d7ec, 0);
            i960_st_u32(I960_WORKRAM, 0x20d7ec, 0, g4w - 1u);
        } else {
            g4w = i960_ld_u32(I960_WORKRAM, 0x20d7e8, 0);
            i960_st_u32(I960_WORKRAM, 0x20d7e8, 0, g4w - 1u);
        }
        /* @0x32100–0x32134: compact via r8 → r6, then bump done/remain */
        g6w = i960_ld_u32(I960_WORKRAM, 0x20d7e4, 0);
        g4w = i960_ld_u32(I960_WORKRAM, 0x20d7e0, 0);
        g5w = i960_ld_u32(I960_WORKRAM, r8_cur, 0);
        r8_cur -= 4u;
        i960_st_u32(I960_WORKRAM, r6, 0, g5w);
        i960_st_u32(I960_WORKRAM, 0x20d6f0, g6w << 2, obj_ix);
        i960_st_u32(I960_WORKRAM, 0x20d7e4, 0, g6w + 1u);
        i960_st_u32(I960_WORKRAM, 0x20d7e0, 0, g4w - 1u);

    next_slot:
        /* @0x3213C–0x32148: r7--; r6 -= 4; loop while r7 >= 0 */
        slot_i--;
        r6 -= 4u;
    } while (slot_i >= 0);
}
