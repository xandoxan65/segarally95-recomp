/* The player scene callees (code/race/player_scene.c) under their lifted names: run the original, then the port
 * from the same state, compare. The lifted definitions are renamed lifted_<name> (ported/player_scene.list). */
#include "wrap_common.h"
#include "race/player_scene.h"
#include "lift_syms.h"
#include <string.h>

SR_DIFF_WRAP3(geo_view_mode_latch, sr_player_mode_latch(a0))
SR_DIFF_WRAP3(geo_view_scene_epilogue, sr_player_pose_publish(a0))

/* Returns in g0 as well as the C return. */
#define SR_DIFF_WRAP_RET(name, call)                                      \
    u32 lifted_##name(u32 a0, u32 a1, u32 a2);                           \
    u32 name(u32 a0, u32 a1, u32 a2) {                                   \
        u32 l, n;                                                        \
        SR_DIFF_BEGIN();                                                 \
        l = lifted_##name(a0, a1, a2);                                   \
        SR_DIFF_NATIVE();                                                \
        n = (call);                                                      \
        SR_DIFF_RET(l, n);                                               \
        SR_DIFF_END(#name);                                              \
        return l;                                                        \
    }

SR_DIFF_WRAP_RET(geo_view_fov_scale, sr_steer_amount())
SR_DIFF_WRAP_RET(geo_view_pitch_smooth, sr_player_steer_smooth())
SR_DIFF_WRAP_RET(geo_rng_u8, sr_rng_next())

/* scene_prep keeps the caller's fp: its temporaries land in scene_frame's 0x220-byte frame, and g1/g2 point at
 * fp+0x130 / fp+0x140 in it. Compare the frame up to 0x150. */
void *lifted_geo_view_scene_prep(void *a0, void *a1, void *a2);
void *geo_view_scene_prep(void *a0, void *a1, void *a2) {
    uint8_t *frame = (uint8_t *)fp;
    void *l;
    u32 n;
    SR_DIFF_BEGIN();
    SR_DIFF_EXTRA(frame, 0x150);
    l = lifted_geo_view_scene_prep(a0, a1, a2);
    SR_DIFF_NATIVE();
    n = sr_player_scene_prep((u32)(uintptr_t)a0, sr_wrap_host_ptr(a1), sr_wrap_host_ptr(a2), frame);
    SR_DIFF_RET((u32)(uintptr_t)l, n);
    SR_DIFF_END("geo_view_scene_prep");
    return l;
}

/* vec_push takes g0..g6 (g1/g2/g3/g5 host pointers into scene_frame's frame) and returns in g0.
 * The original spills g8..g12 to its i960 stack frame (sp+0x60.., which lies in CRX RAM around 0x200b20) and
 * reloads them on return. That spill is calling-convention residue, not game state: the native side gets the
 * same words written here so the RAM comparison covers everything else. */
void lifted_geo_view_copro_vec_push(u32 a0, u32 a1, u32 a2);
void geo_view_copro_vec_push(u32 a0, u32 a1, u32 a2) {
    const u32 node = (u32)g0, scale = (u32)g4, slot = (u32)g6;
    uint8_t *p1 = sr_wrap_host_ptr((void *)g1), *p2 = sr_wrap_host_ptr((void *)g2);
    uint8_t *p3 = sr_wrap_host_ptr((void *)g3), *p5 = sr_wrap_host_ptr((void *)g5);
    u32 l, n;
    SR_DIFF_BEGIN();
    if(sr_wrap_is_host((void *)g5)) SR_DIFF_EXTRA(p5, 12);
    lifted_geo_view_copro_vec_push(a0, a1, a2);
    l = (u32)g0;
    SR_DIFF_NATIVE();
    n = sr_player_wheel_force(node, p1, p2, p3, scale, p5, slot);
    {
        const u32 spill[5] = {(u32)g8, (u32)g9, (u32)g10, (u32)g11, (u32)g12};
        memcpy((void *)(sp + 0x60), spill, sizeof(spill));
    }
    SR_DIFF_RET(l, n);
    SR_DIFF_END("geo_view_copro_vec_push");
}

/* object_bind keeps the caller's fp and writes fp+0x40..0x5b. */
void lifted_geo_view_object_bind(void *a0, u32 a1, u32 a2);
void geo_view_object_bind(void *a0, u32 a1, u32 a2) {
    uint8_t *frame = (uint8_t *)fp;
    SR_DIFF_BEGIN();
    SR_DIFF_EXTRA(frame, 0x60);
    lifted_geo_view_object_bind(a0, a1, a2);
    SR_DIFF_NATIVE();
    sr_player_gap_scale((u32)(uintptr_t)a0, frame);
    SR_DIFF_END("geo_view_object_bind");
}

/* Until the player_update module provides sr_player_frame_update, run the lifted frame_update on the same memory
 * with the registers scene_frame passes (g0 node, g1/g2 host pointers into its frame, g3 corr, g14 = 0). */
__attribute__((weak)) void sr_player_frame_update(uint32_t node, uint32_t a1[3], uint32_t a2[3], uint32_t corr) {
    g0 = node;
    g1 = (uintptr_t)a1;
    g2 = (uintptr_t)a2;
    g3 = corr;
    g14 = 0;
    geo_view_frame_update((void *)(uintptr_t)node, a1, a2);
}

/* The i960 registers as globals: scene_frame leaves values in them, and the lifted callees the native side still
 * runs (the frame_update shim) change them. Both sides end with the lifted run's values. */
typedef struct {
    uintptr_t g[16], r[16], fp, sp;
    double f[4];
} sr_regs_t;

static void regs_save(sr_regs_t *s) {
    s->g[0] = g0; s->g[1] = g1; s->g[2] = g2; s->g[3] = g3; s->g[4] = g4; s->g[5] = g5; s->g[6] = g6;
    s->g[7] = g7; s->g[8] = g8; s->g[9] = g9; s->g[10] = g10; s->g[11] = g11; s->g[12] = g12; s->g[13] = g13;
    s->g[14] = g14; s->g[15] = g15;
    s->r[0] = r0; s->r[1] = r1; s->r[2] = r2; s->r[3] = r3; s->r[4] = r4; s->r[5] = r5; s->r[6] = r6;
    s->r[7] = r7; s->r[8] = r8; s->r[9] = r9; s->r[10] = r10; s->r[11] = r11; s->r[12] = r12; s->r[13] = r13;
    s->r[14] = r14; s->r[15] = r15;
    s->fp = fp; s->sp = sp;
    s->f[0] = fp0; s->f[1] = fp1; s->f[2] = fp2; s->f[3] = fp3;
}

static void regs_load(const sr_regs_t *s) {
    g0 = s->g[0]; g1 = s->g[1]; g2 = s->g[2]; g3 = s->g[3]; g4 = s->g[4]; g5 = s->g[5]; g6 = s->g[6];
    g7 = s->g[7]; g8 = s->g[8]; g9 = s->g[9]; g10 = s->g[10]; g11 = s->g[11]; g12 = s->g[12]; g13 = s->g[13];
    g14 = s->g[14]; g15 = s->g[15];
    r0 = s->r[0]; r1 = s->r[1]; r2 = s->r[2]; r3 = s->r[3]; r4 = s->r[4]; r5 = s->r[5]; r6 = s->r[6];
    r7 = s->r[7]; r8 = s->r[8]; r9 = s->r[9]; r10 = s->r[10]; r11 = s->r[11]; r12 = s->r[12]; r13 = s->r[13];
    r14 = s->r[14]; r15 = s->r[15];
    fp = s->fp; sp = s->sp;
    fp0 = s->f[0]; fp1 = s->f[1]; fp2 = s->f[2]; fp3 = s->f[3];
}

/* scene_frame's callees spill registers to the i960 stack above sp, which lies in CRX RAM (vec_push: g8..g12 at
 * sp+0x60, around 0x200b20). That is calling-convention residue, not game state: the native side gets the lifted
 * run's bytes of that window. Without this copy only the first call differs, in exactly those five words. */
#define SR_STACK_WINDOW 0x80u

void lifted_geo_view_scene_frame(void *a0, void *a1, void *a2);
void geo_view_scene_frame(void *a0, void *a1, void *a2) {
    sr_regs_t entry, after;
    uint8_t stack[SR_STACK_WINDOW];
    uint8_t *crx = sr_at(SR_CRX_BASE), *st = (uint8_t *)sp;
    unsigned stack_n = 0;
    if(!sr_mem.crx || model2_ram_mut((u32)(uintptr_t)a0) == NULL) {
        lifted_geo_view_scene_frame(a0, a1, a2);
        return;
    }
    if(st >= crx && st < crx + SR_CRX_SIZE) {
        stack_n = (unsigned)(crx + SR_CRX_SIZE - st);
        if(stack_n > SR_STACK_WINDOW) stack_n = SR_STACK_WINDOW;
    }
    regs_save(&entry);
    {
        SR_DIFF_BEGIN();
        lifted_geo_view_scene_frame(a0, a1, a2);
        regs_save(&after);
        if(stack_n) memcpy(stack, st, stack_n);
        SR_DIFF_NATIVE();
        regs_load(&entry);
        sr_player_scene_frame((u32)(uintptr_t)a0);
        if(stack_n) memcpy(st, stack, stack_n);
        regs_load(&after);
        SR_DIFF_END("geo_view_scene_frame");
    }
}
