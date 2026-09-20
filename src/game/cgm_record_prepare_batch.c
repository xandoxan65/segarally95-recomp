/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_02a290_50.asm @0x2A290 */
// @rom 0x2a290 +0x44 cgm_record_prepare_batch

#include "i960_lift.h"
#include "i960_mem.h"
#include "cgm_format.h"
#include "model2_memory.h"

#include <stdio.h>
#include <stdlib.h>

extern void libc_printf(const char * fmt, u32 arg1, u32 arg2);

/* @0x2A2C4: lda 0x005C9280,g0 — format bytes live at this workram cell. */
#define CGM_AUX_FORMAT  0x005c9280u

/*
 * @0x29AA8 (bal from 0x2A2B4): if g0 > 0x7f return -1; else st g0 → 0x20C950,
 * return 0. Single shot — not a decrement loop.
 */
static u32 cgm_batch_slot_gate(u32 probe)
{
    if (probe > 0x7fu)
        return 0u - 1u;
    i960_st_u32(I960_WORKRAM, CGM_SLOT_CURSOR, 0, probe);
    return 0u;
}

void cgm_record_prepare_batch(u32 arg0, u32 arg1, u32 arg2)
{
    u32 slot;
    u32 probe;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x29FF4: draw_scene uses r11 layer flags; restore after record dispatch. */
    g4 = r11;

    /* @0x2A290: ld 0x20C958; @0x2A298: cmpibne 0 → compile @ 0x2A2C4. */
    if (i960_ld_u32(I960_WORKRAM, CGM_G13_MASK, 0) == 0u) {
        /* @0x2A29C–0x2A2A8: seed 0x20C958 = slot<<7. */
        slot = i960_ld_u32(I960_WORKRAM, CGM_SLOT_CURSOR, 0);
        i960_st_u32(I960_WORKRAM, CGM_G13_MASK, 0, slot << 7);
        /* @0x2A2B0–0x2A2B8: slot+24 via bal 0x29AA8; g0==0 → early ret. */
        probe = cgm_batch_slot_gate(slot + 24u);
        if (probe == 0u) {
            g0 = 0u;
            return;
        }
        /* probe == -1 (overflow): fall through to compile. */
    }

    if (getenv("I960_TRACE_SPLASH")) {
        fprintf(stderr,
                "lift: prepare_batch compile aux=%#x slot=%u mask=%#x\n",
                CGM_AUX_FORMAT,
                (unsigned)i960_ld_u32(I960_WORKRAM, CGM_SLOT_CURSOR, 0),
                (unsigned)i960_ld_u32(I960_WORKRAM, CGM_G13_MASK, 0));
    }

    /*
     * @0x2A2C4: lda 0x005C9280,g0 → call 0x05CEC0.
     * Dispatch does mov g0,r12; ldob (r12) — format chars at the aux cell
     * (ROM mirror is "FBT Error!\\n" until runtime writes the buffer).
     */
    g0 = CGM_AUX_FORMAT;
    g1 = CGM_STAGE_BASE;
    libc_printf((const char *)(uintptr_t)CGM_AUX_FORMAT, CGM_STAGE_BASE, 0);
    g4 = r11;
    /* @0x2A2D0: subo 1,0,g0 */
    g0 = 0u - 1u;
}
