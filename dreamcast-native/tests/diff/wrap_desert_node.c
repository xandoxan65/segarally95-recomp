/* Desert helicopter ports (code/race/desert_node.c) under their lifted names (ported/desert_node.list), and the
 * diff build's desert_hooks.h: the GEO port writes the lifted code makes, and the lifted pen push.
 *
 * The hooks run lifted code that changes g0..g5 and g14, so each wrapper keeps the registers the original left
 * and puts them back after the port. The GEO state is not rewound between the two runs, so the GEO receives
 * each object twice here; only the write pointer 0x802008 is read back, and nothing in the node writes it. */
#include "wrap_common.h"
#include "race/desert_node.h"
#include "race/desert_hooks.h"
#include "i960_mem.h"
#include "model2_memory.h"

#include <stdio.h>
#include <string.h>

void geo_attract_pen_prg_push_b(uintptr_t obj, uintptr_t obj18, u32 wr_base);

uint32_t sr_hook_geo_list_ptr(void) { return i960_mmio_read_u32(0x802008u); }

void sr_hook_geo_object_start(uint32_t list_ptr) { i960_mmio_write_u32(0x801008u, list_ptr + 0x34u); }

void sr_hook_geo_bank_word(uint32_t word) {
    i960_mmio_write_u32(0x800010u + ((sr_ld32(0x202278u) & 3u) << 10), word);
}

void sr_hook_geo_catalog(uint32_t catalog) {
    const uint8_t *p = model2_rom_at(catalog);
    uint32_t w[4], i;
    if(!p) return;
    memcpy(w, p, sizeof w);
    for(i = 0; i < 4; i++) i960_mmio_write_u32(GEO_PRG_FIFO, w[i]);
}

void sr_hook_pen_push(uint32_t obj, uint32_t angles, uint32_t eye, uint32_t pen_mode) {
    g3 = pen_mode;
    geo_attract_pen_prg_push_b(obj, angles, eye);
}

typedef struct {
    uintptr_t g[16], fp, sp;
} regs_t;

static void regs_save(regs_t *r) {
    r->g[0] = g0; r->g[1] = g1; r->g[2] = g2; r->g[3] = g3;
    r->g[4] = g4; r->g[5] = g5; r->g[6] = g6; r->g[7] = g7;
    r->g[8] = g8; r->g[9] = g9; r->g[10] = g10; r->g[11] = g11;
    r->g[12] = g12; r->g[13] = g13; r->g[14] = g14; r->g[15] = g15;
    r->fp = fp;
    r->sp = sp;
}

static void regs_load(const regs_t *r) {
    g0 = r->g[0]; g1 = r->g[1]; g2 = r->g[2]; g3 = r->g[3];
    g4 = r->g[4]; g5 = r->g[5]; g6 = r->g[6]; g7 = r->g[7];
    g8 = r->g[8]; g9 = r->g[9]; g10 = r->g[10]; g11 = r->g[11];
    g12 = r->g[12]; g13 = r->g[13]; g14 = r->g[14]; g15 = r->g[15];
    fp = r->fp;
    sp = r->sp;
}

#define DN_WRAP(name, call)                                               \
    void lifted_##name(u32 a0, u32 a1, u32 a2);                          \
    void name(u32 a0, u32 a1, u32 a2) {                                  \
        regs_t regs_;                                                    \
        SR_DIFF_BEGIN();                                                 \
        lifted_##name(a0, a1, a2);                                       \
        regs_save(&regs_);                                               \
        SR_DIFF_NATIVE();                                                \
        call;                                                            \
        SR_DIFF_END(#name);                                              \
        regs_load(&regs_);                                               \
    }

DN_WRAP(game_start_race_desert_node_init, sr_desert_node_init(a0))
DN_WRAP(game_start_race_desert_node_frame, sr_desert_node_frame(a0))
DN_WRAP(game_start_race_desert_pose_step, sr_desert_pose_step(a0))
DN_WRAP(game_start_race_desert_tgp55_walk, sr_desert_dust_draw())
DN_WRAP(game_start_race_desert_obj_near_flag, sr_desert_sound_near(a0))

/* The node passes the dust position as a guest address (0x216a70). */
void lifted_game_start_race_desert_rng_jitter(u32 a0, u32 a1, u32 a2);
void game_start_race_desert_rng_jitter(u32 a0, u32 a1, u32 a2) {
    regs_t regs;
    if(sr_wrap_is_host((void *)(uintptr_t)a0)) {
        fprintf(stderr, "sr_diff: desert_rng_jitter with a host pointer, not checked\n");
        lifted_game_start_race_desert_rng_jitter(a0, a1, a2);
        return;
    }
    {
        SR_DIFF_BEGIN();
        lifted_game_start_race_desert_rng_jitter(a0, a1, a2);
        regs_save(&regs);
        SR_DIFF_NATIVE();
        sr_desert_dust_spawn(a0, a1);
        SR_DIFF_END("game_start_race_desert_rng_jitter");
        regs_load(&regs);
    }
}

u32 lifted_game_start_race_desert_table_pick(u32 a0, u32 a1, u32 a2);
u32 game_start_race_desert_table_pick(u32 a0, u32 a1, u32 a2) {
    regs_t regs;
    u32 l;
    SR_DIFF_BEGIN();
    l = lifted_game_start_race_desert_table_pick(a0, a1, a2);
    regs_save(&regs);
    SR_DIFF_NATIVE();
    SR_DIFF_RET(l, sr_desert_path_table());
    SR_DIFF_END("game_start_race_desert_table_pick");
    regs_load(&regs);
    return l;
}

u32 lifted_game_start_race_desert_near_pick(u32 query, u32 idx, u32 a2);
u32 game_start_race_desert_near_pick(u32 query, u32 idx, u32 a2) {
    regs_t regs;
    u32 l;
    SR_DIFF_BEGIN();
    l = lifted_game_start_race_desert_near_pick(query, idx, a2);
    regs_save(&regs);
    SR_DIFF_NATIVE();
    SR_DIFF_RET(l, sr_desert_near_pick(query, idx));
    SR_DIFF_END("game_start_race_desert_near_pick");
    regs_load(&regs);
    return l;
}

/* The rank comes in g3. */
u32 lifted_game_start_race_desert_slot_flag(u32 pos, u32 angles, u32 lane);
u32 game_start_race_desert_slot_flag(u32 pos, u32 angles, u32 lane) {
    const int32_t rank = (int32_t)(u32)g3;
    regs_t regs;
    u32 l;
    SR_DIFF_BEGIN();
    l = lifted_game_start_race_desert_slot_flag(pos, angles, lane);
    regs_save(&regs);
    SR_DIFF_NATIVE();
    SR_DIFF_RET(l, sr_desert_slot_flag(pos, angles, lane, rank));
    SR_DIFF_END("game_start_race_desert_slot_flag");
    regs_load(&regs);
    return l;
}
