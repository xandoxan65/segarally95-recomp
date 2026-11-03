/* Sega Rally native Dreamcast entry: one mode, one tick per displayed frame.
 *
 *   default      the toolchain and frame clock check (RACE_TICK_LIMIT=N ends after N ticks)
 *   LOCKSTEP=1   replay /pc/race.pak and print its checkpoints (docs/dreamcast-port.md)
 *   PRACTICE=1   desert practice from /pc/practice.pak, drawn from /pc/desert.course, on the controller:
 *                stick steers, R accelerates, L brakes, Y / X shift up / down, A changes the view, START restarts */
#include <enDjinn/enj_enDjinn.h>
#include <stdio.h>
#include <stdlib.h>
#include "frame_clock.h"
#include "sr_alloc.h"
#include "lockstep.h"
#include "race/io_poll.h"
#include "race/race_tick.h"
#include "race/palette_irq.h"
#include "sr_mem.h"
#include <math.h>
#include "pc_prof.h"
#ifdef SR_PRACTICE
#include "render/ta.h"
#include "render/pvr_backend.h"
#include "model2_tgp.h"
#include <arch/timer.h>
#endif

typedef struct {
    sr_frame_clock_t clock;
    uint32_t ticks;
    uint32_t frames;
    uint32_t late_frames;
#ifdef SR_PRACTICE
    sr_course_t course;
    sr_actors_t actors; /* n = 0 without /pc/desert.attach */
    int hud2d;          /* the arcade's HUD (2D layer) is drawn: the text stand-in only shows the debug lines */
    uint32_t title_len, title_frames; /* the title card's frames (/pc/<course>.title), and those still to show */
    uint32_t gear;
    sr_render_stats_t stats;
    uint64_t render_us_total, render_us_max, tris_total, cycles_xform, cycles_emit, verts_total;
    uint64_t cycles_direct, cycles_deferred, cycles_material;
    uint32_t chunks_total, instances_total;
    uint32_t renders;
    /* CPU time of one frame: its race ticks and both render passes; frames over a vblank's 16.7 ms */
    uint64_t frame_us, frame_us_max;
    uint32_t frames_over, dropped;
    uint32_t attempt, record_ticks; /* RECORD=1 */
    int view_held;
    uint64_t pt_flush_us, pt_hud_us; /* the punch-through callback's parts: the deferred list, the HUD */
#endif
} sr_run_t;

static sr_run_t s_run;

