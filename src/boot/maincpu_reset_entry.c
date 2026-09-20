/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_000420_144.asm */
// @rom 0x420 +0x144 maincpu_reset_entry

#include "i960_lift.h"
#include "model2_rom.h"
#include "i960_mem.h"

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret  callee r5 */
/* abi: u32 arg0=g0, u32 arg1=g1, u32 arg2=g2 → void */
/* call site: caller 0x328 */

/* pointers: g13=void *, g4=u32, g5=u32 *, g6=u32 *, g8=void *, r5=void * */

void maincpu_reset_entry(u32 arg0, u32 arg1, u32 arg2)
{
    g0 = (uintptr_t)arg0;
    g1 = (uintptr_t)arg1;
    g2 = (uintptr_t)arg2;

    r5 = g8;
    g5 = (uintptr_t)(model2_maincpu_rom + 0x3e0); /* rom @0x3e0: "." */
    g6 = (uintptr_t)(model2_cpu_wait + 0x0);
    g8 = 0 - 1;
    do {
        g4 = *(u32 *)g5;
        *(u32 *)g6 = (u32)g4;
        g5 = g5 + 4;
        g4 = *(u32 *)g5;
        g6 = g6 + 4;
    } while ((unsigned char)g8 != g4);
    g4 = i960_mmio_read_u8(0xf80000); /* boot_mmio_gap */;
    g7 = 0;
    g6 = (uintptr_t)(model2_crx_ram + 0x1e40);
    g4 = 2 | g4;
    g5 = (uintptr_t)(model2_maincpu_rom + 0xb0); /* rom:boot_prcb */
    i960_mmio_write_u8(0xf80000, (u8)g4); /* boot_mmio_gap */;
    do {
        g4 = *(u32 *)g5;
        g7 = g7 + 1;
        r4 = 31 + 13;
        *(u32 *)g6 = (u32)g4;
        g6 = g6 + 4;
        g5 = g5 + 0x4;
    } while (g7 < r4);
    g7 = 0;
    g6 = (uintptr_t)(model2_crx_ram + 0x0);
    g5 = (uintptr_t)(model2_maincpu_rom + 0x1030);
    do {
        g4 = *(u32 *)g5;
        g7 = g7 + 1;
        g13 = 0x101;
        *(u32 *)g6 = (u32)g4;
        g6 = g6 + 4;
        g5 = g5 + 0x4;
    } while (g7 < g13);
    g0 = 0;
    g1 = 0;
    g1 = 0;
    g2 = 0;
    g8 = (uintptr_t)(model2_crx_ram + 0x0);
    g5 = (uintptr_t)(model2_workram + 0x0);
    g3 = 0;
    g4 = 0xffff;
    i960_st_u32(I960_WORKRAM, 0x201e54, 0, (u32)g8);
    do {
        g4 = g4 - 1;
        r4 = 0 - 1;
        *(u64 *)g5 = (u64)g0;
        g5 = g5 + 0x10;
    } while (g4 != r4);
    g7 = 0;
    g6 = (uintptr_t)(model2_workram + 0xa0000);
    g5 = (uintptr_t)(model2_maincpu_rom + 0x1000); /* rom @0x1000: "P!Z" */
    do {
        g4 = *(u32 *)g5;
        g7 = g7 + 1;
        g13 = 3 << 15;
        *(u32 *)g6 = (u32)g4;
        g6 = g6 + 4;
        g5 = g5 + 0x4;
    } while (g7 < g13);
    g8 = (uintptr_t)i960_vaddr_ptr(0xff000010);
    g13 = (uintptr_t)(model2_maincpu_rom + 0x3d0);
    i960_synmovq(g8, g13);
    g8 = r5;
    return;
}
