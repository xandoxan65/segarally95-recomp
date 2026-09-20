/* Slot word probe @ 0x2AED0 — guest VAs only (point base / record / obj+0x14).
 *
 * Pack @ 0x2B0B0 passes g0=point-table base, g1=12-byte record, g2=obj+0x14.
 * Visibility: four XZ 0x2e edge tests (obj.z, pi.z, pj.x, obj.x, pi.x, pj.z),
 * then and/or + cmpibg 0,g4 on the full word (same-sign → visible).
 *
 * source: disasm/maincpu/maincpu_02aed0_1e0.asm */
// @rom 0x2aed0 +0x1e0 geo_view_slot_word_probe

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

#include <string.h>

static u32 pts_xyz(u32 pts, u32 ix, u32 *x, u32 *y, u32 *z)
{
    u32 scaled = ix + (ix << 1);
    u32 ea = pts + (scaled << 2);

    *x = i960_ld_u32(I960_ABS, ea, 0);
    *y = i960_ld_u32(I960_ABS, ea, 4);
    *z = i960_ld_u32(I960_ABS, ea, 8);
    return ea;
}

/*
 * ABI: g0=point base, g1=record VA, g2=object+0x14 VA → g0 = 0/1 visible.
 * Private frame for 0x40(fp)…0x88(fp); do not alias caller's sp.
 */
u32 geo_view_slot_word_probe(u32 arg0, u32 arg1, void *arg2)
{
    u32 pts = arg0;
    u32 rec = arg1;
    u32 obj14 = (u32)(uintptr_t)arg2;
    u8 frame[0x90];
    uintptr_t fp_save = fp;
    u32 ix;
    u32 x0, y0, z0, x1, y1, z1, x2, y2, z2, x3, y3, z3;
    u32 ox, oy, oz;
    u32 r13v, r12v, g6v, g5v;
    u32 acc;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;

    ix = i960_ld_u16(I960_ABS, rec, 4);
    ox = i960_ld_u32(I960_ABS, obj14, 0);
    oy = i960_ld_u32(I960_ABS, obj14, 4);
    oz = i960_ld_u32(I960_ABS, obj14, 8);
    pts_xyz(pts, ix, &x0, &y0, &z0);

    ix = i960_ld_u16(I960_ABS, rec, 6);
    pts_xyz(pts, ix, &x1, &y1, &z1);

    ix = i960_ld_u16(I960_ABS, rec, 8);
    pts_xyz(pts, ix, &x2, &y2, &z2);

    ix = i960_ld_u16(I960_ABS, rec, 0xa);
    pts_xyz(pts, ix, &x3, &y3, &z3);

    /* fp mirrors used by the ldq bursts below. */
    *(u32 *)(frame + 0x40) = x0;
    *(u32 *)(frame + 0x44) = y0;
    *(u32 *)(frame + 0x48) = z0;
    *(u32 *)(frame + 0x50) = x1;
    *(u32 *)(frame + 0x54) = y1;
    *(u32 *)(frame + 0x58) = z1;
    *(u32 *)(frame + 0x60) = x2;
    *(u32 *)(frame + 0x64) = y2;
    *(u32 *)(frame + 0x68) = z2;
    *(u32 *)(frame + 0x70) = x3;
    *(u32 *)(frame + 0x74) = y3;
    *(u32 *)(frame + 0x78) = z3;
    *(u32 *)(frame + 0x80) = ox;
    *(u32 *)(frame + 0x84) = oy;
    *(u32 *)(frame + 0x88) = oz;

    /*
     * Four 0x2e XZ edge tests @ 0x2AF4C–0x2B078. Payload is always
     * (obj.z, pi.z, pj.x, obj.x, pi.x, pj.z) — never Y. Live regs / ldq
     * after fp stores yield the four edges p0→p1→p2→p3→p0.
     */
    i960_mmio_write_u32(0x884000, 0x2e005c5cu);
    i960_mmio_write_u32(0x884000, oz);
    i960_mmio_write_u32(0x884000, z0);
    i960_mmio_write_u32(0x884000, x1);
    i960_mmio_write_u32(0x884000, ox);
    i960_mmio_write_u32(0x884000, x0);
    i960_mmio_write_u32(0x884000, z1);
    r13v = i960_ld_u32(I960_MMIO, 0x884000, 0);

    i960_mmio_write_u32(0x884000, 0x2e005c5cu);
    i960_mmio_write_u32(0x884000, oz);
    i960_mmio_write_u32(0x884000, z1);
    i960_mmio_write_u32(0x884000, x2);
    i960_mmio_write_u32(0x884000, ox);
    i960_mmio_write_u32(0x884000, x1);
    i960_mmio_write_u32(0x884000, z2);
    r12v = i960_ld_u32(I960_MMIO, 0x884000, 0);

    i960_mmio_write_u32(0x884000, 0x2e005c5cu);
    i960_mmio_write_u32(0x884000, oz);
    i960_mmio_write_u32(0x884000, z2);
    i960_mmio_write_u32(0x884000, x3);
    i960_mmio_write_u32(0x884000, ox);
    i960_mmio_write_u32(0x884000, x2);
    i960_mmio_write_u32(0x884000, z3);
    g6v = i960_ld_u32(I960_MMIO, 0x884000, 0);

    i960_mmio_write_u32(0x884000, 0x2e005c5cu);
    i960_mmio_write_u32(0x884000, oz);
    i960_mmio_write_u32(0x884000, z3);
    i960_mmio_write_u32(0x884000, x0);
    i960_mmio_write_u32(0x884000, ox);
    i960_mmio_write_u32(0x884000, x3);
    i960_mmio_write_u32(0x884000, z0);
    g5v = i960_ld_u32(I960_MMIO, 0x884000, 0);

    /*
     * @0x2B080–0x2B0AC: and-chain then or-chain of the four 0x2e results.
     * cmpibg 0,g4 → branch when (i32)g4 < 0 (full-word sign, not low byte).
     * and-all-negative → visible (1); else or-any-negative → reject (0);
     * else visible (1).
     */
    acc = r13v & r12v;
    acc &= g6v;
    acc &= g5v;
    if ((i32)acc < 0) {
        g0 = 1;
        fp = fp_save;
        return 1;
    }
    acc = r13v | r12v;
    acc |= g6v;
    acc |= g5v;
    if ((i32)acc < 0) {
        g0 = 0;
        fp = fp_save;
        return 0;
    }
    g0 = 1;
    fp = fp_save;
    return 1;
}