static void render_text(void *data) {
    const sr_run_t *run = data;
    char line[96];
#ifdef SR_LOCKSTEP
    snprintf(line, sizeof(line), "lockstep race tick %lu", (unsigned long)sr_lockstep_race_tick());
    enj_qfont_write(line, 20 * ENJ_XSCALE, 160, PVR_LIST_PT_POLY);
#endif
#ifdef SR_PRACTICE
    /* The arcade HUD's values as text when the pack has no 2D layer (older packs); the debug lines always. */
    if(!run->hud2d) {
        int32_t left = ((int32_t)sr_ld32(0x20b0b0u) + 0x3b) / 60;
        const uint32_t lap = sr_ld32(0x2020bcu);
        const float kmh = fabsf(sr_f(sr_ld32(0x20b0b4u)) * 216.f);
        if(left > 99) left = 99;
        if(left < 0) left = 0;
        enj_font_scale_set(2);
        snprintf(line, sizeof(line), "TIME %2ld", (long)left);
        enj_qfont_write(line, 20 * ENJ_XSCALE, 16, PVR_LIST_PT_POLY);
        snprintf(line, sizeof(line), "%3d km/h  %lu", (int)kmh, (unsigned long)sr_ld32(0x500078u));
        enj_qfont_write(line, 400 * ENJ_XSCALE, 16, PVR_LIST_PT_POLY);
        const uint32_t cs = lap * 200u / 115u; /* a tick is 1/57.5 s */
        snprintf(line, sizeof(line), "LAP %lu'%02lu\"%02lu", (unsigned long)(cs / 6000), (unsigned long)(cs / 100 % 60),
                 (unsigned long)(cs % 100));
        enj_qfont_write(line, 20 * ENJ_XSCALE, 440, PVR_LIST_PT_POLY);
    }
    {
        enj_font_scale_set(1);
        snprintf(line, sizeof(line), "late %lu  tris %lu", (unsigned long)run->late_frames,
                 (unsigned long)run->stats.tris_drawn);
        enj_qfont_write(line, 460 * ENJ_XSCALE, 452, PVR_LIST_PT_POLY);
        /* what the game's poll read: wheel, accelerator, brake (0x202050..52), the lever's gear (0x202044) */
        snprintf(line, sizeof(line), "in %02x %02x %02x  lever %lu", (unsigned)sr_ld8(0x202050u),
                 (unsigned)sr_ld8(0x202051u), (unsigned)sr_ld8(0x202052u), (unsigned long)sr_ld32(0x202044u));
        enj_qfont_write(line, 460 * ENJ_XSCALE, 436, PVR_LIST_PT_POLY);
    }
    return;
#endif
    enj_font_scale_set(3);
    enj_qfont_write("SEGA RALLY native", 20 * ENJ_XSCALE, 40, PVR_LIST_PT_POLY);
    enj_font_scale_set(1);
    snprintf(line, sizeof(line), "tick %lu  late %lu", (unsigned long)run->ticks, (unsigned long)run->late_frames);
    enj_qfont_write(line, 20 * ENJ_XSCALE, 120, PVR_LIST_PT_POLY);
}

#ifdef SR_PRACTICE
static void render_pass(sr_run_t *run, int list) {
    const uint64_t t0 = timer_us_gettime64();
    float view[12];
    if(model2_tgp_view_matrix(view)) {
        sr_pvr_begin(list, view, &run->stats);
        sr_pvr_course(&run->course);
        sr_pvr_actors(&run->actors, &run->course);
    }
    run->frame_us += timer_us_gettime64() - t0;
}

/* The punch-through pass is the frame's last: send what the opaque pass deferred, close the frame's accounts. */
/* The cars' shadows: closed volumes on the opaque modifier list, darkening the course inside them. */
static void render_mod(void *data) {
    (void)data;
    sr_ta_begin(PVR_LIST_OP_MOD);
    sr_pvr_shadows_submit();
    sr_ta_finish();
}

/* The palette interrupt's white wash (race/palette_irq.h): the start's fade-in from white, the fly-in's wash. */
static void render_wash(void *data) {
    (void)data;
    sr_ta_begin(PVR_LIST_TR_POLY);
    sr_pvr_wash(sr_palette_wash());
    sr_ta_finish();
}

static void render_pt(void *data) {
    sr_run_t *run = data;
    const uint64_t before = run->frame_us, t0 = timer_us_gettime64();
    SR_PC_PROF_PHASE(SR_PC_PROF_RENDER);
    run->dropped = sr_pvr_flush_deferred();
    run->pt_flush_us += timer_us_gettime64() - t0;
    sr_ta_begin(PVR_LIST_PT_POLY);
    sr_pvr_countdown(&run->course);
    {
        const uint64_t t1 = timer_us_gettime64();
        sr_pvr_hud_submit();
        sr_pvr_hud_needle();
        run->pt_hud_us += timer_us_gettime64() - t1;
    }
    sr_ta_finish();
    run->frame_us += timer_us_gettime64() - t0;
    run->render_us_total += run->frame_us - before;
    if(run->frame_us > run->frame_us_max) run->frame_us_max = run->frame_us;
    if(run->frame_us > 16683) run->frames_over++;
    run->tris_total += run->stats.tris_drawn;
    run->verts_total += run->stats.verts_transformed;
    run->chunks_total += run->stats.chunks_drawn;
    run->instances_total += run->stats.instances_drawn;
    run->cycles_xform += run->stats.cycles_xform;
    run->cycles_emit += run->stats.cycles_emit;
    run->cycles_direct += run->stats.cycles_direct;
    run->cycles_deferred += run->stats.cycles_deferred;
    run->cycles_material += run->stats.cycles_material;
    run->renders++;
    SR_PC_PROF_PHASE(SR_PC_PROF_OTHER);
}

