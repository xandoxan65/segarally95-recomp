/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_00003150_110.asm */
// @rom 0x3150 +0x110 game_dispatch_main

#include "i960_lift.h"
#include "model2_rom.h"
#include "i960_mem.h"
#include "i960_host.h"
#include "lift_syms.h"

void game_dispatch_main(u32 arg0, u32 arg1, u32 arg2)
{
    void *arg0_p = (void *)arg0;
    void *a2 = (void *)arg2;

    (void)arg0_p;
    (void)a2;
    g1 = (uintptr_t)arg1;

    game_irq_wr_stub(0, 0);
    game_comm_aa24_seed(0, 0);
    i960_st_u32(I960_WORKRAM, 0x202094, 0, (u32)g14);
    game_workram_bx_stub(0, 0);
    game_staging_init(0, 0, 0);
    game_subsys_init_stub(0, 0, 0);
    game_cold_boot_init(0, 0, 0);
    /* Host: present cold-boot splash once (HW draws via irq_init_major @ 0x26980). */
    if (i960_host_boot_screen()) {
        boot_tile_splash_frame(0, 0, 0);
        i960_host_frame_present();
    }
    /*
     * Do not invent board type 3 when a host scene is seeded — ROM never stores
     * 3 into 0x20a530 here. board==3 makes attract skip hook 0x5b1cc0 and makes
     * scene_hud_alt ret immediately, so the leaderboard never installs.
     */
    g2 = (uintptr_t)i960_vaddr_ptr(0x11802323);
    i960_mmio_write_u32(0x884000, (u32)g2); /* copro_fifo */
    g0 = 0x1e240;
    format_scan_bx_stub(0, 0);
    game_mode_apply(0, 0, 0);

    L_00003190:
        if (i960_host_dispatch_halted())
            return;
        g4 = i960_ld_u32(I960_WORKRAM, 0x20a530, 0);
        /* lift: cmpibe 0, g4, 0x31fc @ 0x3198 */
        comm_board_dispatch(0, 0, 0);
        g4 = i960_ld_u32(I960_WORKRAM, 0x20a534, 0);
        g5 = (g4 << 3) - g4;
        g5 = g5 << 6;
        {
            u32 scaled = (u32)g5;
            const u8 *p;

            p = model2_rom_at(0x1a121c4 + scaled);
            g4 = p ? *(const u32 *)p : 0;
            p = model2_rom_at(0x1a121c0 + scaled);
            g5 = p ? (uintptr_t)p : 0;
        }
        g7 = 0;
        g6 = (uintptr_t)(model2_crx_ram + 0xa540);
        i960_st_u32(I960_WORKRAM, 0x20a550, 0, (u32)g4);
        do {
            const u8 *cell = model2_rom_at((u32)(g5 + 8));
            g4 = cell ? *(const u32 *)cell : 0;
            g7 = g7 + 1;
            *(u32 *)g6 = (u32)g4;
            g6 = g6 + 4;
            g5 = g5 + 0x4;
        } while (g7 <= 3);
        backup_sram_sync(0, 0, 0);
        g4 = i960_ld_u32(I960_WORKRAM, 0x202098, 0);
        g4 = g4 & 15;
        g4 = i960_ld_u32(I960_WORKRAM, 0x5a2110, (u32)(g4 << 2));
        /* lift: cmpibe 0, g4, 0x3220 @ 0x3214 */
        if (g4)
            i960_call_indirect(g4);
        goto L_00003230;
        i960_st_u32(I960_WORKRAM, 0x202098, 0, (u32)g14);
        i960_st_u32(I960_WORKRAM, 0x20209c, 0, (u32)g14);

    L_00003230:
        i960_mmio_write_u32(0x8000f0, (u32)g14); /* geo_regs */
        copro_fifo_init(0, 0);
        game_frame_update(0, 0, 0);
        g0 = 0 - 1;
        g1 = 0;
        texture_bank_select((u32)g0, (u32)g1, (u32)g2);
        game_mode_apply(0, 0, 0);
        geo_fifo_emit(0, 0);
        geo_fifo_preset(0, 0);
        i960_host_frame_present();
        goto L_00003190;
}
