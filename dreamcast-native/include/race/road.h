/* Road surface queries and the road cell search (code/race/road_query.c). */
#ifndef SR_RACE_ROAD_H
#define SR_RACE_ROAD_H
#include <stdint.h>

/* 0x2ABC0: start a surface query at xyz around road cell `index` (flags: the original's g3, 0 for the car). */
void sr_road_query_a(uint32_t course, uint32_t index, const uint8_t *xyz, uint32_t flags);
/* 0x2ACC0: read it into out+0x0c..0x1c: normal xyz, height above the surface (or wall distance), flag byte. */
void sr_road_query_b(const uint8_t *xyz, uint8_t *out, uint32_t slot);
/* 0x2AED0: 1 when xyz is inside the quad cell record `rec` over the point table `pts`. */
uint32_t sr_road_cell_probe(uint32_t pts, uint32_t rec, const uint8_t *xyz);
/* 0x2B0B0: find the cell under xyz near the previous one; `index` holds two halfwords (cell, counter). */
int32_t sr_road_cell_pack(const uint8_t *xyz, uint8_t *index);
/* 0x27B28: wrap an angle (float bits) into [-pi, pi]. */
uint32_t sr_angle_wrap(uint32_t bits);

#endif
