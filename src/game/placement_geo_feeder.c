/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_023cc8_900.asm */
// @rom 0x23cc8 +0x900 placement_geo_feeder

#include "i960_lift.h"
#include "i960_fp.h"
#include "model2_rom.h"
#include "i960_mem.h"

/* convention: kind=leaf_ret  args g0,g1,g2  link g14 ret */
/* abi: void * arg0=g0, void * arg1=g1, u32 arg2=g2 → void * g0 */

/* pointers: fp=u32 *, g0=u32 *, g1=u32 *, g12=u32 *, g13=u32 *, g2=u32 *, g3=u32 *, g4=u32 *, g5=u32, g6=u32, g7=u32 *, g8=u32, r10=unsigned char *, r11=u32 *, r12=u32 *, r13=u32 *, r3=u32, r4=u32, r8=void *, r9=u32 *, sp=u64 * */

#include "lift_syms.h"

void * placement_geo_feeder(void * arg0, void * arg1, u32 arg2)
{
    u32 * arg0_p = (u32 *)arg0;
    u32 * a1 = (u32 *)arg1;

    /* data .long 0x00801008 — MAME misread; real code starts next. */
    if (!arg0 || !arg1)
        return arg0;
    g0 = (uintptr_t)arg0;
    g1 = (uintptr_t)arg1;
    g2 = (uintptr_t)arg2;
    g4 = i960_ld_u32(I960_WORKRAM, 0x202278, 0);
    g5 = *(u32 *)((uintptr_t)arg0_p + 0x4);
    g4 = 3 & g4;
    g4 = g4 << 10;
    i960_mmio_write_u32((u32)(g4 + 0x800010), (u32)g14);
    /* @0x23CEC: ldq catalog[ix*16] → r4..r7; movq/stq to PRG (not ldl/stl). */
    {
        u32 cat = 0x2864b40u + ((u32)g5 << 4);

        r4 = i960_ld_u32(I960_ROM, cat, 0);
        r5 = i960_ld_u32(I960_ROM, cat, 4);
        r6 = i960_ld_u32(I960_ROM, cat, 8);
        r7 = i960_ld_u32(I960_ROM, cat, 12);
    }
    g8 = r4;
    g9 = r5;
    g10 = r6;
    g11 = r7;
    i960_mmio_write_u32(0x804000, (u32)g8);
    i960_mmio_write_u32(0x804000, (u32)g9);
    i960_mmio_write_u32(0x804000, (u32)g10);
    i960_mmio_write_u32(0x804000, (u32)g11);
    r11 = (uintptr_t)i960_vaddr_ptr(0x10802121);
    i960_mmio_write_u32(0x884000, (u32)r11); /* copro_fifo */;
    r8 = (uintptr_t)i960_vaddr_ptr(0x10002020);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    g4 = *(u32 *)a1;
    g5 = *(u32 *)((uintptr_t)a1 + 0x4);
    g6 = *(u32 *)((uintptr_t)a1 + 0x8);
    r9 = (uintptr_t)i960_vaddr_ptr(0x13802727);
    i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g6); /* copro_fifo */;
    g5 = *(u32 *)((uintptr_t)a1 + 0x1c);
    r10 = (uintptr_t)i960_vaddr_ptr(0x15002a2a);
    i960_mmio_write_u32(0x884000, (u32)r10); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
    g4 = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
    g5 = *(u32 *)((uintptr_t)a1 + 0x18);
    r11 = (uintptr_t)i960_vaddr_ptr(0x14802929);
    i960_mmio_write_u32(0x884000, (u32)r11); /* copro_fifo */;
    g4 = g4 + r7;
    i960_st_u32(I960_WORKRAM, 0x20b940, 0, (u32)g4);
    i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
    g5 = *(u32 *)((uintptr_t)a1 + 0x20);
    r8 = (uintptr_t)i960_vaddr_ptr(0x15802b2b);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    g4 = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
    i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
    g5 = i960_ld_u32(I960_MMIO, 0x802008, 0); /* geo_write_start_read */;
    i960_st_u32(I960_WORKRAM, (u32)g4, 0, (u32)g5);
    r9 = (uintptr_t)i960_vaddr_ptr(0x2800505);
    i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
    g4 = g4 + 4;
    i960_st_u32(I960_WORKRAM, 0x20a290, 0, (u32)g4);
    g5 = g5 + 0x34;
    i960_mmio_write_u32(0x801008, (u32)g5); /* geo_write_start */;
    /*
     * Body after this walks caller frame (fp+0x80/0xa0/0xb0, g12…). Attract
     * carousel uses course_catalog_span instead. Exit via the ROM epilogue
     * pop @ 0x24084 so the object push/pop pair stays balanced; skip
     * object_extra (needs live fp+0xb0).
     */
    r11 = (uintptr_t)i960_vaddr_ptr(0x10802121);
    i960_mmio_write_u32(0x884000, (u32)r11); /* copro_fifo pop */
    return arg0;
    g4 = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
    r10 = *(u32 *)(fp + 0x80);
    g2 = (uintptr_t)arg2 - g4;
    r5 = r10 ^ (1u << 31);
    if (1 > (signed char)g2) {
        g2 = 1;
    }
    g4 = i960_ld_u32(I960_WORKRAM, 0x202278, 0);
    g4 = g4 & 3;
    g4 = g4 << 10;
    i960_mmio_write_u32((u32)(g4 + 0x800010), (u32)g14);
    r11 = *(u32 *)(fp + 0xa0);
    g4 = i960_ld_u64(I960_ROM, 0x2864b40, (u32)(r11 << 4));
    *(u32 *)(fp + 0x60) = (u32)g4;
    *(u32 *)(fp + 0x64) = (u32)g5;
    *(u32 *)(fp + 0x68) = (u32)g6;
    if (g2 == 0)
        goto L_00023e54;
    if ((u32)g7 >= (u32)g2)
        goto L_00023e58;

    L_00023e54:
        g2 = g7;

    L_00023e58:
        *(u32 *)(fp + 0x6c) = (u32)g2;
        r8 = (u32)*((u64 *)(fp + 0x60));
        g4 = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
        i960_st_u64(I960_MMIO, 0x804000, 0, (u64)r8);
        r9 = *(u32 *)(fp + 0x50);
        g4 = g4 + g2;
        i960_st_u32(I960_WORKRAM, 0x20b940, 0, (u32)g4);
        g0 = *(unsigned char *)((uintptr_t)a1 + 0x50);
        if (r9 == 0)
            goto L_00023e98;
        g0 = 15 & g0;
        geo_attract_pen_prg_push_a((u32)g0, 0, 0);
    goto L_00023eb0;

    L_00023e98:
        g3 = 15 & g0;
        g0 = (uintptr_t)a1;
        g1 = (uintptr_t)a1 + 0x18;
        g2 = 0x20220c;
        geo_attract_pen_prg_push_b((uintptr_t)g0, (uintptr_t)g1, (u32)g2);

    L_00023eb0:
        r10 = *(u32 *)(fp + 0xb0);
        g4 = *(unsigned char *)(r10 + 0x50);
        if (!((g4 >> 4) & 1))
            goto L_00023f18;
        g5 = *(u32 *)(g12 + 0x14);
        if ((unsigned char)g5 != 0) {
            g4 = i960_ld_u32(I960_WORKRAM, 0x202278, 0);
            g4 = g4 & 3;
            g4 = g4 << 10;
            i960_mmio_write_u32((u32)(g4 + 0x800010), (u32)g14);
            g0 = i960_ld_u64(I960_ROM, 0x2864b40, (u32)(g5 << 4));
            *(u32 *)(fp + 0x70) = (u32)g0;
            *(u32 *)(fp + 0x74) = (u32)g1;
            *(u32 *)(fp + 0x78) = (u32)g2;
            g4 = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
            *(u32 *)(fp + 0x7c) = (u32)g3;
            r8 = (u32)*((u64 *)(fp + 0x70));
            g4 = g4 + g3;
            i960_st_u32(I960_WORKRAM, 0x20b940, 0, (u32)g4);
            i960_st_u64(I960_MMIO, 0x804000, 0, (u64)r8);
        }

    L_00023f18:
        g4 = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
        g13 = 0;
        g4 = r3 - g4;
        if (1 < (signed char)g4) {
            r9 = *(u32 *)(fp + 0x90);
            r4 = *(u32 *)(fp + 0xb0);
            g7 = r9 + 4;
        }

    L_00023f38:
        r10 = (uintptr_t)i960_vaddr_ptr(0x10002020);
        i960_mmio_write_u32(0x884000, (u32)r10); /* copro_fifo */;
        g5 = *(u32 *)g7;
        g4 = *(u32 *)(r4 + 0x30);
        r11 = *(u32 *)(fp + 0x90);
        g5 = i960_f64_to_u32((i960_u32_to_f64(g4)) + (i960_u32_to_f64(g5)));
        g4 = *(u32 *)(g7 + 0x4);
        g6 = *(u32 *)r11;
        r8 = (uintptr_t)i960_vaddr_ptr(0x13802727);
        i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g6); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        if (g13 <= 1) {
            r9 = *(u32 *)(fp + 0xb0);
            g4 = *(u32 *)(r9 + 0x24);
            r10 = (uintptr_t)i960_vaddr_ptr(0x15002a2a);
            i960_mmio_write_u32(0x884000, (u32)r10); /* copro_fifo */;
            i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        }
    r11 = (uintptr_t)i960_vaddr_ptr(0x14802929);
    i960_mmio_write_u32(0x884000, (u32)r11); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)r5); /* copro_fifo */;
    g6 = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
    g4 = i960_ld_u32(I960_MMIO, 0x802008, 0); /* geo_write_start_read */;
    i960_st_u32(I960_WORKRAM, (u32)g6, 0, (u32)g4);
    r8 = (uintptr_t)i960_vaddr_ptr(0x2800505);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    g4 = g4 + 0x34;
    i960_mmio_write_u32(0x801008, (u32)g4); /* geo_write_start */;
    g4 = i960_ld_u32(I960_WORKRAM, 0x202278, 0);
    g5 = *(u32 *)(g12 + 0x8);
    g4 = 3 & g4;
    g4 = g4 << 10;
    i960_mmio_write_u32((u32)(g4 + 0x800010), (u32)g14);
    g0 = i960_ld_u64(I960_ROM, 0x2864b40, (u32)(g5 << 4));
    g7 = g7 + 12;
    r9 = *(u32 *)(fp + 0x90);
    r9 = r9 + 12;
    g4 = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
    r12 = g0;
    r13 = g1;
    r4 = r4 + 0x4;
    i960_st_u64(I960_MMIO, 0x804000, 0, (u64)r12);
    r10 = (uintptr_t)i960_vaddr_ptr(0x10802121);
    *(u32 *)(fp + 0x90) = (u32)r9;
    g13 = g13 + 1;
    i960_mmio_write_u32(0x884000, (u32)r10); /* copro_fifo */;
    g6 = g6 + 4;
    i960_st_u32(I960_WORKRAM, 0x20a290, 0, (u32)g6);
    g3 = g3 + g4;
    i960_st_u32(I960_WORKRAM, 0x20b940, 0, (u32)g3);
    if (g13 > 3)
        goto L_00024084;
    g4 = r3 - g3;
    if (1 < (signed char)g4)
        goto L_00023f38;

    L_00024084:
        g4 = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
        r11 = (uintptr_t)i960_vaddr_ptr(0x10802121);
        i960_mmio_write_u32(0x884000, (u32)r11); /* copro_fifo */;
        g4 = r3 - g4;
        if (5 >= (signed char)g4)
            goto L_000240ac;
        g0 = *(u32 *)(fp + 0xb0);
        geo_attract_object_extra((void *)(uintptr_t)g0, g1, g2);

    L_000240ac:
        g8 = (u32)*((u64 *)(sp - 0x20));
        g12 = *(u32 *)(sp - 0x10);
        return (void *)(uintptr_t)g0;
    sp = sp + 0x20;
    *(u64 *)(sp - 0x10) = (u64)g8;
    r3 = *(u32 *)g0;
    r8 = (uintptr_t)i960_vaddr_ptr(0x10002020);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    r4 = g1 + 0x40;
    g7 = g1 + 0x44;
    g4 = *(u32 *)r4;
    g5 = *(u32 *)g7;
    g13 = g1 + 0x48;
    g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) + (i960_u32_to_f64(g4)));
    g5 = *(u32 *)g13;
    g3 = g1 + 0x4c;
    g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) + (i960_u32_to_f64(g4)));
    g5 = *(u32 *)g3;
    g4 = i960_f64_to_u32((i960_u32_to_f64(g5)) + (i960_u32_to_f64(g4)));
    fp0 = i960_u32_to_f64(g4);
    r10 = 0;
    r11 = (uintptr_t)i960_vaddr_ptr(0x3fd00000);
    fp1 = i960_rifl_read(r10, r11);
    i960_rifl_write(&r8, &r9, (fp1) * (fp0));
    fp0 = i960_rifl_read(r8, r9);
    r9 = i960_f64_to_u32(fp0);
    fp1 = i960_u32_to_f64(r9);
    r10 = (uintptr_t)i960_vaddr_ptr(0x47ae147b);
    r11 = (uintptr_t)i960_vaddr_ptr(0x3f947ae1);
    fp0 = i960_u32_to_f64(r9);
    fp0 = i960_rifl_read(r10, r11);
    i960_rifl_write(&r8, &r9, (fp1) - (fp0));
    g5 = *(u32 *)g1;
    g6 = *(u32 *)(g1 + 0x8);
    fp1 = i960_rifl_read(r8, r9);
    r9 = (uintptr_t)i960_vaddr_ptr(0x13802727);
    i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
    g4 = i960_f64_to_u32(fp1);
    i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g6); /* copro_fifo */;
    g4 = *(u32 *)(g1 + 0x1c);
    r10 = (uintptr_t)i960_vaddr_ptr(0x15002a2a);
    i960_mmio_write_u32(0x884000, (u32)r10); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g5 = *(u32 *)r4;
    g7 = *(u32 *)g7;
    g4 = *(u32 *)g13;
    g6 = *(u32 *)g3;
    g5 = i960_f64_to_u32((i960_u32_to_f64(g7)) + (i960_u32_to_f64(g5)));
    g4 = i960_f64_to_u32((i960_u32_to_f64(g6)) + (i960_u32_to_f64(g4)));
    g5 = i960_f64_to_u32((i960_u32_to_f64(g5)) - (i960_u32_to_f64(g4)));
    fp0 = i960_u32_to_f64(g5);
    r8 = (uintptr_t)i960_vaddr_ptr(0xb4395810);
    r9 = (uintptr_t)i960_vaddr_ptr(0x3fce76c8);
    fp1 = i960_rifl_read(r8, r9);
    i960_rifl_write(&r10, &r11, (fp1) * (fp0));
    fp0 = i960_rifl_read(r10, r11);
    g4 = i960_f64_to_u32(fp0);
    r11 = (uintptr_t)i960_vaddr_ptr(0x14802929);
    i960_mmio_write_u32(0x884000, (u32)r11); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g4 = *(u32 *)(g1 + 0x20);
    r8 = (uintptr_t)i960_vaddr_ptr(0x15802b2b);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    g6 = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g4 = i960_ld_u32(I960_MMIO, 0x802008, 0); /* geo_write_start_read */;
    i960_st_u32(I960_WORKRAM, (u32)g6, 0, (u32)g4);
    r9 = (uintptr_t)i960_vaddr_ptr(0x2800505);
    i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
    g4 = g4 + 0x34;
    i960_mmio_write_u32(0x801008, (u32)g4); /* geo_write_start */;
    g4 = i960_ld_u32(I960_WORKRAM, 0x202278, 0);
    g5 = *(u32 *)(g0 + 0x4);
    g4 = 3 & g4;
    g4 = g4 << 10;
    i960_mmio_write_u32((u32)(g4 + 0x800010), (u32)g14);
    r4 = i960_ld_u64(I960_ROM, 0x2864b40, (u32)(g5 << 4));
    g4 = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
    g6 = g6 + 4;
    i960_st_u32(I960_WORKRAM, 0x20a290, 0, (u32)g6);
    g8 = r4;
    g4 = g4 + r7;
    g9 = r5;
    i960_st_u32(I960_WORKRAM, 0x20b940, 0, (u32)g4);
    g10 = r6;
    i960_st_u64(I960_MMIO, 0x804000, 0, (u64)g8);
    r10 = (uintptr_t)i960_vaddr_ptr(0x10802121);
    i960_mmio_write_u32(0x884000, (u32)r10); /* copro_fifo */;
    r11 = (uintptr_t)i960_vaddr_ptr(0x10002020);
    i960_mmio_write_u32(0x884000, (u32)r11); /* copro_fifo */;
    g4 = *(u32 *)g1;
    g6 = *(u32 *)(g1 + 0x4);
    g7 = *(u32 *)(g1 + 0x8);
    r8 = (uintptr_t)i960_vaddr_ptr(0x13802727);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g6); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g7); /* copro_fifo */;
    g4 = *(u32 *)(g1 + 0x1c);
    r9 = (uintptr_t)i960_vaddr_ptr(0x15002a2a);
    i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g4 = *(u32 *)(g1 + 0x18);
    r10 = (uintptr_t)i960_vaddr_ptr(0x14802929);
    i960_mmio_write_u32(0x884000, (u32)r10); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g4 = *(u32 *)(g1 + 0x20);
    r11 = (uintptr_t)i960_vaddr_ptr(0x15802b2b);
    i960_mmio_write_u32(0x884000, (u32)r11); /* copro_fifo */;
    g5 = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g4 = i960_ld_u32(I960_MMIO, 0x802008, 0); /* geo_write_start_read */;
    *(u32 *)g5 = (u32)g4;
    r8 = (uintptr_t)i960_vaddr_ptr(0x2800505);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    g6 = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
    g5 = g5 + 4;
    i960_st_u32(I960_WORKRAM, 0x20a290, 0, (u32)g5);
    g4 = g4 + 0x34;
    i960_mmio_write_u32(0x801008, (u32)g4); /* geo_write_start */;
    g6 = g2 - g6;
    if (1 > (signed char)g6) {
        g6 = 1;
    }
    g4 = i960_ld_u32(I960_WORKRAM, 0x202278, 0);
    g5 = *(u32 *)(g0 + 0xc);
    g4 = 3 & g4;
    g4 = g4 << 10;
    i960_mmio_write_u32((u32)(g4 + 0x800010), (u32)g14);
    r4 = i960_ld_u64(I960_ROM, 0x2864b40, (u32)(g5 << 4));
    *(u32 *)(fp + 0x40) = (u32)r4;
    *(u32 *)(fp + 0x44) = (u32)r5;
    *(u32 *)(fp + 0x48) = (u32)r6;
    if (g6 == 0)
        goto L_0002442c;
    g5 = g6;
    if (r7 >= g6) {
        *(u32 *)(fp + 0x4c) = (u32)g5;
        r8 = (u32)*((u64 *)(fp + 0x40));
        g13 = g1;
        g4 = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
        g3 = r3 + 4;
        i960_st_u64(I960_MMIO, 0x804000, 0, (u64)r8);
        r8 = (uintptr_t)i960_vaddr_ptr(0x2800505);
        r10 = (uintptr_t)i960_vaddr_ptr(0x13802727);
        r11 = (uintptr_t)i960_vaddr_ptr(0x15002a2a);
        g4 = g4 + g5;
        g7 = 0;
        i960_st_u32(I960_WORKRAM, 0x20b940, 0, (u32)g4);
    } else {
        g5 = r7;
    }

    L_0002442c:
        r9 = (uintptr_t)i960_vaddr_ptr(0x10002020);
        i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
        g5 = *(u32 *)g3;
        g4 = *(u32 *)(g13 + 0x30);
        g6 = *(u32 *)r3;
        g5 = i960_f64_to_u32((i960_u32_to_f64(g4)) + (i960_u32_to_f64(g5)));
        g4 = *(u32 *)(g3 + 0x4);
        i960_mmio_write_u32(0x884000, (u32)r10); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g6); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
        i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        if (g7 <= 1) {
            g4 = *(u32 *)(g1 + 0x24);
            i960_mmio_write_u32(0x884000, (u32)r11); /* copro_fifo */;
            i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
        }
    g6 = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
    g4 = i960_ld_u32(I960_MMIO, 0x802008, 0); /* geo_write_start_read */;
    i960_st_u32(I960_WORKRAM, (u32)g6, 0, (u32)g4);
    i960_mmio_write_u32(0x884000, (u32)r8); /* copro_fifo */;
    g4 = g4 + 0x34;
    i960_mmio_write_u32(0x801008, (u32)g4); /* geo_write_start */;
    g4 = i960_ld_u32(I960_WORKRAM, 0x202278, 0);
    g5 = *(u32 *)(g0 + 0x8);
    g4 = 3 & g4;
    g4 = g4 << 10;
    i960_mmio_write_u32((u32)(g4 + 0x800010), (u32)g14);
    r4 = i960_ld_u64(I960_ROM, 0x2864b40, (u32)(g5 << 4));
    g3 = g3 + 12;
    r3 = r3 + 0xc;
    g4 = i960_ld_u32(I960_WORKRAM, 0x20b940, 0);
    r12 = r4;
    r13 = r5;
    g13 = g13 + 0x4;
    i960_st_u64(I960_MMIO, 0x804000, 0, (u64)r12);
    r9 = (uintptr_t)i960_vaddr_ptr(0x10802121);
    g7 = g7 + 1;
    i960_mmio_write_u32(0x884000, (u32)r9); /* copro_fifo */;
    g6 = g6 + 4;
    i960_st_u32(I960_WORKRAM, 0x20a290, 0, (u32)g6);
    g4 = g4 + r7;
    i960_st_u32(I960_WORKRAM, 0x20b940, 0, (u32)g4);
    if (g7 <= 3)
        goto L_0002442c;
    r10 = r9;
    g2 = g2 - g4;
    i960_mmio_write_u32(0x884000, (u32)r10); /* copro_fifo */;
    if (5 >= (signed char)g2)
        goto L_0002454c;
    g0 = g1;
    geo_attract_object_extra((void *)(uintptr_t)g0, g1, g2);

    L_0002454c:
        g8 = (u32)*((u64 *)(sp - 0x10));
        return (void *)(uintptr_t)g0;
    /* data .long 0x00000000 */
    /* data .long 0x00000000 */
    g3 = *(u32 *)g0;
    r13 = (uintptr_t)i960_vaddr_ptr(0x10002020);
    i960_mmio_write_u32(0x884000, (u32)r13); /* copro_fifo */;
    g4 = *(u32 *)g1;
    g5 = *(u32 *)(g1 + 0x4);
    g6 = *(u32 *)(g1 + 0x8);
    r12 = (uintptr_t)i960_vaddr_ptr(0x13802727);
    i960_mmio_write_u32(0x884000, (u32)r12); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g5); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g6); /* copro_fifo */;
    g4 = *(u32 *)(g1 + 0x1c);
    r13 = (uintptr_t)i960_vaddr_ptr(0x15002a2a);
    i960_mmio_write_u32(0x884000, (u32)r13); /* copro_fifo */;
    i960_mmio_write_u32(0x884000, (u32)g4); /* copro_fifo */;
    g4 = *(u32 *)(g1 + 0x18);
}
