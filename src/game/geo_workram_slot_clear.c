/* Zero seven workram slots @ 0x21727C and seed counters @ 0x217250 (first thunk @ 0x46098). */
// @rom 0x46098 +0x40 geo_workram_slot_clear

#include "i960_lift.h"
#include "i960_mem.h"

void geo_workram_slot_clear(u32 arg0, u32 arg1, u32 arg2)
{
    u32 count;
    u32 addr;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    count = 7;
    addr = 0x21727c;
    while (count > 0) {
        i960_st_u32(I960_WORKRAM, addr, 0, 0);
        count--;
        addr -= 4;
    }
    i960_st_u32(I960_WORKRAM, 0x217250, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x217254, 0, 0);
}