#ifdef SR_RENDER_PROF
/* RENDER_PROF=1: PRFC1 counts one pipeline-freeze cause over the render pass, a different one every 100 frames. */
#include <dc/perfctr.h>
static const struct {
    perf_cntr_event_t event;
    const char *name;
} s_freeze[] = {
    {PMCR_PIPELINE_FREEZE_BY_DCACHE_MISS_MODE, "data cache miss"},
    {PMCR_PIPELINE_FREEZE_BY_ICACHE_MISS_MODE, "instruction cache miss"},
    {PMCR_PIPELINE_FREEZE_BY_CPU_REGISTER_MODE, "register dependency"},
    {PMCR_PIPELINE_FREEZE_BY_FPU_MODE, "FPU"},
    {PMCR_PIPELINE_FREEZE_BY_BRANCH_MODE, "branch"},
    {PMCR_INSTRUCTION_ISSUED_MODE, "instructions issued (count)"},
};
#define N_FREEZE (sizeof(s_freeze) / sizeof(s_freeze[0]))
static uint64_t s_freeze_total[N_FREEZE], s_freeze_xform[N_FREEZE], s_freeze_emit[N_FREEZE];
static uint32_t s_freeze_frames[N_FREEZE];
#endif

/* The title card: the whole frame, no 3D. */
static void render_title(void *data) {
    (void)data;
    sr_ta_begin(PVR_LIST_OP_POLY);
    sr_pvr_title_draw();
    sr_ta_finish();
}

static void render_op(void *data) {
    sr_run_t *run = data;
    const uint64_t before = run->frame_us;
    SR_PC_PROF_PHASE(SR_PC_PROF_RENDER);
#ifdef SR_RENDER_PROF
    const uint32_t which = (run->renders / 100) % N_FREEZE;
    perf_cntr_start(PRFC1, s_freeze[which].event, PMCR_COUNT_CPU_CYCLES);
#endif
    run->stats = (sr_render_stats_t){0};
    sr_ta_begin(PVR_LIST_OP_POLY);
    render_pass(run, SR_LIST_BOTH);
    sr_ta_finish();
#ifdef SR_RENDER_PROF
    s_freeze_total[which] += perf_cntr_count(PRFC1);
    s_freeze_xform[which] += run->stats.freeze_xform;
    s_freeze_emit[which] += run->stats.freeze_emit;
    s_freeze_frames[which]++;
    perf_cntr_stop(PRFC1);
    perf_cntr_clear(PRFC1);
#endif
    run->render_us_total += run->frame_us - before;
    if(run->frame_us - before > run->render_us_max) run->render_us_max = run->frame_us - before;
    SR_PC_PROF_PHASE(SR_PC_PROF_OTHER);
}

#ifndef SR_PRACTICE_REPLAY
#ifdef SR_RECORD
/* RECORD=1: the drive so far to /pc/driveN.inputs, N counting the attempts (START begins the next). */
static void record_save(sr_run_t *run) {
    char path[32];
    snprintf(path, sizeof(path), "/pc/drive%lu.inputs", (unsigned long)run->attempt);
    if(sr_session_record_save(path) != 0) printf("sr: could not write %s\n", path);
}
#endif

/* The cabinet's controls from a Dreamcast pad: the stick across the wheel's whole range (0x00..0xff, 0x80 centre),
 * pedals 0..0xe0. */
