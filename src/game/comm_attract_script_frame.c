/* Shared fp shadow for 0x104A0 + comm_attract_inner_3 script draws. */

#include "comm_attract_script_frame.h"
#include "i960_mem.h"
#include "model2_rom.h"

static u8 s_fp[COMM_ATTRACT_FP_SIZE];

static u32 fp_index(u32 fp_off)
{
    if (fp_off < COMM_ATTRACT_FP_BASE)
        return 0;
    fp_off -= COMM_ATTRACT_FP_BASE;
    if (fp_off >= COMM_ATTRACT_FP_SIZE)
        return COMM_ATTRACT_FP_SIZE - 4u;
    return fp_off;
}

u32 comm_attract_fp_u32(u32 fp_off)
{
    u32 idx = fp_index(fp_off);
    u32 v;

    v = (u32)s_fp[idx];
    v |= (u32)s_fp[idx + 1u] << 8;
    v |= (u32)s_fp[idx + 2u] << 16;
    v |= (u32)s_fp[idx + 3u] << 24;
    return v;
}

void comm_attract_fp_st_u32(u32 fp_off, u32 value)
{
    u32 idx = fp_index(fp_off);

    s_fp[idx] = (u8)value;
    s_fp[idx + 1u] = (u8)(value >> 8);
    s_fp[idx + 2u] = (u8)(value >> 16);
    s_fp[idx + 3u] = (u8)(value >> 24);
}

void comm_attract_script_frame_bind_fp(void)
{
    fp = (uintptr_t)s_fp - COMM_ATTRACT_FP_BASE;
}

void comm_attract_script_frame_load(void)
{
    u32 table_idx;
    u32 desc_base;
    u32 ix;
    u32 rec;
    u32 off;

    table_idx = i960_ld_u32(I960_WORKRAM, 0x20a800, 0);
    desc_base = i960_ld_u32(I960_WORKRAM, 0x20a80c, 0);

    /* lda (g4)[g4*8] → *9; lda (g5)[g4*4] → address of record (not a load). */
    ix = table_idx + (table_idx << 3);
    rec = desc_base + (ix << 2);

    for (off = 0; off < 0x24u; off++)
        s_fp[off] = model2_workram_mirror_u8(rec + off);
}
