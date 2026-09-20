#ifndef CGM_MERGE_H
#define CGM_MERGE_H

#include "i960_lift.h"

typedef struct {
    u16 *words;
    u32 word_cap;
    u32 merge_lo;
    u32 merge_mid;
    u32 merge_max;
    u32 merge_max2;
    u32 g13_mask;
} model2_cgm_merge_ctx;

/* @rom 0x2A4E0 — returns color15 or 0xffffffff on reject */
u32 palram_bus_merge(model2_cgm_merge_ctx *ctx, u32 g0, u32 g1, u32 g2);
void palram_bus_merge_host(u32 g0, u32 g1, u32 g2);

/* Lone-D: merge raw u16 + scratch (@ 0x2A4E0 / 0x2A490); no XOR. */
u32 cgm_d_merge_commit(u16 raw_u16);

#endif
