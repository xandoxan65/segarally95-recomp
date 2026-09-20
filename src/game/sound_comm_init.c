/* Sound comm UART init @ 0x25CE0 (game_cold_boot_init tail). */
// @rom 0x25ce0 +0x54 sound_comm_init

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_snd.h"

#include <stdio.h>

void sound_comm_init(u32 arg0, u32 arg1, u32 arg2)
{
    u32 val;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    i960_st_u16(I960_ABS, 0x01c80002u, 0, (u16)g14);
    i960_call_rom(0x25bc0);
    i960_st_u16(I960_ABS, 0x01c80002u, 0, (u16)g14);
    i960_call_rom(0x25bc0);
    i960_st_u16(I960_ABS, 0x01c80002u, 0, (u16)g14);
    i960_call_rom(0x25bc0);

    val = i960_ld_u16(I960_ABS, 0x01c80002u, 0);
    val |= (1u << 6);
    i960_st_u16(I960_ABS, 0x01c80002u, 0, (u16)val);
    i960_call_rom(0x25bc0);

    i960_st_u16(I960_ABS, 0x01c80002u, 0, 0x004eu);
    i960_call_rom(0x25bc0);

    i960_st_u16(I960_ABS, 0x01c80002u, 0, 0x0038u); /* 31+24 = 55 */
    i960_call_rom(0x25bf8);

    if (model2_snd_log_enabled())
        fprintf(stderr,
                "lift: sound UART init mode=0x4e command=0x38 advertise=%u\n",
                (unsigned)i960_ld_u8(I960_WORKRAM, 0x202018, 0));
}
