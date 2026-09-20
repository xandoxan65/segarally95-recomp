/* Desert race nearest-point pick @ 0x41F80 — TGP 0x50 distance compares.
 *
 * ABI: g0 = query XYZ*, g1 = current index. Walks prev/curr/next ring slots
 * at 0x215c40[i*3], keeps the closest (max TGP readback). Returns index in g0.
 *
 * source: /tmp/dasm_41f80/maincpu_041f80_180.asm */
// @rom 0x41f80 +0x164 game_start_race_desert_near_pick

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

static u32 ring_slot(u32 idx)
{
    u32 scaled = idx + (idx << 1);

    return 0x215c40u + (scaled << 2);
}

u32 game_start_race_desert_near_pick(u32 query, u32 idx, u32 arg2)
{
    u32 count;
    i32 prev;
    u32 next;
    u32 best;
    u32 cur_d;
    u32 qx, qz;
    u32 px, pz;
    u32 slot;
    u32 d;

    (void)arg2;
    g14 = 0;

    count = i960_ld_u32(I960_WORKRAM, 0x216a50, 0);
    best = idx;
    prev = (i32)idx - 1;
    if (prev < 0)
        prev = prev + (i32)count;

    next = idx + 1u;
    if (next >= count)
        next -= count;

    qx = i960_ld_u32(I960_ABS, query, 0);
    qz = i960_ld_u32(I960_ABS, query, 8);
    slot = ring_slot(idx);
    px = i960_ld_u32(I960_WORKRAM, slot, 0);
    pz = i960_ld_u32(I960_WORKRAM, slot, 8);

    i960_mmio_write_u32(0x884000, 0x28005050u);
    i960_mmio_write_u32(0x884000, qx);
    i960_mmio_write_u32(0x884000, (u32)g14);
    i960_mmio_write_u32(0x884000, qz);
    i960_mmio_write_u32(0x884000, px);
    i960_mmio_write_u32(0x884000, (u32)g14);
    i960_mmio_write_u32(0x884000, pz);
    cur_d = i960_mmio_read_u32(0x884000);

    qx = i960_ld_u32(I960_ABS, query, 0);
    qz = i960_ld_u32(I960_ABS, query, 8);
    slot = ring_slot(next);
    px = i960_ld_u32(I960_WORKRAM, slot, 0);
    pz = i960_ld_u32(I960_WORKRAM, slot, 8);

    i960_mmio_write_u32(0x884000, 0x28005050u);
    i960_mmio_write_u32(0x884000, qx);
    i960_mmio_write_u32(0x884000, (u32)g14);
    i960_mmio_write_u32(0x884000, qz);
    i960_mmio_write_u32(0x884000, px);
    i960_mmio_write_u32(0x884000, (u32)g14);
    i960_mmio_write_u32(0x884000, pz);
    d = i960_mmio_read_u32(0x884000);
    /* cmpr cur_d,d; ble keep — update best only when cur_d > d. */
    if (i960_u32_to_f64(cur_d) > i960_u32_to_f64(d)) {
        best = next;
        /* @0x42070–0x42074: the taken keep branch skips both moves. */
        cur_d = d;
    }

    qx = i960_ld_u32(I960_ABS, query, 0);
    qz = i960_ld_u32(I960_ABS, query, 8);
    slot = ring_slot((u32)prev);
    px = i960_ld_u32(I960_WORKRAM, slot, 0);
    pz = i960_ld_u32(I960_WORKRAM, slot, 8);

    i960_mmio_write_u32(0x884000, 0x28005050u);
    i960_mmio_write_u32(0x884000, qx);
    i960_mmio_write_u32(0x884000, (u32)g14);
    i960_mmio_write_u32(0x884000, qz);
    i960_mmio_write_u32(0x884000, px);
    i960_mmio_write_u32(0x884000, (u32)g14);
    i960_mmio_write_u32(0x884000, pz);
    d = i960_mmio_read_u32(0x884000);
    if (i960_u32_to_f64(cur_d) > i960_u32_to_f64(d))
        best = (u32)prev;

    g0 = best;
    return best;
}
