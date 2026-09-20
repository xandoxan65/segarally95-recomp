/* Unlifted operator test-menu slots — return g0=1 so mode-4 returns to the list.
 * Full bodies not yet lifted; each @rom marks the real handler entry. */
// @rom 0x8840 +0x4 test_menu_memory
// @rom 0x8b00 +0x4 test_menu_sound
// @rom 0x6a20 +0x4 test_menu_crt
// @rom 0xbe30 +0x4 test_menu_coin
// @rom 0x7670 +0x4 test_menu_output
// @rom 0x7d80 +0x4 test_menu_drive
// @rom 0xa3d0 +0x4 test_menu_bookkeeping
// @rom 0x7910 +0x4 test_menu_backup

#include "i960_lift.h"
#include "i960_mem.h"

#include <stdio.h>

static u32 test_menu_unlifted(const char *name, u32 arg0)
{
    static u32 s_last_slot = 0xffffffffu;
    u32 slot = i960_ld_u32(I960_WORKRAM, 0x20a2ec, 0);

    (void)arg0;
    if (slot != s_last_slot) {
        fprintf(stderr, "lift: test menu '%s' (slot %u) not lifted — returning to list\n",
                name, (unsigned)slot);
        s_last_slot = slot;
    }
    g0 = 1;
    return 1;
}

u32 test_menu_memory(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg1;
    (void)arg2;
    return test_menu_unlifted("MEMORY TEST", arg0);
}

u32 test_menu_sound(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg1;
    (void)arg2;
    return test_menu_unlifted("SOUND TEST", arg0);
}

u32 test_menu_crt(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg1;
    (void)arg2;
    return test_menu_unlifted("C.R.T TEST", arg0);
}

u32 test_menu_coin(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg1;
    (void)arg2;
    return test_menu_unlifted("COIN ASSIGNMENTS", arg0);
}

u32 test_menu_output(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg1;
    (void)arg2;
    return test_menu_unlifted("OUTPUT TEST", arg0);
}

u32 test_menu_drive(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg1;
    (void)arg2;
    return test_menu_unlifted("DRIVE BD TEST", arg0);
}

u32 test_menu_bookkeeping(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg1;
    (void)arg2;
    return test_menu_unlifted("BOOKKEEPING", arg0);
}

u32 test_menu_backup(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg1;
    (void)arg2;
    return test_menu_unlifted("BACKUP DATA CLEAR", arg0);
}
