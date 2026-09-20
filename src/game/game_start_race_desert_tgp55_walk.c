/* Desert race TGP 0x55 list walk @ 0x41CA0 — emit scaled markers for active slots.
 *
 * Walks 0x216a88 entries (stride 0x1c) while entry float > −1.5, issuing TGP
 * 0x55 + GEO 0x05 bind and stq catalog @ 0x28655c0. Called every node frame
 * after the main catalogs (inactive path @ 0x43320).
 *
 * source: /tmp/dasm_41ca0b/maincpu_041ca0_300.asm */
// @rom 0x41ca0 +0x1c8 game_start_race_desert_tgp55_walk

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_rom.h"
#include "model2_memory.h"

#include <string.h>

static void prg_stq_vaddr(u32 vaddr)
{
    const u8 *p = model2_rom_at(vaddr);
    u32 w0, w1, w2, w3;
    u32 bank;

    if (!p)
        return;
    memcpy(&w0, p + 0, 4);
    memcpy(&w1, p + 4, 4);
    memcpy(&w2, p + 8, 4);
    memcpy(&w3, p + 12, 4);
    bank = i960_ld_u32(I960_WORKRAM, 0x202278, 0) & 3u;
    i960_mmio_write_u32(0x800010u + (bank << 10), 0u);
    i960_mmio_write_u32(GEO_PRG_FIFO, w0);
    i960_mmio_write_u32(GEO_PRG_FIFO, w1);
    i960_mmio_write_u32(GEO_PRG_FIFO, w2);
    i960_mmio_write_u32(GEO_PRG_FIFO, w3);
    i960_st_u32(I960_WORKRAM, 0x20b940, 0,
                i960_ld_u32(I960_WORKRAM, 0x20b940, 0) + w3);
}

void game_start_race_desert_tgp55_walk(u32 arg0, u32 arg1, u32 arg2)
{
    u32 g8_save = (u32)g8;
    u32 entry = 0x216a88u;
    u32 pos_a = entry - 16u;
    u32 pos_b = entry - 24u;
    u32 idx = 0;
    double neg15 = i960_rifl_read(0, 0xbff80000u);
    double scale_k = i960_rifl_read(0xeb851eb8u, 0x3faeb851u);

    (void)arg0;
    (void)arg1;
    (void)arg2;
    g14 = 0;

    for (;;) {
        double v = i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, entry, 0));

        if (v <= neg15)
            break;
        if (entry > 0x21716cu)
            break;

        {
            u32 mag = i960_ld_u32(I960_WORKRAM, entry, 0) & 0x7fffffffu;
            u32 p0 = i960_ld_u32(I960_WORKRAM, pos_b, 0);
            u32 py = i960_ld_u32(I960_WORKRAM, 0x216a74, idx << 2);
            u32 p1 = i960_ld_u32(I960_WORKRAM, pos_a, 0);
            u32 slot = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
            u32 wr = i960_mmio_read_u32(0x802008);
            u32 eye_x = i960_ld_u32(I960_WORKRAM, 0x20220c, 0);
            u32 eye_z = i960_ld_u32(I960_WORKRAM, 0x202214, 0);
            u32 scale;

            if (slot != 0)
                i960_st_u32(I960_WORKRAM, slot, 0, wr);

            i960_mmio_write_u32(0x884000, 0x2a805555u);
            i960_mmio_write_u32(0x884000, p0);
            i960_mmio_write_u32(0x884000, py);
            i960_mmio_write_u32(0x884000, p1);
            i960_mmio_write_u32(0x884000, eye_x);
            i960_mmio_write_u32(0x884000, p0);
            i960_mmio_write_u32(0x884000, eye_z);
            i960_mmio_write_u32(0x884000, p1);

            scale = (u32)i960_f64_to_u32(2.0 - i960_u32_to_f64(mag));
            i960_mmio_write_u32(0x884000, 0x3fb33333u); /* 0.1f-ish const in r3 */
            i960_mmio_write_u32(0x884000, scale);
            i960_mmio_write_u32(0x884000, 0x3f800000u); /* +1.0 */

            i960_st_u32(I960_WORKRAM, 0x20a290, 0, slot + 4u);
            i960_mmio_write_u32(0x801008, wr + 0x34u);

            {
                double cur = i960_u32_to_f64(i960_ld_u32(I960_WORKRAM, entry, 0));
                u32 bank = i960_ld_u32(I960_WORKRAM, 0x202278, 0) & 3u;
                u32 next = (u32)i960_f64_to_u32(cur - scale_k);

                i960_mmio_write_u32(0x800010u + (bank << 10), 0u);
                prg_stq_vaddr(0x028655c0u);
                i960_st_u32(I960_WORKRAM, entry, 0, next);
            }
        }

        entry += 28u;
        idx += 28u;
        pos_a += 28u;
        pos_b += 28u;
        if ((i32)entry > (i32)0x21716c)
            break;
    }

    g8 = g8_save;
}
