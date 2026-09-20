#ifndef PLACEMENT_CATALOG_FEED_H
#define PLACEMENT_CATALOG_FEED_H

#include "i960_lift.h"

/* Push one catalog row @ 0x02864B40[index] through geo/copro FIFO (placement feeder prologue). */
void placement_catalog_push(u32 catalog_index);

/* Push catalog row for placement stream instance *pi* (catalog index = placement index, static RE). */
void placement_catalog_push_placement_index(u32 placement_index);

/* Feed track segment [start, start+count) using placement indices as catalog rows. */
void placement_catalog_feed_track_batch(u32 start, u32 count);

/* 0=race draw list, 1=track placement segment (desert @ index 9+). */
int placement_catalog_feed_source(void);

#define PLACEMENT_TRACK_DESERT_START 9u
#define PLACEMENT_TRACK_DESERT_COUNT 101u

void placement_catalog_push_race_slot(u32 slot);

/* Feed *count* indices from RE-backed table (race draw list @ ROM 0x34E88). */
void placement_catalog_feed_race_batch(u32 start, u32 count);

/* Batch size from I960_HOST_PLACEMENT_BATCH (default 4). */
unsigned placement_catalog_feed_env_batch(void);

#endif
