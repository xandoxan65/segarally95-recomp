/* Matrix row-swap + upload @ 0x33A50 — chase_alt helper.
 *
 * g0 → 12-float matrix (often a private-frame EA). Swaps selected rows,
 * clears T, uploads via TGP 0x22, 0x27(−T), 0x26 readback into the same
 * buffer.
 *
 * source: disasm/maincpu/maincpu_033a50_1ac.asm */
// @rom 0x33a50 +0x1ac geo_view_matrix_row_swap_upload

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_rom.h"

#include <string.h>

static int m_is_host(uintptr_t base)
{
    if (base > 0xffffffffull)
        return 1;
    return model2_ram_mut((u32)base) == NULL;
}

static u32 m_ld(uintptr_t base, u32 off)
{
    if (m_is_host(base)) {
        u32 v;

        memcpy(&v, (u8 *)base + off, 4);
        return v;
    }
    return i960_ld_u32(I960_ABS, (u32)base, off);
}

static void m_st(uintptr_t base, u32 off, u32 v)
{
    if (m_is_host(base)) {
        memcpy((u8 *)base + off, &v, 4);
        return;
    }
    i960_st_u32(I960_ABS, (u32)base, off, v);
}

void geo_view_matrix_row_swap_upload(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t m = g0;
    u32 t0, t1, t2;
    u32 a, b;
    unsigned i;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* m[1] ↔ m[3] */
    a = m_ld(m, 4);
    b = m_ld(m, 12);
    m_st(m, 12, a);
    m_st(m, 4, b);

    /* m[2] ↔ m[6] */
    a = m_ld(m, 8);
    b = m_ld(m, 24);
    m_st(m, 24, a);
    m_st(m, 8, b);

    /* save T = m[9..11]; m[5] ↔ m[7] */
    t0 = m_ld(m, 0x24);
    a = m_ld(m, 20);
    b = m_ld(m, 28);
    t1 = m_ld(m, 0x28);
    t2 = m_ld(m, 0x2c);
    m_st(m, 0x24, 0);
    m_st(m, 0x28, 0);
    m_st(m, 0x2c, 0);
    m_st(m, 20, b);
    m_st(m, 28, a);

    /* TGP 0x20; 0x25; 0x22 ← 12 floats. */
    i960_mmio_write_u32(0x884000, 0x10002020u);
    i960_mmio_write_u32(0x884000, 0x12802525u);
    i960_mmio_write_u32(0x884000, 0x11002222u);
    for (i = 0; i < 12u; i++)
        i960_mmio_write_u32(0x884000, m_ld(m, i * 4u));

    /* 0x27(−T) */
    i960_mmio_write_u32(0x884000, 0x13802727u);
    i960_mmio_write_u32(0x884000, t0 ^ 0x80000000u);
    i960_mmio_write_u32(0x884000, t1 ^ 0x80000000u);
    i960_mmio_write_u32(0x884000, t2 ^ 0x80000000u);

    /* 0x26 readback → m[0..11] */
    i960_mmio_write_u32(0x884000, 0x13002626u);
    for (i = 0; i < 12u; i++)
        m_st(m, i * 4u, i960_mmio_read_u32(0x884000));

    i960_mmio_write_u32(0x884000, 0x10802121u);
}
