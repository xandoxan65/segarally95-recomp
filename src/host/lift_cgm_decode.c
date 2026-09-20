/* Isolated CGM catalog decode harness — same lifted calls as comm_attract_inner_2 init. */

#include "track_viewer.h"
#include "cgm_format.h"
#include "i960_host.h"
#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"
#include "model2_memory.h"
#include "model2_rom.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CGM_SPLASH_CATALOG_VADDR  0x02879db0u

static const char *cgm_decode_dump_dir(const track_viewer_opts_t *opts)
{
    const char *dump = getenv("I960_PALETTE_DUMP");

    if (dump && *dump)
        return dump;
    if (opts && opts->palette_dump && opts->palette_dump[0])
        return opts->palette_dump;
    return "build/lift/cgm_decode";
}

static void cgm_decode_boot_prereqs(void)
{
    /* game_cold_boot_init @ 0x32A0 — globals + char RAM + palette before tile draw. */
    game_seed_globals(0, 0, 0);
    boot_tile_splash_init(0, 0, 0);
    boot_palette_splash_upload(0, 0, 0);
}

static u32 cgm_decode_resolve_vaddr(uint32_t offset_or_vaddr)
{
    if (offset_or_vaddr >= MAIN_DATA_A)
        return offset_or_vaddr;
    return MAIN_DATA_A + offset_or_vaddr;
}

static u32 cgm_decode_inner_2_preamble(u32 catalog_vaddr, u32 mode, u32 flags)
{
  /* comm_attract_inner_2 @ 0x10728–0x1077C (init path, before countdown). */
    comm_attract_catalog_seed(0, 0, 0);
    tile_attract_palram_gate(0xffffu);
    i960_st_u8(I960_ABS, 0x181c000u, 0, 0);
    tile_map_banks_clear(0, 0, 0);

    g0 = 0;
    g1 = 0;
    g2 = catalog_vaddr;
    g3 = mode;
    g4 = flags;
    return catalog_draw_setup((u32)g0, (u32)g1, (u32)g2);
}

static void cgm_decode_viewport_sync(void)
{
    u32 slot;

    /*
     * Map @ 0x01004000 is MAME tile_layer[2] (pair index 2 after
     * draw_common layer>>=1). vscr is 0x20b920 → tile_ram[0x5006] →
     * 0x0100a00c — not 0x20b91e (pair1). (+vscr) places map row N at
     * FB y=0 when vscr=N*8 (splash decode only; car_display leaves 0).
     */
    slot = i960_ld_u32(I960_WORKRAM, CGM_SLOT_CURSOR, 0);
    if (slot > 0u && slot < 64u)
        i960_st_u16(I960_WORKRAM, 0x20b920, 0, (u16)(slot * 8u));
}

static void cgm_decode_inner_2_tail(void)
{
    /* @0x10788–0x107B0 after catalog_draw_setup on splash path. */
    if (i960_ld_u8(I960_WORKRAM, 0x202018, 0) != 0u)
        comm_palette_index_call(0xa7u);
    comm_attract_splash_bind(0);
    cgm_decode_viewport_sync();
}

static u32 cgm_decode_count_map_tiles(u32 map_base)
{
    u32 nz = 0;
    u32 i;

    for (i = 0; i < 0x4000u; i += 2u) {
        if (i960_ld_u16(I960_ABS, map_base, i) != 0)
            nz++;
    }
    return nz;
}

int i960_lift_cgm_decode_run(const track_viewer_opts_t *opts)
{
    u32 catalog_vaddr;
    u32 batch;
    const char *dump;
    int header_ok;

    if (!opts || !opts->cgm_decode)
        return 1;

    /* Host probe: full game-select HUD (BG + title + icons) as display_setup. */
    if (opts->cgm_vaddr == 0x1u || opts->cgm_vaddr == 0x0210cf40u) {
        const char *dir = opts->palette_dump && opts->palette_dump[0]
            ? opts->palette_dump
            : "build/lift/select_hud";

        cgm_workram_runtime_init();
        cgm_decode_boot_prereqs();
        cgm_workram_runtime_init();
        /* EXPORT path (nvram country=export → 0x202019=2). */
        i960_st_u8(I960_WORKRAM, 0x202019, 0, 2);
        i960_st_u32(I960_WORKRAM, 0x20209c, 0, 2);
        game_start_display_setup(0, 0, 0);
        boot_tile_splash_frame(0, 0, 0);
        model2_palette_state_dump(dir);
        fprintf(stderr, "lift: select HUD probe done — %s\n", dir);
        return 0;
    }

    catalog_vaddr = cgm_decode_resolve_vaddr(opts->cgm_vaddr);
    dump = cgm_decode_dump_dir(opts);

    if (!model2_rom_at(catalog_vaddr)) {
        fprintf(stderr,
                "lift: --decode-cgm %#x — not mapped in main_data ROM\n",
                catalog_vaddr);
        return 1;
    }

    cgm_workram_runtime_init();
    scene_list_seed(0, 0, 0);
    header_ok = cgm_header_match(catalog_vaddr);
    fprintf(stderr,
            "lift: CGM decode catalog=%#x mode=%u flags=%#x header_match=%d\n",
            catalog_vaddr,
            (unsigned)opts->cgm_mode,
            (unsigned)opts->cgm_flags,
            header_ok);

    cgm_decode_boot_prereqs();
    cgm_workram_runtime_init();

    batch = cgm_decode_inner_2_preamble(
        catalog_vaddr, opts->cgm_mode, opts->cgm_flags);
    if (batch == (u32)-1) {
        fprintf(stderr, "lift: catalog_draw_setup failed for %#x\n", catalog_vaddr);
        return 1;
    }

    if (opts->cgm_splash_preamble)
        cgm_decode_inner_2_tail();
    else
        cgm_decode_viewport_sync();

    /* CGM printf staging writes label literals to L2 @ 0x01000000 — not splash art. */
    tile_map_bank_clear(0x01000000u);

    /* IRQ/vblank path @0x26980 commits queued 32-byte palette rows after CGM draw. */
    boot_tile_splash_frame(0, 0, 0);

    fprintf(stderr,
            "lift: catalog_draw_setup batch=%d L1_nonzero_words=%u L2_nonzero_words=%u\n",
            (int)batch,
            cgm_decode_count_map_tiles(0x01004000u),
            cgm_decode_count_map_tiles(0x01000000u));

    model2_palette_state_dump(dump);

    fprintf(stderr, "lift: CGM decode done — %s\n", dump);
    return 0;
}
