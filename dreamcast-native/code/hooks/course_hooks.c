/* Course drawing hooks on the Dreamcast (include/race/course_hooks.h): not drawn yet. */
#include "race/course_hooks.h"

void sr_hook_course_bank(uint32_t bank) { (void)bank; }
void sr_hook_course_prg(uint32_t w) { (void)w; }
void sr_hook_course_catalog(uint32_t w0, uint32_t w1, uint32_t w2, uint32_t w3) {
    (void)w0;
    (void)w1;
    (void)w2;
    (void)w3;
}
void sr_hook_course_reg(uint32_t addr, uint32_t value) {
    (void)addr;
    (void)value;
}
/* The GEO write pointer the original stores into the TGP parameter list; no GEO here. */
uint32_t sr_hook_course_write_ptr(void) { return 0; }
