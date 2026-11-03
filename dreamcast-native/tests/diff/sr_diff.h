/* Differential check of one ported routine against its lifted original, inside the host reference run.
 *
 *   SR_DIFF_BEGIN();                  RAM and TGP state before
 *   lifted_name(args);                the original, as the game runs it
 *   SR_DIFF_NATIVE();                 keep its end state, rewind to the state before
 *   sr_name(args);                    the port, from the same state
 *   SR_DIFF_END("name");              compare the two end states, then continue from the original's
 *
 * SR_DIFF_EXTRA(ptr, len) right after SR_DIFF_BEGIN also compares a host buffer such as the caller's frame.
 *
 * The game always continues from the lifted routine's results, so one wrong port shows up at its own call and
 * does not spread. Mismatches go to stderr (first differing words) and are counted per routine; the summary
 * prints at exit. SR_DIFF_STOP=1 aborts at the first mismatch. */
#ifndef SR_DIFF_H
#define SR_DIFF_H
#include <stdint.h>

typedef struct sr_diff_frame sr_diff_frame_t;

sr_diff_frame_t *sr_diff_begin(void);
/* Also compare a host buffer the routine writes outside RAM (the caller's i960 frame), up to 1 KiB. */
void sr_diff_extra(sr_diff_frame_t *f, void *ptr, unsigned len);
void sr_diff_native(sr_diff_frame_t *f);
/* Compare a returned value as part of the check. */
void sr_diff_ret(sr_diff_frame_t *f, uint32_t lifted, uint32_t native);
void sr_diff_end(sr_diff_frame_t *f, const char *name);

#define SR_DIFF_BEGIN()       sr_diff_frame_t *sr_diff_f_ = sr_diff_begin()
#define SR_DIFF_EXTRA(p, n)   sr_diff_extra(sr_diff_f_, (p), (n))
#define SR_DIFF_NATIVE()      sr_diff_native(sr_diff_f_)
#define SR_DIFF_RET(l, n)     sr_diff_ret(sr_diff_f_, (l), (n))
#define SR_DIFF_END(name)     sr_diff_end(sr_diff_f_, name)

#endif
