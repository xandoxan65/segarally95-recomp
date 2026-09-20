#ifndef COMM_ATTRACT_SCRIPT_FRAME_H
#define COMM_ATTRACT_SCRIPT_FRAME_H

#include "i960_lift.h"
#define COMM_ATTRACT_FP_BASE  0x40u
#define COMM_ATTRACT_FP_SIZE  0xd0u

void comm_attract_script_frame_load(void);
void comm_attract_script_frame_bind_fp(void);
u32 comm_attract_fp_u32(u32 fp_off);
void comm_attract_fp_st_u32(u32 fp_off, u32 value);

#endif
