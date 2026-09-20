/* Attract / IO option flag seed @ 0x2970 (bal entry @ 0x2978).
 * source: disasm/maincpu/maincpu_002970_70.asm */
// @rom 0x2970 +0x70 game_option_flags_init

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_nvram.h"

void game_option_flags_init(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x2980–0x29D0 */
    i960_st_u8(I960_WORKRAM, 0x202018, 0, 1);
    i960_st_u8(I960_WORKRAM, 0x20201a, 0, 1);
    i960_st_u8(I960_WORKRAM, 0x20201e, 0, 1);
    i960_st_u8(I960_WORKRAM, 0x202019, 0, 0);
    i960_st_u8(I960_WORKRAM, 0x20201b, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x202020, 0, 0x3f800000u); /* movr +1.0 */
    i960_st_u8(I960_WORKRAM, 0x20201c, 0, 0);
    i960_st_u8(I960_WORKRAM, 0x20201d, 0, 0);
    i960_st_u8(I960_WORKRAM, 0x20201f, 0, 4);

    /* Host NVRAM overrides factory defaults when a file was loaded. */
    model2_nvram_apply_options();
}
