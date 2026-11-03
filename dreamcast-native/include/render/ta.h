/* The Dreamcast's TA data through the store queues: every 32-byte header or vertex goes to a store queue address
 * that advances through the TA's 8 MiB input window (the TA takes any address in it, so consecutive 32-byte
 * blocks, such as a 64-byte sprite's halves, simply follow each other; KOS's pvr_dr toggles between two).
 *
 *   sr_ta_begin(list); p = sr_ta_target(); fill 32 bytes at p; sr_ta_commit(p); ...; sr_ta_finish();
 *
 * between KOS's pvr_list_begin and pvr_list_finish of that list (enDjinn's render callbacks are), which hold the
 * store queues for the TA.
 *
 * Sending lists by DMA instead (KOS's vertex DMA mode) was measured and is slower here: docs/dreamcast-port.md. */
#ifndef SR_RENDER_TA_H
#define SR_RENDER_TA_H
#include <stdint.h>
#include <dc/pvr.h>
#include <dc/sq.h>

extern uint32_t sr_ta_sq; /* the store queue address of the next 32 bytes */

static inline void sr_ta_begin(pvr_list_t list) {
    (void)list;
    sr_ta_sq = (uint32_t)SQ_MASK_DEST(PVR_TA_INPUT);
}

static inline void sr_ta_finish(void) {
}

static inline void *sr_ta_target(void) {
    const uint32_t p = sr_ta_sq;
    sr_ta_sq = p + 32u;
    return (void *)p;
}

/* Send the 32 bytes at p (from sr_ta_target): the asm reads all of them, so their stores come first. */
static inline void sr_ta_commit(void *p) {
    __asm__ volatile("pref @%1" : : "m"(*(const pvr_poly_hdr_t *)p), "r"(p));
}

#endif
