/* Course selection as the race code reads it. */
#ifndef SR_RACE_COURSE_H
#define SR_RACE_COURSE_H
#include <stdint.h>

/* Set when the race was entered straight into desert practice without the select screens (the host reference's
 * --practice, and the native build until its front end exists): the course is then 0x20a8c4, because 0x214354
 * still holds the car class. Otherwise the course is 0x214354, as the original reads it. */
extern int sr_practice_skip;

uint32_t sr_race_course_index(void);

#endif
