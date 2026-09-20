/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_0005cdc8_40.asm */
// @rom 0x5cdc8 +0x40 libc_strcpy

#include "i960_lift.h"

/* convention: kind=leaf_bx  args g0,g1  link g14→g2 bx */
/* abi: void * dst=g0, const void * src=g1 → void * g0 */

/* pointers: g0=void *, g1=const void *, g4=u32, g5=unsigned char *, g6=unsigned char * */

void * libc_strcpy(void * dst, const void * src)
{
    unsigned char * d = (unsigned char *)dst;
    const unsigned char * s = (const unsigned char *)src;

    g2 = g14;
    g14 = 0;
    g6 = (uintptr_t)s;
    if ((uintptr_t)d == 0)
        goto L_0005cde0;
    if ((unsigned char)*s != 0)
        goto L_0005cde8;

    L_0005cde0:
        g0 = 0;
        /* bx (g2) */
        return (void *)(uintptr_t)g0;

    L_0005cde8:
        g5 = (uintptr_t)d - 1;
        do {
            g4 = *(unsigned char *)s;
            g5 = g5 + 1;
            g6 = (uintptr_t)s + 0x1;
            *(unsigned char *)g5 = (unsigned char)g4;
        } while ((unsigned char)g4 != 0);
    /* bx (g2) */
    return (void *)d;
}
