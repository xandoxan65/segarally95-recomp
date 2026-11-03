/* Car pens and particles (code/race/particles.c) against the lifted originals (ported/particles.list).
 *
 * Several originals take host pointers to the object (the attract and select screens pass their own frame
 * shadows); a pointer into CRX or work RAM is turned back into its address, anything else runs the original only.
 * Hooks into lifted code change g0..g5 and g14, so each wrapper keeps the registers the original left. */
#include "wrap_common.h"
#include "race/particles.h"
#include "i960_mem.h"
#include "model2_memory.h"

void geo_attract_pen_prg_push_a(u32 pen_mode, u32 arg1, u32 arg2);

void sr_hook_pen_push_a(uint32_t pen_mode) { geo_attract_pen_prg_push_a(pen_mode, 0, 0); }

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

/* A host pointer into CRX or work RAM as its i960 address; 0 when it points elsewhere. */
static uint32_t ram_va(const void *p) {
    const uint8_t *b = (const uint8_t *)p;
    if(b >= model2_crx_ram && b < model2_crx_ram + sizeof(model2_crx_ram)) return 0x200000u + (uint32_t)(b - model2_crx_ram);
    if(b >= model2_workram && b < model2_workram + WORKRAM_SIZE) return 0x500000u + (uint32_t)(b - model2_workram);
    return 0;
}

#define PT_WRAP(name, call)                                               \
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

PT_WRAP(game_start_race_object_pen_walk, sr_object_pen_walk(a0))
PT_WRAP(game_start_race_object_pen_desc, sr_object_pen_desc(a0))
PT_WRAP(game_start_race_object_pen_far, sr_object_pen_far(a0, a1, a2))
PT_WRAP(game_start_race_object_pen_mid, sr_object_pen_mid(a0, a1, a2))
PT_WRAP(geo_attract_copro_submit, sr_particles_submit())

void lifted_geo_attract_object_pen(void *a0, u32 a1, u32 a2);
void geo_attract_object_pen(void *a0, u32 a1, u32 a2) {
    const uint32_t obj = ram_va(a0);
    regs_t regs_;
    if(!obj) {
        lifted_geo_attract_object_pen(a0, a1, a2);
        return;
    }
    {
        SR_DIFF_BEGIN();
        lifted_geo_attract_object_pen(a0, a1, a2);
        regs_save(&regs_);
        SR_DIFF_NATIVE();
        sr_object_pen(obj, a1, a2);
        SR_DIFF_END("geo_attract_object_pen");
        regs_load(&regs_);
    }
}

void lifted_geo_attract_glyph_emit(void *a0, void *a1, u32 a2);
void geo_attract_glyph_emit(void *a0, void *a1, u32 a2) {
    const uint32_t obj = ram_va(a1);
    regs_t regs_;
    if(!obj) {
        lifted_geo_attract_glyph_emit(a0, a1, a2);
        return;
    }
    {
        SR_DIFF_BEGIN();
        lifted_geo_attract_glyph_emit(a0, a1, a2);
        regs_save(&regs_);
        SR_DIFF_NATIVE();
        sr_glyph_emit((uint32_t)(uintptr_t)a0, obj, a2);
        SR_DIFF_END("geo_attract_glyph_emit");
        regs_load(&regs_);
    }
}

void lifted_geo_attract_object_extra(void *a0, u32 a1, u32 a2);
void geo_attract_object_extra(void *a0, u32 a1, u32 a2) {
    const uint32_t obj = ram_va(a0);
    regs_t regs_;
    if(!obj) {
        lifted_geo_attract_object_extra(a0, a1, a2);
        return;
    }
    {
        SR_DIFF_BEGIN();
        lifted_geo_attract_object_extra(a0, a1, a2);
        regs_save(&regs_);
        SR_DIFF_NATIVE();
        sr_particle_spawn(obj);
        SR_DIFF_END("geo_attract_object_extra");
        regs_load(&regs_);
    }
}

void lifted_geo_rng_next(void);
void geo_rng_next(void) {
    regs_t regs_;
    uint32_t l, n;
    SR_DIFF_BEGIN();
    lifted_geo_rng_next();
    l = (uint32_t)g0;
    regs_save(&regs_);
    SR_DIFF_NATIVE();
    n = sr_particle_rng();
    SR_DIFF_RET(l, n);
    SR_DIFF_END("geo_rng_next");
    regs_load(&regs_);
}

u32 lifted_geo_attract_copro_object(u32 a0, u32 a1, u32 a2);
u32 geo_attract_copro_object(u32 a0, u32 a1, u32 a2) {
    regs_t regs_;
    u32 l, n;
    SR_DIFF_BEGIN();
    l = lifted_geo_attract_copro_object(a0, a1, a2);
    regs_save(&regs_);
    SR_DIFF_NATIVE();
    n = sr_particle_step(a0, a1);
    SR_DIFF_RET(l, n);
    SR_DIFF_END("geo_attract_copro_object");
    regs_load(&regs_);
    return l;
}
