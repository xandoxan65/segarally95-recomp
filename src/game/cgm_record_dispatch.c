/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_02a0f8_400.asm @0x2A0F0 */
// @rom 0x2a0f8 +0x28 cgm_record_dispatch

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "i960_host_invoke.h"
#include "i960_host_staging.h"
#include "model2_memory.h"
#include "model2_rom.h"

#include "cgm_format.h"

/* convention: kind=leaf_bx  args g0,g2,g1  link g14→g1 bx */
/* abi: u32 arg0=g0, u32 arg1=g2 → void */

#define CGM_RECORD_TRAMPOLINE  0x005c9118u
#define CGM_DISPATCH_REC_BASE  0x005c9300u
#define I960_RET_INSN          0x0a000000u

void cgm_record_dispatch_reset(void)
{
}

static u16 rec_ldis(u32 rec, u32 off)
{
    const u8 *p = model2_rom_at(rec + off);

    if (rec >= WORKRAM_BASE && rec < WORKRAM_BASE + WORKRAM_SIZE)
        return i960_ld_u16(I960_WORKRAM, rec, off);
    if (!p)
        return 0;
    return (u16)p[0] | ((u16)p[1] << 8);
}

u32 cgm_dispatch_record_build(u32 handler_rom)
{
    u32 rec = CGM_DISPATCH_REC_BASE;

    i960_st_u16(I960_WORKRAM, rec, 0x10, 0);
    i960_st_u16(I960_WORKRAM, rec, 0x12, 1);
    i960_st_u32(I960_WORKRAM, rec, 0x14, handler_rom);
    return rec;
}

void cgm_record_dispatch(u32 arg0, u32 arg1)
{
    u32 rec = arg0;
    u32 index;
    u32 handler_wr;
    u32 handler_rom;

    (void)arg1;
    /* @0x2A0F0: lda 0x005C9118,g14; @0x2A0F8: mov g14,g1; @0x2A0FC: mov 0,g14 */
    g14 = CGM_RECORD_TRAMPOLINE;
    g1 = CGM_RECORD_TRAMPOLINE;
    g14 = 0;

    if (rec == 0u)
        return;
    if (rec < WORKRAM_BASE && !model2_rom_at(rec)) {
        g0 = 0;
        return;
    }

    g4 = rec_ldis(rec, 0x10u);
    g5 = rec_ldis(rec, 0x12u);
    index = (u32)g4 * (u32)g5;
    /*
     * @0x2A10C lda 0x14(g0)[g4*2],g0 — i960 lda = effective address, not a load.
     * For pixmap records this is the next stream record (after w*h u16 pixels),
     * which catalog_draw_setup @0x29FC4 writes back to fp+0x40. Treating this as
     * ld left g0 as ASCII from the next name; prepare_batch then returned -1 and
     * INSERT COIN only ever registered glyph 0 (sa_insert).
     */
    g0 = rec + 0x14u + index * 2u;

    handler_wr = i960_ld_u32(I960_WORKRAM, CGM_RECORD_TRAMPOLINE, 0);
    handler_rom = i960_host_resolve_call_target(handler_wr);
    if (handler_rom == 0u || handler_rom == 0x0002a118u || handler_rom == I960_RET_INSN) {
        /*
         * Static ROM: trampoline unpatched → bx lands on ret @ 0x2A118.
         * Hardware only advances the stream (g0 = next-record EA from lda).
         * Do not invent prepare_batch / 0x1111 flush here — those run only when
         * the runtime trampoline at 0x5C9118 is patched to a real handler.
         */
        u32 next = (u32)g0;

        g0 = next;
        if (next >= MAIN_DATA_A
            || (next >= CGM_STAGE_DATA && next < CGM_STAGE_DATA + 0x2000u))
            i960_st_u32(I960_WORKRAM, CGM_STAGE_BASE, 0, next);
        return;
    }

    if (i960_host_invoke_lifted(handler_rom))
        return;

    /* Handler not yet lifted — fall back to ROM trampoline dispatch. */
    i960_call_indirect(CGM_RECORD_TRAMPOLINE);
}