static void practice_controls(sr_run_t *run, uint32_t *steer, uint32_t *accel, uint32_t *brake) {
    enj_ctrlr_state_t **states = enj_ctrl_get_states();
    const enj_ctrlr_state_t *pad = states ? states[0] : NULL;
    *steer = 0x80;
    *accel = *brake = 0;
    if(!pad) return;
    *steer = (uint32_t)(0x80 + (int32_t)pad->joyx);
    *accel = (uint32_t)pad->rtrigger * 0xe0u / 255u;
    *brake = (uint32_t)pad->ltrigger * 0xe0u / 255u;
    sr_io_board_view(pad->button.A & ENJ_BUTTON_DOWN);
    run->view_held = (pad->button.A & ENJ_BUTTON_DOWN) != 0;
    if(pad->button.Y == ENJ_BUTTON_DOWN_THIS_FRAME && run->gear < 4) run->gear++;
    if(pad->button.X == ENJ_BUTTON_DOWN_THIS_FRAME && run->gear > 1) run->gear--;
    if(pad->button.START == ENJ_BUTTON_DOWN_THIS_FRAME) {
#ifdef SR_RECORD
        record_save(run);
        run->attempt++;
        sr_session_record_reset();
#endif
        sr_session_restart();
        run->gear = 1;
        run->title_frames = run->title_len; /* the card again, then the countdown */
        sr_pvr_title_begin();
    }
}
#endif
#endif

