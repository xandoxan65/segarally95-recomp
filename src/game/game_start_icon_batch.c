/* Host latch for championship/practice icon batch (not a ROM workram cell). */
#include "game_start_icon_batch.h"

static u32 s_icon_batch;

void game_start_icon_batch_set(u32 batch)
{
    s_icon_batch = batch;
}

u32 game_start_icon_batch_get(void)
{
    return s_icon_batch;
}
