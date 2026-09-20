/* Game subsys boot @ 0x2C80 — comm-flag gate + coin defaults + backup SRAM.
 * source: disasm/maincpu/maincpu_002c80_130.asm */
// @rom 0x2c80 +0x160 game_subsys_boot

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"
#include "model2_nvram.h"

void game_subsys_boot(u32 arg0, u32 arg1, u32 arg2)
{
    u32 hash;
    u16 stored;
    u16 mode;
    u32 valid;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x2C80 */
    comm_flag_gate(0, 0, 0);

    /* Re-apply NVRAM before checksum so a prior save keeps COUNTRY / coinage. */
    model2_nvram_apply_options();

    /* @0x2C84–0x2CB8: mode-table checksum vs 0x202010 / mode word 0x202012. */
    g0 = 0x202012u;
    g1 = 34u; /* addo 31,3 */
    hash = game_mode_table_fetch((u32)g0, (u32)g1);
    hash &= 0xffffu;
    stored = i960_ld_u16(I960_WORKRAM, 0x202010, 0);
    mode = i960_ld_u16(I960_WORKRAM, 0x202012, 0);

    if (stored != (u16)hash || mode != 36u) {
        /* @0x2CBC–0x2CC8 */
        game_coin_option_boot(0, 0, 0);
        i960_st_u16(I960_ABS, 0x01d00000u, 0, 0);
        i960_st_u16(I960_ABS, 0x01d00c68u, 0, 0);
        /* Defaults just wiped options — restore NVRAM again. */
        model2_nvram_apply_options();
    }

    /*
     * @0x2CD0–0x2DAC: backup-SRAM validate/copy paths — still pending full lift.
     * Fall through to the always-run coin validate at @0x2DD0.
     */
    valid = coin_option_validate(0, 0, 0);
    /* @0x2DD4–0x2DDC: call mode_index_step when g0 == -1 */
    if (valid == (u32)-1)
        game_mode_index_step(0, 0, 0);

    /* Ensure chute masks even when option checksum already matched (skipped A9D0). */
    if (i960_ld_u16(I960_ABS, 0x01d00016u, 0) == 0)
        game_coin_sram_reset(0, 0, 0);
}
