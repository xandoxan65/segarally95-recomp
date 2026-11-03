/* Desert helicopter drawing hooks on the Dreamcast: nothing yet (include/race/desert_hooks.h). The list slots
 * then hold 0 instead of the GEO write pointer, which only the Model 2 renderer reads. */
#include "race/desert_hooks.h"

uint32_t sr_hook_geo_list_ptr(void) { return 0; }
void sr_hook_geo_object_start(uint32_t list_ptr) { (void)list_ptr; }
void sr_hook_geo_bank_word(uint32_t word) { (void)word; }
void sr_hook_geo_catalog(uint32_t catalog) { (void)catalog; }
void sr_hook_pen_push(uint32_t obj, uint32_t angles, uint32_t eye, uint32_t pen_mode) {
    (void)obj;
    (void)angles;
    (void)eye;
    (void)pen_mode;
}
