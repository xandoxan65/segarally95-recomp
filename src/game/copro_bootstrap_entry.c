/* Copro FIFO bootstrap entry @ 0x4D50. */
// @rom 0x4d50 +0x8 copro_bootstrap_entry

#include "i960_lift.h"

extern void copro_fifo_init(u32 arg0, u32 arg1);

void copro_bootstrap_entry(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg2;
    copro_fifo_init(arg0, arg1);
}
