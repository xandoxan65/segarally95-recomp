/* Course section selection (code/race/course_view.c) against the lifted originals; the course drawing hooks
 * for the diff build write the GEO ports as the originals do. */
#include "wrap_common.h"
#include "race/course_view.h"
#include "race/course_hooks.h"
#include "i960_mem.h"

void sr_hook_course_bank(uint32_t bank) { i960_mmio_write_u32(0x800010u + (bank << 10), 0u); }
void sr_hook_course_prg(uint32_t w) { i960_mmio_write_u32(0x804000u, w); }
void sr_hook_course_catalog(uint32_t w0, uint32_t w1, uint32_t w2, uint32_t w3) {
    i960_mmio_write_u32(0x804000u, w0);
    i960_mmio_write_u32(0x804000u, w1);
    i960_mmio_write_u32(0x804000u, w2);
    i960_mmio_write_u32(0x804000u, w3);
}
void sr_hook_course_reg(uint32_t addr, uint32_t value) { i960_mmio_write_u32(addr, value); }
uint32_t sr_hook_course_write_ptr(void) { return i960_mmio_read_u32(0x802008u); }

SR_DIFF_WRAP3(game_start_race_prg_budget, sr_race_prg_budget())
SR_DIFF_WRAP3(geo_attract_course_view_bind, sr_course_view_bind(a0, a1, a2))
SR_DIFF_WRAP3(geo_attract_course_prg_mode, sr_course_prg_mode(a0, a1))
SR_DIFF_WRAP3(geo_attract_copro_marker_burst, sr_course_marker_burst())

/* catalog_span reads its budget and flag from g3 / g4. */
void lifted_geo_attract_course_catalog_span(u32 a0, u32 a1, u32 a2);
void geo_attract_course_catalog_span(u32 a0, u32 a1, u32 a2) {
    const u32 budget = (u32)g3, flag = (u32)g4;
    SR_DIFF_BEGIN();
    lifted_geo_attract_course_catalog_span(a0, a1, a2);
    SR_DIFF_NATIVE();
    sr_course_catalog_span(a0, a1, a2, budget, flag);
    SR_DIFF_END("geo_attract_course_catalog_span");
}

/* span_table_draw reads the list range from g3 / g4. */
void lifted_geo_attract_span_table_draw(u32 a0, u32 a1, u32 a2);
void geo_attract_span_table_draw(u32 a0, u32 a1, u32 a2) {
    const u32 start = (u32)g3, ix = (u32)g4;
    SR_DIFF_BEGIN();
    lifted_geo_attract_span_table_draw(a0, a1, a2);
    SR_DIFF_NATIVE();
    sr_course_span_table(a0, a1, a2, start, ix);
    SR_DIFF_END("geo_attract_span_table_draw");
}
