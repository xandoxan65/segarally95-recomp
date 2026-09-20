/* Semantic C — guest VAs via i960_ld/st only (no raw host deref).
 * source: disasm/maincpu/maincpu_02abc0_100.asm */
// @rom 0x2abc0 +0x100 geo_view_table_index_a

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_fp.h"
#include "model2_rom.h"
#include "model2_tgp.h"
#include "i960_mem.h"

#include <stdio.h>
#include <string.h>

/* abi: void * arg0=course (g0), u32 arg1=index (g1), void * arg2=object XYZ guest VA (g2)
 * Implicit: g3 (flags word, shifted <<1 into payload). */

static int xyz_is_host(uintptr_t p)
{
    /*
     * Scene_prep / scene_frame pass private-frame EAs. Truncating to u32 can
     * land inside a mapped guest window — never treat a high-bit pointer as
     * guest (same rule as geo_view_copro_vec_push).
     */
    if (p > 0xffffffffull)
        return 1;
    return model2_ram_mut((u32)p) == NULL;
}

static u32 obj_ld_u32(u32 obj_va, u32 off, void *host)
{
    if (host != NULL) {
        u32 v;

        memcpy(&v, (u8 *)host + off, 4);
        return v;
    }
    return i960_ld_u32(I960_ABS, obj_va, off);
}

void geo_view_table_index_a(void *arg0, u32 arg1, void *arg2)
{
    u32 course = (u32)(uintptr_t)arg0;
    u32 index = arg1;
    uintptr_t obj_p = (uintptr_t)arg2;
    u32 obj_va = (u32)obj_p;
    u8 *obj_host = xyz_is_host(obj_p) ? (u8 *)arg2 : NULL;
    u32 table_ptr;
    u32 table_word0;
    u32 table_word1;
    u32 slot;
    u32 count;
    i32 prev;
    i32 next;
    u32 ox, oy, oz;
    u32 cursor;

    /* lda (g0)[g0*2] → course*3; lda 0x5dcac0[g0*4]. */
    table_ptr = 0x5dcac0u + ((course * 3u) << 2);
    table_word0 = i960_ld_u32(I960_WORKRAM, table_ptr, 0);
    table_word1 = i960_ld_u32(I960_WORKRAM, table_ptr, 4);

    slot = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
    i960_st_u32(I960_ABS, slot, 0, table_word0);
    /* Host 0x20a290 may be a GEO leftover; arm the typed row word0. */
    model2_tgp_arm_course_handle(table_word0);
    i960_mmio_write_u32(0x884000, 0x03800707u);
    slot += 4u;
    i960_st_u32(I960_WORKRAM, 0x20a290, 0, slot);

    count = i960_mmio_read_u32(0x884000);

    /* Early ret: index < 0 || index >= count. */
    if ((i32)index < 0 || (i32)index >= (i32)count) {
        static int skip_logged;

        if (skip_logged < 4) {
            lift_log(
                    "lift: table_index_a skip 0x52 idx=%d count=%u cursor=%#x\n",
                    (int)(i32)index, (unsigned)count, (unsigned)slot);
            fflush(stderr);
            skip_logged++;
        }
        return;
    }

    prev = (i32)index - 1;
    if (prev < 0)
        prev += (i32)count;
    next = (i32)index + 1;
    if (next >= (i32)count)
        next -= (i32)count;

    cursor = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
    ox = obj_ld_u32(obj_va, 0, obj_host);
    oy = obj_ld_u32(obj_va, 4, obj_host);
    oz = obj_ld_u32(obj_va, 8, obj_host);

    /* @0x2AC2C–0x2ACB0: 9 words (0..0x20) then TGP 0x29005252; slot += 0x24. */
    i960_st_u32(I960_ABS, cursor + 0x00u, 0, ox);
    i960_st_u32(I960_ABS, cursor + 0x04u, 0, oy);
    i960_st_u32(I960_ABS, cursor + 0x08u, 0, oz);
    i960_st_u32(I960_ABS, cursor + 0x0cu, 0, table_word1);
    i960_st_u32(I960_ABS, cursor + 0x10u, 0, count << 2);
    i960_st_u32(I960_ABS, cursor + 0x14u, 0, (u32)prev << 2);
    i960_st_u32(I960_ABS, cursor + 0x18u, 0, index << 2);
    i960_st_u32(I960_ABS, cursor + 0x1cu, 0, (u32)next << 2);
    i960_st_u32(I960_ABS, cursor + 0x20u, 0, (u32)g3 << 1);

    {
        u32 blk[9];

        blk[0] = ox;
        blk[1] = oy;
        blk[2] = oz;
        blk[3] = table_word1;
        blk[4] = count << 2;
        blk[5] = (u32)prev << 2;
        blk[6] = index << 2;
        blk[7] = (u32)next << 2;
        blk[8] = (u32)g3 << 1;
        model2_tgp_arm_52_block(blk);
    }

    i960_mmio_write_u32(0x884000, 0x29005252u);
    /* g5 was cursor+4; lda 0x20(g5) → cursor+0x24. */
    i960_st_u32(I960_WORKRAM, 0x20a290, 0, cursor + 0x24u);
    {
        static int run_logged;
        union {
            u32 u;
            float f;
        } fx, fy, fz;

        if (run_logged < 6) {
            fx.u = ox;
            fy.u = oy;
            fz.u = oz;
            lift_log(
                    "lift: table_index_a 0x52 idx=%u count=%u cursor=%#x "
                    "q=(%.4g,%.4g,%.4g) host=%d\n",
                    (unsigned)index, (unsigned)count, (unsigned)cursor,
                    fx.f, fy.f, fz.f, obj_host != NULL);
            fflush(stderr);
            run_logged++;
        }
    }
}
