/* Semantic lift: 4-wide FIFO upload wrapper @ ROM 0x2A6D0. */
/* source: decomp/disasm/maincpu/maincpu_02a6d0_d0.asm */
// @rom 0x2a6d0 +0x70 cgm_fifo_upload_wrapper

#include "i960_lift.h"

extern u32 cgm_fifo_upload_runner(u32 arg0, u32 arg1, u32 arg2);

/*
 * @0x2A6D0–0x2A740: save g0..g4, four ``call 0x2A5A0`` with permuted bounds.
 * Merge payload is always wrapper ``g4`` (runner ``r14``); D/U gate is entry
 * ``g5`` bit 0 on the first call, then prior runner return thereafter.
 * No FIFO ``ldos`` in this cluster — payload must already be in ``g4``.
 */
void cgm_fifo_upload_wrapper(u32 arg0, u32 arg1, u32 arg2)
{
    u32 sav_g0 = arg0;
    u32 sav_g1 = arg1;
    u32 sav_g2 = arg2;
    u32 sav_g3 = (u32)g3;
    u32 sav_g4 = (u32)g4;
    u32 flags = (u32)g5;

    /* Call 1 @ 0x2A6E8: g2←g0; other regs still entry values. */
    g0 = sav_g0;
    g1 = sav_g1;
    g2 = sav_g0;
    g3 = sav_g3;
    g4 = sav_g4;
    g5 = flags;
    flags = cgm_fifo_upload_runner((u32)g0, (u32)g1, (u32)g2);

    /* Call 2 @ 0x2A704. */
    g0 = sav_g0;
    g1 = sav_g3;
    g2 = sav_g2;
    g3 = sav_g3;
    g4 = sav_g4;
    g5 = flags;
    flags = cgm_fifo_upload_runner((u32)g0, (u32)g1, (u32)g2);

    /* Call 3 @ 0x2A720. */
    g0 = sav_g2;
    g1 = sav_g3;
    g2 = sav_g2;
    g3 = sav_g1;
    g4 = sav_g4;
    g5 = flags;
    flags = cgm_fifo_upload_runner((u32)g0, (u32)g1, (u32)g2);

    /* Call 4 @ 0x2A73C. */
    g0 = sav_g2;
    g1 = sav_g1;
    g2 = sav_g0;
    g3 = sav_g1;
    g4 = sav_g4;
    g5 = flags;
    (void)cgm_fifo_upload_runner((u32)g0, (u32)g1, (u32)g2);
}

/* @0x2A750–0x2A794: column span — one ``0x2A5A0`` per g1..g3. */
void cgm_fifo_upload_span(u32 arg0, u32 arg1, u32 arg2)
{
    u32 sav_g0 = arg0;
    u32 col = arg1;
    u32 sav_g2 = arg2;
    u32 col_end = (u32)g3;
    u32 payload = (u32)g4;
    u32 flags = (u32)g5;

    if ((i32)arg1 > (i32)g3)
        return;

    while ((i32)col <= (i32)col_end) {
        g0 = sav_g0;
        g1 = col;
        g2 = sav_g2;
        g3 = col;
        g4 = payload;
        g5 = flags;
        flags = cgm_fifo_upload_runner((u32)g0, (u32)g1, (u32)g2);
        col++;
    }
}
