/* One race tick in the original's order (game_dispatch_main @ 0x3150's loop in mode 3, with the practice race
 * frame @ 0x1BBA0 as its handler): bookkeeping, camera, race flow, object lists, the node handlers, then the
 * vblank's work and the control poll that the next tick reads.
 *
 * Drawing, the HUD's 2D layer and the palette fades are left to the renderer (hooks). The tick covers the race
 * frame's sub-states of 0x2020a8 (race_phases.h): setup, countdown, the race, the finish and time-up fanfares,
 * until a fanfare advances the scene table 0x2020ac and the race frame is left. */
#include "race/race_tick.h"
#include "race/camera.h"
#include "race/course.h"
#include "race/course_view.h"
#include "race/car.h"
#include "race/io_poll.h"
#include "race/particles.h"
#include "race/race_flow.h"
#include "race/race_phases.h"
#include "race/palette_irq.h"
#include "race/render_hooks.h"
#include "race/sound_cmd.h"
#include "sr_mem.h"
#include "sr_tgp.h"
#include "model2_tgp.h"
#include <stdio.h>

static void not_ported(const char *what, uint32_t value) {
    static uint32_t reported[8];
    static unsigned n;
    unsigned i;
    for(i = 0; i < n; i++)
        if(reported[i] == value) return;
    if(n < 8) reported[n++] = value;
    printf("sr: %s %#lx is not ported yet\n", what, (unsigned long)value);
}

/* Handlers whose ports are still in progress: weak, so the real ones replace them when linked. */
__attribute__((weak)) void sr_desert_node_frame(uint32_t node) { not_ported("desert object node", node); }
__attribute__((weak)) void sr_player_frame_update(uint32_t node, uint32_t a1[3], uint32_t a2[3], uint32_t corr) {
    (void)a1;
    (void)a2;
    (void)corr;
    not_ported("player frame update at node", node);
}

/* game_dispatch_main @ 0x3198-0x31E8 without a comm board: the poll counter, then a 4-word status copy. */
static void comm_status(void) {
    const uint32_t count = sr_ld32(0x20a754u);
    uint32_t base, i;
    if(sr_ld8(0x1a10000u) != 1) {
        sr_st32(0x20a758u, 1);
        sr_st32(0x20a754u, count + 1);
    }
    base = 0x1a121c0u + ((sr_ld32(0x20a534u) * 7u) << 6);
    sr_st32(0x20a550u, sr_ld32(base + 4));
    for(i = 0; i < 4; i++) sr_st32(0x20a540u + i * 4, sr_ld32(base + 8 + i * 4));
}

/* The race list (0x20c97c .. sentinel 0x20ca10): each node's handler, by its staged ROM address. */
static void list_walk(void) {
    uint32_t node = sr_ld32(0x20c97cu), guard = 0;
    while(node != 0x20ca10u && ++guard <= 128) {
        const uint32_t handler = sr_ld32(node + 4);
        switch(handler) {
        case 0x5d3f40u: sr_player_scene_frame(node); break;
        case 0x5cacf0u: sr_car_frame(node); break;
        case 0x5e17a0u: sr_desert_node_frame(node); break;
        case 0:
            /* 0x2AB94: the walk moves to the next node and unlinks that one's predecessor (this node) */
            node = sr_ld32(node + 0xc);
            sr_race_list_unlink(sr_ld32(node + 8));
            continue;
        default: not_ported("race list handler", handler); break;
        }
        node = sr_ld32(node + 0xc);
    }
}

/* TICK_PROF=1 (Dreamcast): CPU cycles per part of the tick, summed in sr_tick_prof (race_tick.h). */
uint64_t sr_tick_prof[SR_TICK_PROF_PARTS];
#ifdef SR_TICK_PROF
#include <dc/perfctr.h>
#define TP_START() uint64_t tp_last = perf_cntr_count(PRFC0)
#define TP(part) do { const uint64_t tp_now = perf_cntr_count(PRFC0); sr_tick_prof[part] += tp_now - tp_last; tp_last = tp_now; } while(0)
#else
#define TP_START() (void)0
#define TP(part) (void)0
#endif

