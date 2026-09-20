/* Sound UART toggle @ 0x25BC0 (helper for sound_comm_init). */
// @rom 0x25bc0 +0x38 sound_uart_toggle

#include "i960_lift.h"
#include "i960_mem.h"

void sound_uart_toggle(u32 arg0, u32 arg1, u32 arg2)
{
    u32 val;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    val = i960_ld_u16(I960_ABS, 0x01c80002u, 0);
    val ^= (1u << 6);
    i960_st_u16(I960_ABS, 0x01c80002u, 0, (u16)val);
}
