/* Lockstep checkpoints (code/race/checkpoint.c): a running hash of the race nodes' words, the same on the host's
 * native run and on the Dreamcast, printed every SR_CHECKPOINT_TICKS race ticks. */
#ifndef SR_RACE_CHECKPOINT_H
#define SR_RACE_CHECKPOINT_H
#include <stdint.h>

/* A checkpoint every SR_CHECKPOINT_TICKS ticks from tick SR_CHECKPOINT_FROM (the Dreamcast keeps 256): make
 * CHECKPOINT_TICKS=1 CHECKPOINT_FROM=N narrows down where two runs part. */
#ifndef SR_CHECKPOINT_TICKS
#define SR_CHECKPOINT_TICKS 60u
#endif
#ifndef SR_CHECKPOINT_FROM
#define SR_CHECKPOINT_FROM 0u
#endif

/* Fold the 0x500000..0x5002ff node words (the player, the road follower, the desert object) into `h`. */
uint32_t sr_checkpoint_fold(uint32_t h);

#endif