/* The practice race frame (0x1BBA0), its gameplay part. */
static void race_frame(void) {
    TP_START();
    {
        uint32_t scale = 0x41c80000u; /* 25.0 */
        if((int32_t)sr_ld32(0x20a560u) > 0) {
            const uint32_t count = sr_ld32(0x213978u);
            if((int32_t)count < 2) scale = sr_b((float)(11.25 - (double)(int32_t)(2u - count) * 2.5));
        }
        sr_st32(0x213970u, scale);
    }
    sr_geo_frame_begin();
    sr_tgp_cmd(0x11802323u, 0, 0, 0, 0);
    sr_tgp_cmd(0x12802525u, 0, 0, 0, 0);
    sr_race_idle_wheel(); /* host reference: wheel held idle until GO */
    sr_cam_bind(0, 0);
    TP(0);
    switch((int32_t)sr_ld32(0x2020a8u) >> 1) {
    case 0: sr_race_slot0(); break;
    case 1: sr_race_slot1(); break;
    case 2: sr_race_slot2(); break;
    case 3: sr_race_slot3(); break;
    case 4: sr_race_slot4(); break;
    case 5: sr_race_slot5(); break;
    default: not_ported("race sub-state", sr_ld32(0x2020a8u)); break;
    }
    sr_obj_bind(sr_ld32(0x20a560u));
    sr_list_flag_prep(sr_ld32(0x20aac0u), 0x1bc54u);
    sr_race_prg_budget();
    TP(1);
    /* The cars' pens and the particles they spawn (race/particles.h), from the camera's latched view. */
    model2_tgp_restore_latched_view_matrix();
    sr_race_pens_before_course();
    TP(2);
    /* Course section (0x1BCxx-0x1BD24: acc, view bind). The reference draws the course events before it (not
     * ported: their polygon counts in 0x20b940 only change the course budget) and restores the camera's latched
     * view on the TGP first. Then the particle submit and, in the behind view, the followed car. */
    model2_tgp_restore_latched_view_matrix();
    sr_course_view_frame(sr_race_course_index());
    TP(3);
    sr_race_pens_after_course();
    TP(4);
    if(sr_ld32(0x213a04u) == 1) sr_st32(0x213a04u, 2);
    /* The node handlers start from the camera's latched view (the reference restores it before the walk). */
    model2_tgp_restore_latched_view_matrix();
    list_walk();
    TP(5);
    sr_race_prg_slot(0); /* the countdown / fanfare / HUD handler */
    TP(6);
}

int sr_demo_endless_time;

/* The vblank's work (geo_vsync_wait @ 0x4758 and the interrupt it stands for). */
static void vblank(void) {
    uint32_t frame = sr_ld32(0x202260u);
    sr_st32(0x202004u, 0);
    sr_st32(0x202008u, sr_ld32(0x202008u) + 1);
    sr_palette_irq_frame();
    sr_sound_drain();
    frame = frame + 1 >= 2 ? 0 : frame + 1;
    sr_st32(0x202260u, frame);
}

void sr_race_tick(void) {
    comm_status();
    sr_st32(0x20a290u, 0x918000u); /* backup_sram_sync: the TGP parameter list restarts every frame */
    race_frame();
    vblank();
    /* game_mode_apply: frame flag, then the controls for the next tick. */
    sr_st32(0x202000u, 1);
    sr_st32(0x2020a0u, 0);
    if(sr_demo_endless_time && (int32_t)sr_ld32(0x214120u) > 0) {
        /* demo drives only (not the arcade): below 15 s left, 30 s more, as the host's I960_HOST_ENDLESS_TIME does at
         * the same point, before the poll */
        const int32_t left = (int32_t)sr_ld32(0x20b0b0u);
        if(left < 15 * 60) sr_st32(0x20b0b0u, (uint32_t)(left + 30 * 60));
    }
    sr_io_poll();
    sr_geo_frame_end();
}
