/* Auto-lifted semantic C — edit by hand; not tier-3 byte-matched. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_03bcc0_e0.asm */
// @rom 0x3bcc0 +0xe0 geo_attract_course_carousel_draw

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "lift_syms.h"
#include "model2_rom.h"

#include <stdio.h>
#include <stdlib.h>

u32 geo_attract_course_carousel_draw(u32 arg0, void * arg1, u32 arg2)
{
    u32 course_ix;
    u32 scaled;
    u32 rec_va;
    u32 slot_va;
    u32 wr_slot;
    u32 wr_ptr;
    u32 span_a;
    u32 span_b;

    course_ix = arg0;
    /* @0x3BCC4: lda (g0)[g0*2] → course_ix*3 */
    scaled = course_ix + (course_ix << 1);
    /*
     * @0x3BCC8: lda 0x5dca00[scaled*4], r6 — effective address of the 3-word
     * course row (not a memory load). Row layout in maincpu mirror:
     *   +0 catalog threshold base, +4 slot record, +8 index list.
     */
    rec_va = 0x5dca00u + (scaled << 2);
    /* @0x3BCD0: ld 0x4(r6) → slot record for nearest-index walk. */
    slot_va = i960_ld_u32(I960_WORKRAM, rec_va, 4);
    /* arg1 is host fp-shadow pose (script_finish fp+0x40). */
    geo_attract_course_slot_call((void *)(uintptr_t)slot_va, arg1, arg2);

    if ((i32)g0 >= 0) {
        /* @0x3BCFC: r4=g0-1, r5=1, 0x2020a4←0 */
        span_a = g0 - 1u;
        span_b = 1u;
        i960_st_u32(I960_WORKRAM, 0x2020a4, 0, (u32)g14);
    } else {
        /* @0x3BCE0: r4=-1-g0, r5=-1, 0x2020a4←1 */
        span_a = (u32)(-1 - (i32)g0);
        span_b = (u32)-1;
        i960_st_u32(I960_WORKRAM, 0x2020a4, 0, 1u);
    }

    i960_mmio_write_u32(0x800160, (u32)g14);
    i960_mmio_write_u32(0x804000, 0x48c35000u);
    wr_slot = i960_ld_u32(I960_WORKRAM, 0x20a290, 0);
    wr_ptr = i960_ld_u32(I960_MMIO, 0x802008, 0);
    i960_st_u32(I960_WORKRAM, wr_slot, 0, wr_ptr);
    i960_mmio_write_u32(0x884000, 0x02800505u);
    i960_st_u32(I960_WORKRAM, 0x20a290, 0, wr_slot + 4u);
    i960_mmio_write_u32(0x801008, wr_ptr + 0x34u);

    /* @0x3BD40/@0x3BD50: g0=course_ix, g1=span_a → prg mode switch. */
    {
        const char *span_log = getenv("I960_GEO_SPAN_LOG");

        if (span_log && span_log[0] && span_log[0] != '0') {
            static unsigned s_span_log;
            static u32 s_prev_span = 0xffffffffu;
            static int s_prev_env = -1;
            u32 *pose = (u32 *)arg1;
            int env_on = (course_ix == 0u || course_ix == 4u)
                         && (span_a > 36u || span_a <= 7u);
            u32 cam = i960_ld_u32(I960_WORKRAM, 0x20a7f4, 0) & 0xffu;
            u32 tab = i960_ld_u32(I960_WORKRAM, 0x20a800, 0);
            int changed = (span_a != s_prev_span) || (env_on != s_prev_env);

            if (changed || s_span_log < 8u
                || ((course_ix == 0u) && (s_span_log % 45u) == 0u)) {
                fprintf(stderr,
                        "lift: carousel course=%u tab=%u cam=%u span_a=%u "
                        "span_b=%#x env921862=%d%s query=(%.3g,%.3g,%.3g) "
                        "slot_g0=%#x\n",
                        (unsigned)course_ix, (unsigned)tab, (unsigned)cam,
                        (unsigned)span_a, (unsigned)span_b, env_on,
                        changed ? " *" : "",
                        pose ? (float)i960_u32_to_f64(pose[0]) : 0.f,
                        pose ? (float)i960_u32_to_f64(pose[1]) : 0.f,
                        pose ? (float)i960_u32_to_f64(pose[2]) : 0.f,
                        (unsigned)g0);
                s_span_log++;
                s_prev_span = span_a;
                s_prev_env = env_on;
            }
        }
    }
    geo_attract_course_prg_mode(course_ix, span_a, arg2);

    /* @0x3BD6C: catalog_span(g0=r6 row ptr, g1=span_a, g2=span_b, g3=budget, g4=1). */
    g3 = arg2;
    g4 = 1u;
    geo_attract_course_catalog_span(rec_va, span_a, span_b);
    return span_a;
}
