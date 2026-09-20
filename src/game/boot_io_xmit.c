/* Boot comm-board glyph xmit trampoline @ 0x1DB8 (bx through workram pointer). */
// @rom 0x1db8 +0x10 boot_io_xmit

#include "i960_lift.h"

void boot_io_xmit(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;
    /* Host: display board handler not modeled; byte already in 0x01C00000. */
}
