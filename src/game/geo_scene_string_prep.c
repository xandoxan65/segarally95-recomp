/* Scene string prep @ 0x5E30 — submode-1 attract string table draw.
 * source: disasm/maincpu/maincpu_005e30_1e8.asm */
// @rom 0x5e30 +0x1e8 geo_scene_string_prep

#include "i960_lift.h"
#include "lift_log.h"
#include "i960_mem.h"

#include "lift_syms.h"

#include <stdio.h>
#include <string.h>

static void printf_guest(u32 fmt_va)
{
    uintptr_t fp_save = fp;
    /*
     * libc_printf marshals through fp+0x40 when fp != 0. On i960 a `call`
     * allocates a fresh frame so the caller's 0x40(fp) table is preserved; the
     * host shares one fp, so force the scratch va-block instead.
     */
    fp = 0;
    libc_printf((const char *)(uintptr_t)fmt_va, (u32)g1, (u32)g2);
    fp = fp_save;
}

u32 geo_scene_string_prep(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    /*
     * Private frame — ROM does lda 0x30(sp),sp then stq …,0x40(fp). Host must
     * not set fp=sp and cast (u32)(fp+off) into I960_WORKRAM.
     */
    u8 frame[0xa0];
    u32 strs[11];
    u32 i;
    u32 cursor;
    u32 flags;
    u8 pad;
    u32 bits;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;

    /* ldq/ldl/ld from 0x5a4dd0.. — keep a C copy; printf must not clobber. */
    for (i = 0; i < 11u; i++) {
        strs[i] = i960_ld_u32(I960_WORKRAM, 0x5a4dd0u + (i << 2), 0);
        *(u32 *)(fp + 0x40 + (i << 2)) = strs[i];
    }

    tile_cursor_seed(22u, 9u);
    printf_guest(0x5a4e00u);

    {
        static int s_logged;

        if (!s_logged) {
            lift_log(
                    "lift: test menu string_prep ptr0=0x%x cursor=%u\n",
                    strs[0],
                    i960_ld_u32(I960_WORKRAM, 0x20a2ec, 0));
            s_logged = 1;
        }
    }

    for (i = 0; (i32)i <= 10; i++) {
        cursor = i960_ld_u32(I960_WORKRAM, 0x20a2ec, 0);
        if ((u8)i == (u8)cursor) {
            g0 = strs[i];
            g1 = 1;
            geo_scene_glyph_pair((u32)g0, (u32)g1, (u32)g2);
        } else {
            boot_tile_script_run(0x5a4e14u);
            g0 = strs[i];
            boot_tile_script_run((u32)g0);
        }
        boot_tile_script_run(0x5a4e18u);
    }

    geo_scene_row_print(2u, (u32)g1, (u32)g2);

    flags = i960_ld_u32(I960_WORKRAM, 0x202074, 0);
    if ((flags & 1u)
        || ((i960_ld_u8(I960_WORKRAM, 0x202064, 0) >> 2) & 1u)
        || ((i960_ld_u8(I960_WORKRAM, 0x202064, 0) >> 4) & 1u)) {
        i960_st_u32(I960_WORKRAM, 0x20209c, 0,
                    i960_ld_u32(I960_WORKRAM, 0x20209c, 0) + 1u);
        goto epilogue;
    }

    cursor = i960_ld_u32(I960_WORKRAM, 0x20a2ec, 0);
    flags = i960_ld_u32(I960_WORKRAM, 0x202074, 0);
    if ((flags >> 1) & 1u)
        goto bump;
    pad = (u8)i960_ld_u8(I960_WORKRAM, 0x202064, 0);
    if ((pad >> 3) & 1u)
        goto bump;
    bits = ((pad >> 5) & 1u)
         | ((pad >> 5) & 2u)
         | ((pad >> 5) & 4u)
         | (((u32)i960_ld_u8(I960_WORKRAM, 0x202065, 0) & 1u) << 3);
    if ((u8)bits != 0)
        goto bump;
    bits = ((pad >> 5) & 1u)
         | ((pad >> 5) & 2u)
         | ((pad >> 5) & 4u)
         | (((u32)i960_ld_u8(I960_WORKRAM, 0x202065, 0) & 1u) << 3);
    if ((u8)bits == 0)
        goto store_cursor;

bump:
    cursor += 1u;

store_cursor:
    i960_st_u32(I960_WORKRAM, 0x20a2ec, 0, cursor);
    /* ROM st g14 — leaf link is 0; host g14 is the call return PC. */
    if (cursor > 10u)
        i960_st_u32(I960_WORKRAM, 0x20a2ec, 0, 0);

epilogue:
    if (i960_ld_u32(I960_WORKRAM, 0x20a2f8, 0) != 0) {
        tile_cursor_seed(30u, 31u + 8u);
        g1 = i960_ld_u32(I960_WORKRAM, 0x202014, 0);
        printf_guest(0x5a4e20u);
    }

    fp = fp_save;
    return (u32)g0;
}
