/* Attract tile text helpers @ 0xC4D8 / 0xC644 / 0xC66C (shared by C640 and C5D0). */

#include "tile_attract_shared.h"
#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"

void tile_attract_flags_prologue(void)
{
    u32 g18;
    u32 g19;
    u32 g1a;
    u32 g1e;
    u32 g18b;

    /* @ 0xC4D8: load attract flag bytes and derive jump target in g5; bx (g5) when
     * runtime tables @ 0x20A570 are populated. Static lift: load only. */
    g18 = i960_ld_u8(I960_WORKRAM, 0x202018, 0);
    g19 = i960_ld_u8(I960_WORKRAM, 0x202019, 0);
    g1a = i960_ld_u8(I960_WORKRAM, 0x20201a, 0);
    g1e = i960_ld_u8(I960_WORKRAM, 0x20201e, 0);
    g18b = i960_ld_u8(I960_WORKRAM, 0x20201c, 0);
    (void)g18;
    (void)g19;
    (void)g1a;
    (void)g1e;
    (void)g18b;
}

void tile_attract_index_walk(void)
{
    u32 cursor;

    /* @ 0xC644–0xC658: ld 20209c; mov 15,g0; lda 0x1(g4),g4; mov 19,g1; st 20209c.
     * lda is effective-address (cursor + 1), not a byte load from absolute memory. */
    cursor = i960_ld_u32(I960_WORKRAM, 0x20209c, 0);
    g0 = 15;
    cursor = cursor + 1u;
    g1 = 19;
    i960_st_u32(I960_WORKRAM, 0x20209c, 0, cursor);
}

static void tile_attract_text_case0(void)
{
    u32 counter;

    counter = i960_ld_u32(I960_WORKRAM, 0x202098, 0);
    i960_st_u8(I960_WORKRAM, 0x20a580, 0, (u8)g14);
    i960_st_u32(I960_WORKRAM, 0x20209c, 0, (u32)g14);
    counter++;
    i960_st_u32(I960_WORKRAM, 0x202098, 0, counter);
}

void tile_attract_text_cases(void)
{
    u32 mode;

    mode = (u32)i960_ld_u8(I960_WORKRAM, 0x20201b, 0);
    i960_st_u32(I960_WORKRAM, 0x20a530, 0, (u32)g14);
    if (mode > 5u)
        mode = 5u;

    switch (mode) {
    case 0:
        tile_attract_text_case0();
        return;
    case 1:
        boot_tile_script_run(0x005ab550u);
        i960_st_u8(I960_WORKRAM, 0x20a580, 0, 1);
        return;
    case 2:
        boot_tile_script_run(0x005ab570u);
        i960_st_u8(I960_WORKRAM, 0x20a580, 0, 2);
        return;
    case 3:
    case 4:
        boot_tile_script_run(0x005ab590u);
        i960_st_u8(I960_WORKRAM, 0x20a580, 0, 3);
        return;
    case 5:
        boot_tile_script_run(0x005ab5b0u);
        i960_st_u8(I960_WORKRAM, 0x20a580, 0, 2);
        return;
    default:
        return;
    }
}
