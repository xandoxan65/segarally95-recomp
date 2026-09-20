/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_0005daa0_70.asm */
// @rom 0x5daa0 +0x70 libc_memcpy

#include "i960_lift.h"
#include "model2_rom.h"

#include <string.h>

/* abi: void * dst=g0, const void * src=g1, u32 len=g2 → void */

static u8 *libc_memcpy_dst(u32 vaddr)
{
    u8 *p;

    p = model2_ram_mut(vaddr);
    if (p)
        return p;
    p = (u8 *)(uintptr_t)model2_rom_at(vaddr);
    return p ? p : (u8 *)(uintptr_t)vaddr;
}

static const u8 *libc_memcpy_src(u32 vaddr)
{
    const u8 *p;

    p = model2_rom_at(vaddr);
    if (p)
        return p;
    p = model2_ram_mut(vaddr);
    return p ? p : (const u8 *)(uintptr_t)vaddr;
}

void libc_memcpy(void * dst, const void * src, u32 len)
{
    u32 dst_va;
    u32 src_va;
    u8 *d;
    const u8 *s;

    if (len == 0u)
        return;

    dst_va = (u32)(uintptr_t)dst;
    src_va = (u32)(uintptr_t)src;
    d = libc_memcpy_dst(dst_va);
    s = libc_memcpy_src(src_va);
    if (!d || !s)
        return;
    memcpy(d, s, (size_t)len);
}
