/* Coin option validate @ 0xA7A0 — g0 = 0 if ok, -1 if defaults were applied.
 * On failure: bal 0xA948 (coin_option_defaults).
 * source: disasm/maincpu/maincpu_00a7a0_100.asm */
// @rom 0xa7a0 +0xf4 coin_option_validate

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"

u32 coin_option_validate(u32 arg0, u32 arg1, u32 arg2)
{
    u32 ok;
    u32 i;
    u32 slot;
    u16 w;
    u8 flags;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0xA7A0–0xA7D4 */
    ok = 1u;
    if (i960_ld_u16(I960_WORKRAM, 0x202028, 0) == 0)
        ok = 0;
    else if (i960_ld_u16(I960_WORKRAM, 0x20202a, 0) == 0)
        ok = 0;
    else if (i960_ld_u16(I960_WORKRAM, 0x20202c, 0) > 9u) /* cmpobl 9,g4 */
        ok = 0;
    else if (i960_ld_u16(I960_WORKRAM, 0x20202e, 0) > 25u) /* not cmpobge 25 */
        ok = 0;

    /* @0xA7D8–0xA7FC: halfwords at 0x202030 must be <= 9. */
    slot = 0x202030u;
    for (i = 0; i < 2u; i++) {
        w = i960_ld_u16(I960_WORKRAM, slot, 0);
        if (w > 9u)
            ok = 0;
        slot += 2u;
    }

    /* @0xA800–0xA87C: mech table checks when (202024 & 12)==0 — skipped for now. */
    flags = i960_ld_u8(I960_WORKRAM, 0x202024, 0);
    (void)flags;

    if (ok == 0)
        coin_option_defaults(0, 0, 0);

    /* @0xA884–0xA88C: g0 = (ok==0) ? -1 : 0 */
    g0 = (ok == 0) ? (u32)-1 : 0u;
    return (u32)g0;
}
