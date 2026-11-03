/* PC_PROF=1 builds: a sampling profiler (code/pc_prof.c). main.c sets sr_pc_prof_phase to what the frame is doing,
 * so the table splits by phase. Without PC_PROF the calls compile to nothing. */
#ifndef SR_PC_PROF_H
#define SR_PC_PROF_H
#include <stdint.h>

#define SR_PC_PROF_OTHER 0
#define SR_PC_PROF_TICK 1   /* the race ticks */
#define SR_PC_PROF_RENDER 2 /* the render callbacks */

#ifdef SR_PC_PROF
extern volatile uint32_t sr_pc_prof_phase;
void sr_pc_prof_start(void);
void sr_pc_prof_dump(void);
#define SR_PC_PROF_PHASE(p) (sr_pc_prof_phase = (p))
#else
#define sr_pc_prof_start() ((void)0)
#define sr_pc_prof_dump() ((void)0)
#define SR_PC_PROF_PHASE(p) ((void)0)
#endif

#endif
