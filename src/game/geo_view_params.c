/* View params @ 0x39540 — seed FOV/pitch workram from DIP bytes, then matrix_seed + pitch_clamp.
 * source: disasm/maincpu/maincpu_039540_98.asm */
// @rom 0x39540 +0x90 geo_view_params

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"

#include "lift_syms.h"

void geo_view_params(u32 arg0, u32 arg1, u32 arg2)
{
    u8 flag;
    u8 raw;
    u32 mag;
    double scaled;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    flag = (u8)i960_ld_u8(I960_WORKRAM, 0x20201a, 0);
    i960_st_u32(I960_WORKRAM, 0x214274, 0, 31u);
    raw = (u8)i960_ld_u8(I960_WORKRAM, 0x20201f, 0);
    mag = (u32)(raw & 0xffu);
    /* both branches: and 0xff then cvtir → same magnitude path */
    (void)flag;
    scaled = (double)(i32)mag / 8.0; /* divrl by 0x40200000 double (= 8.0) */
    g4 = i960_f64_to_u32(scaled);
    i960_st_u32(I960_WORKRAM, 0x214278, 0, (u32)g14);
    i960_st_u32(I960_WORKRAM, 0x214270, 0, (u32)g4);
    geo_view_matrix_seed((u32)g0, (u32)g1, (u32)g2);
    geo_view_pitch_clamp((u32)g0, (u32)g1, (u32)g2);
    i960_st_u8(I960_WORKRAM, 0x202049, 0, (u8)16);
}
