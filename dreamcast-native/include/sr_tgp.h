/* The TGP (MB86234 geometry coprocessor) as ported routines call it: one command word, its input words, then
 * its output words, as the original pushes and pops them on the FIFO at 0x884000. The command word is the full
 * marker the i960 writes (e.g. 0x17802f2f, normalize).
 *
 * Two implementations: code/race/tgp.c, the native one for the Dreamcast and the standalone native host run,
 * and tests/diff/sr_tgp_host.c, which forwards to the host reference's TGP so a ported routine can be checked
 * against its lifted original from the same TGP state. */
#ifndef SR_TGP_H
#define SR_TGP_H
#include <stdint.h>

/* Native build only: reset the TGP and give it the copro data ROM words (course collision data for 0x52). */
void sr_tgp_init(const uint32_t *copro_words, uint32_t n_words);

void sr_tgp_cmd(uint32_t cmd, const uint32_t *in, unsigned n_in, uint32_t *out, unsigned n_out);

/* Road queries (table_index_a, 0x2ABC0): the course row word and the nine-word block the i960 also writes through
 * the 0x20a290 cursor, handed to the TGP directly as the host reference does (model2_tgp_arm_*). */
void sr_tgp_arm_course_handle(uint32_t handle);
void sr_tgp_arm_52_block(const uint32_t words[9]);
/* The 0x60/0x61 slot word a call site stores behind the advanced list cursor (car_motion). */
void sr_tgp_arm_ext_selector(uint32_t word);

#endif
