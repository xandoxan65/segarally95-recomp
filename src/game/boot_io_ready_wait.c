/* Spin on I/O board status bit 5 @ 0x1C00002, then indirect return. */
// @rom 0x1dd8 +0x18 boot_io_ready_wait

#include "i960_lift.h"
#include "i960_mem.h"

#define IO_BOARD_STATUS 0x01C00002u

void boot_io_ready_wait(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;
    /* Hardware waits for bit 5 set; host marks ready immediately. */
    i960_st_u8(I960_ABS, IO_BOARD_STATUS, 0, 0x20u);
}
