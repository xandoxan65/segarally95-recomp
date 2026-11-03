#include "race/course.h"
#include "sr_mem.h"

int sr_practice_skip;

uint32_t sr_race_course_index(void) {
    return sr_ld32(sr_practice_skip ? 0x20a8c4u : 0x214354u);
}
