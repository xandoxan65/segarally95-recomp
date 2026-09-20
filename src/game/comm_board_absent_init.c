/* Comm-board absent init @ 0x1BD0 (tail of comm_board_probe). */
// @rom 0x1bd0 +0x78 comm_board_absent_init

#include "i960_lift.h"
#include "i960_mem.h"

void comm_board_absent_init(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    i960_st_u16(I960_ABS, 0x01c00040u, 0, (u16)g14);
    i960_st_u16(I960_ABS, 0x01c00024u, 0, 1u);
    i960_st_u16(I960_ABS, 0x01c00020u, 0, (u16)g14);
    i960_st_u16(I960_ABS, 0x01c00034u, 0, 0x0053u);
    i960_st_u16(I960_ABS, 0x01c00036u, 0, 0x0045u);
    i960_st_u16(I960_ABS, 0x01c00038u, 0, 0x0047u);
    i960_st_u16(I960_ABS, 0x01c0003au, 0, 0x0041u);
    i960_call_rom(0x1a58);
    i960_call_rom(0x1d38);
    i960_st_u16(I960_ABS, 0x01c00040u, 0, 1u);
}