static void race_updater(void *data) {
    sr_run_t *run = data;
    const uint32_t ticks = sr_frame_ticks(&run->clock);
    if(ticks > 1u) run->late_frames++;
    run->ticks += ticks;
    run->frames++;
#ifdef SR_PRACTICE
    const uint64_t t_update = timer_us_gettime64();
    run->frame_us = 0;
    /* The title card (the arcade's scene 4) before the countdown: the race does not tick, nothing else is drawn.
     * Its frames do not count towards RACE_TICK_LIMIT. */
    if(run->title_frames) {
        run->ticks -= ticks;
        run->title_frames = ticks >= run->title_frames ? 0 : run->title_frames - ticks;
        if(!run->title_frames) sr_pvr_hud_reset_palette();
        enj_render_list_add(PVR_LIST_OP_POLY, render_title, run);
        return;
    }
#endif
#ifdef SR_LOCKSTEP
    /* One race tick per frame, whatever the clock says: the run checks exactness and measures each tick. */
    if(!sr_lockstep_tick()) {
        sr_lockstep_report();
        enj_state_flag_shutdown(NULL);
        return;
    }
#endif
#ifdef SR_PRACTICE
#ifdef SR_PRACTICE_REPLAY
    /* PRACTICE_REPLAY=1: drive the pack's recorded inputs instead of the pad (a hands-free run to film). */
    {
        uint32_t i;
        SR_PC_PROF_PHASE(SR_PC_PROF_TICK);
        for(i = 0; i < ticks; i++)
            if(!sr_lockstep_tick()) break;
        SR_PC_PROF_PHASE(SR_PC_PROF_OTHER);
    }
#else
    {
        uint32_t steer, accel, brake, i;
        practice_controls(run, &steer, &accel, &brake);
        SR_PC_PROF_PHASE(SR_PC_PROF_TICK);
#ifdef SR_RECORD
        for(i = 0; i < ticks; i++) {
            sr_session_record_tick(steer, accel, brake, run->gear, run->view_held);
            /* every ten seconds: a drive is kept even if the program is ended with the reset buttons */
            if(++run->record_ticks % 600u == 0) record_save(run);
        }
#else
        for(i = 0; i < ticks; i++) sr_session_tick(steer, accel, brake, run->gear);
#endif
        SR_PC_PROF_PHASE(SR_PC_PROF_OTHER);
    }
#endif
    run->frame_us += timer_us_gettime64() - t_update;
    enj_render_list_add(PVR_LIST_OP_POLY, render_op, run);
    enj_render_list_add(PVR_LIST_PT_POLY, render_pt, run);
    if(sr_palette_wash()) enj_render_list_add(PVR_LIST_TR_POLY, render_wash, run);
    {
        float view[12];
        if(model2_tgp_view_matrix(view) && sr_pvr_shadows_prepare(&run->actors, &run->course, view))
            enj_render_list_add(PVR_LIST_OP_MOD, render_mod, run);
    }
#endif
#ifdef SR_RACE_TICK_LIMIT
    if(run->ticks >= SR_RACE_TICK_LIMIT) {
        printf("sr: run ticks=%lu frames=%lu late=%lu\n", (unsigned long)run->ticks, (unsigned long)run->frames,
               (unsigned long)run->late_frames);
#ifdef SR_PRACTICE
        sr_lockstep_report();
        if(run->renders)
            printf("sr: render avg %lu us (both passes), opaque pass max %lu us, %lu triangles a frame; frame CPU "
                   "max %lu us, %lu frames over 16.7 ms\n",
                   (unsigned long)(run->render_us_total / run->renders), (unsigned long)run->render_us_max,
                   (unsigned long)(run->tris_total / run->renders), (unsigned long)run->frame_us_max,
                   (unsigned long)run->frames_over);
        if(run->renders) /* 200 cycles a microsecond */
            printf("sr: render a frame: %lu instances, %lu chunks, %lu vertices; transform %lu us, emit %lu us\n",
                   (unsigned long)(run->instances_total / run->renders),
                   (unsigned long)(run->chunks_total / run->renders), (unsigned long)(run->verts_total / run->renders),
                   (unsigned long)(run->cycles_xform / 200 / run->renders),
                   (unsigned long)(run->cycles_emit / 200 / run->renders));
        if(run->renders && run->cycles_material)
            printf("sr: emit parts a frame: direct %lu us, deferred %lu us, material %lu us\n",
                   (unsigned long)(run->cycles_direct / 200 / run->renders),
                   (unsigned long)(run->cycles_deferred / 200 / run->renders),
                   (unsigned long)(run->cycles_material / 200 / run->renders));
#ifdef SR_RENDER_PROF
        {
            uint32_t k;
            for(k = 0; k < N_FREEZE; k++)
                if(s_freeze_frames[k])
                    printf("sr: render pass, %s: %lu us a frame (transform %lu, emit %lu; %lu frames)\n",
                           s_freeze[k].name, (unsigned long)(s_freeze_total[k] / 200 / s_freeze_frames[k]),
                           (unsigned long)(s_freeze_xform[k] / 200 / s_freeze_frames[k]),
                           (unsigned long)(s_freeze_emit[k] / 200 / s_freeze_frames[k]),
                           (unsigned long)s_freeze_frames[k]);
        }
#endif
        sr_pvr_particle_report();
        sr_pvr_hud_report();
        sr_pc_prof_dump();
#ifdef SR_TICK_PROF
        {
            static const char *names[SR_TICK_PROF_PARTS] = {"camera", "race flow", "pens+spawn", "course view",
                                                             "particles", "race list", "prg slot/HUD"};
            int k;
            for(k = 0; k < SR_TICK_PROF_PARTS; k++)
                printf("sr: tick part %s: %lu us a tick\n", names[k],
                       (unsigned long)(sr_tick_prof[k] / 200u / (run->ticks ? run->ticks : 1u)));
        }
#endif
        if(run->renders)
            printf("sr: punch-through callback a frame: deferred list %lu us, HUD %lu us\n",
                   (unsigned long)(run->pt_flush_us / run->renders), (unsigned long)(run->pt_hud_us / run->renders));
        printf("sr: punch-through primitives dropped (deferred list full): %lu\n", (unsigned long)run->dropped);
#endif
        enj_state_flag_shutdown(NULL);
        return;
    }
#endif
    enj_render_list_add(PVR_LIST_PT_POLY, render_text, run);
}

#ifdef SR_PRACTICE
static uint8_t *load_file(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    uint8_t *data;
    long n;
    if(!f) return NULL;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    data = sr_alloc((size_t)n);
    if(data && fread(data, 1, (size_t)n, f) != (size_t)n) {
        free(data);
        data = NULL;
    }
    fclose(f);
    *size = (size_t)n;
    return data;
}

