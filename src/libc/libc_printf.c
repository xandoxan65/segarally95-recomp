/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_0005cec0_50.asm */
// @rom 0x5cec0 +0x50 libc_printf

#include "i960_lift.h"

/* frame function */

/* convention: kind=frame  args g0,g1,g2  link g14→g13 ret  caller r4,r5  callee r4,r5 */
/* abi: const char * fmt=g0, u32 arg1=g1, u32 arg2=g2 → void */
/* abi note: Additional varargs marshalled via fp+0x70 / g13 va-block; g1/g2 usage varies by call site. */
/* call site: caller 0x4d6c, g0=rom:0x5a3d30 */
/* call site: caller 0x5744, g0=rom:0x5a4328, g1=r4 */
/* call site: caller 0x5ca4 */

/* pointers: fp=u32 *, g0=const char *, g13=u64 *, r4=u64 * */

extern void libc_printf_dispatch(void);

void libc_printf(const char * fmt, u32 arg1, u32 arg2)
{
    static u64 printf_va_scratch[4];
    uintptr_t va_base;
    uintptr_t sp_save = sp;
    uintptr_t fp_save = fp;

    g0 = (uintptr_t)fmt;
    g1 = (uintptr_t)arg1;
    g2 = (uintptr_t)arg2;

    r6 = g8;
    r8 = g10;
    /*
     * Host shares one global fp/sp (i960 would allocate a fresh frame on call).
     * Always marshal through scratch — writing fp+0x40/0x70 into a small private
     * frame or into the 8KB boot host_workram stack is a SIGBUS after a few
     * calls (dispatch also bumps sp without restoring).
     */
    fp = 0;
    sp = sp + 0x40;
    g14 = 0;
    va_base = (uintptr_t)printf_va_scratch;
    g13 = va_base;
    *(u64 *)(va_base + 0x10) = (u64)g4;
    *(u64 *)va_base = (u64)(uintptr_t)fmt;
    r5 = 4;
    *(u64 *)(va_base + 0x20) = (u64)g8;
    r4 = va_base;
    libc_printf_dispatch();
    g8 = r6;
    g10 = r8;
    fp = fp_save;
    sp = sp_save;
    return;
}
