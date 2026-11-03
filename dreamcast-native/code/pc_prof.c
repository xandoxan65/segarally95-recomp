/* PC_PROF=1: a sampling profiler (include/pc_prof.h). TMU1 interrupts PC_PROF_HZ times a second and records the
 * interrupted program counter with the phase main.c set; sr_pc_prof_dump writes the samples to /pc/pc_prof.bin as
 * little-endian words, phase << 24 | (pc & 0xffffff) (the program runs at 0x8c000000..), which tools/pc_prof.py
 * turns into a table of functions per phase. */
#include "pc_prof.h"
#ifdef SR_PC_PROF
#include <arch/irq.h>
#include <arch/timer.h>
#include <stdio.h>
#include <stdlib.h>

#define PC_PROF_HZ 5003u /* not a multiple of the frame rate */
#define PC_PROF_MAX (128u * 1024u) /* 26 s */

volatile uint32_t sr_pc_prof_phase;
static uint32_t *s_samples;
static volatile uint32_t s_n;

static void sample(irq_t src, irq_context_t *context, void *data) {
    (void)src;
    (void)data;
    timer_clear(TMU1);
    if(s_n < PC_PROF_MAX) {
        s_samples[s_n++] = sr_pc_prof_phase << 24 | (context->pc & 0xffffffu);
    }
}

void sr_pc_prof_start(void) {
    s_samples = malloc(PC_PROF_MAX * 4u);
    if(!s_samples) {
        printf("sr: PC profile: no RAM\n");
        return;
    }
    irq_set_handler(EXC_TMU1_TUNI1, sample, NULL);
    timer_prime(TMU1, PC_PROF_HZ, 1);
    timer_start(TMU1);
}

void sr_pc_prof_dump(void) {
    FILE *f;
    if(!s_samples) return;
    timer_stop(TMU1);
    f = fopen("/pc/pc_prof.bin", "wb");
    if(f) {
        fwrite(s_samples, 4u, s_n, f);
        fclose(f);
    }
    printf("sr: PC profile: %lu samples to /pc/pc_prof.bin\n", (unsigned long)s_n);
}
#endif