/* The practice course in the pack's start (0x20a8c4, as the host's I960_HOST_PRACTICE_COURSE set it) names the
 * files drawn for it: /pc/NAME.course, .attach, .light and .title. */
static const char *course_name(void) {
    static const char *const names[4] = {"desert", "forest", "mountain", "lakeside"};
    const uint32_t course = sr_ld32(0x20a8c4u);
    return course < 4 ? names[course] : names[0];
}

static uint8_t *load_course_file(const char *ext, size_t *size) {
    char path[48];
    snprintf(path, sizeof path, "/pc/%s.%s", course_name(), ext);
    return load_file(path, size);
}

static int load_course(sr_course_t *course, sr_actors_t *actors) {
    size_t n;
    uint8_t *data = load_course_file("course", &n), *attach;
    if(!data || sr_course_parse(course, data, n)) return -1;
    printf("sr: course %s: %lu meshes, %lu instances\n", course_name(), (unsigned long)course->n_meshes,
           (unsigned long)course->n_instances);
    /* the attach file is optional: without it no cars or helicopter */
    attach = load_course_file("attach", &n);
    if(attach && sr_actors_parse(actors, course, attach, n) == 0)
        printf("sr: %s.attach: %lu attachments\n", course_name(), (unsigned long)actors->n);
    else
        actors->n = 0;
    free(attach);
    /* the cars' per-triangle lighting (optional: without it they keep the baked brightness) */
    attach = load_course_file("light", &n);
    if(attach && sr_actors_light_load(course, attach, n) > 0) printf("sr: %s.light: cars lit\n", course_name());
    free(attach);
    return 0;
}
#endif

int main(__unused int argc, __unused char **argv) {
#ifdef SR_LOCKSTEP
    if(sr_lockstep_load("/pc/race.pak") != 0) return -1;
#endif
#ifdef SR_PRACTICE
    if(sr_lockstep_load("/pc/practice.pak") != 0 || load_course(&s_run.course, &s_run.actors) != 0) {
        printf("sr: practice needs /pc/practice.pak and the /pc/<course>.course it names\n");
        return -1;
    }
#ifdef SR_DEMO
    sr_demo_endless_time = 1;
#endif
#ifdef SR_RECORD
    sr_demo_endless_time = 1; /* as the demo packs are made (SR_ENDLESS_TIME) */
    sr_session_record_reset();
    s_run.attempt = 1;
#endif
    sr_session_keep_start();
    s_run.gear = 1;
#endif
    enj_state_init_defaults();
#ifdef SR_PRACTICE
    {
        /* The course is drawn without the arcade's sky planes yet: clear to its sky's colour (forest, mountain and
         * lakeside: the average of the reference's sky, clouds included). */
        static const uint32_t sky[4] = {0x005a8cc8u, 0x006585d7u, 0x0092a0d2u, 0x007e789fu};
        enj_state_get()->video.bg_color.raw = sky[sr_ld32(0x20a8c4u) & 3u];
    }
#endif
    if(enj_state_startup() != 0) return -1;
    sr_pc_prof_start();
#ifdef SR_PRACTICE
    if(sr_pvr_course_init(&s_run.course) != 0) return -1;
    s_run.hud2d = sr_pvr_hud_init(sr_lockstep_chars(), sr_lockstep_palette(), &s_run.course) == 0;
    {
        /* the title card is optional (tools/make_title_image.py): without it the race starts at the countdown */
        size_t n;
        uint8_t *title = load_course_file("title", &n);
        s_run.title_len = s_run.title_frames = title ? sr_pvr_title_load(title, n) : 0;
        free(title);
        if(s_run.title_len) {
            printf("sr: title card, %lu frames\n", (unsigned long)s_run.title_len);
            sr_pvr_title_begin();
        }
    }
#endif
    enj_mode_t race = {.name = "Race", .mode_updater = race_updater, .data = &s_run};
    enj_mode_push(&race);
    enj_state_run();
    return 0;
}
