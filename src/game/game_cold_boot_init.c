/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/out/lift/slices/maincpu_000032a0_3c.asm */
// @rom 0x32a0 +0x3c game_cold_boot_init

#include "i960_lift.h"
#include "i960_mem.h"
#include "cgm_format.h"
#include "lift_syms.h"

void game_cold_boot_init(u32 arg0, u32 arg1, u32 arg2)
{
    g0 = (uintptr_t)arg0;
    g1 = (uintptr_t)arg1;
    g2 = (uintptr_t)arg2;

    /* Format jump table @ 0x5FBFD0 ← ROM 0x5CFD0 (printf ld); was only in
     * lift_cgm_decode harness before — live attract never seeded it. */
    cgm_workram_runtime_init();
    game_seed_globals(0, 0, 0);
    comm_board_probe(0, 0, 0);
    game_subsys_boot(0, 0, 0);
    comm_board_setup_probe(0, 0, 0);
    g4 = i960_ld_u32(I960_WORKRAM, 0x20a554, 0);
    if ((unsigned char)g4 != 0)
        goto L_000032c0;
    boot_attract_flags_init(0, 0, 0);

    L_000032c0:
        boot_tile_splash_init(0, 0, 0);
    boot_palette_splash_upload(0, 0, 0);
    geo_renderer_init(0, 0, 0);
    sound_comm_init(0, 0, 0);
    game_mmio_boot_init(0, 0, 0);
    copro_bootstrap_entry(0, 0, 0);
    geo_view_boot_apply(0, 0, 0);
}
