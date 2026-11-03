/* sr_tgp_cmd forwarded to the host reference's TGP HLE (lib/model2/tgp), for the differential checks. */
#include "sr_tgp.h"
#include "model2_tgp.h"

void sr_tgp_cmd(uint32_t cmd, const uint32_t *in, unsigned n_in, uint32_t *out, unsigned n_out) {
    unsigned i;
    model2_tgp_fifo_write(cmd);
    for(i = 0; i < n_in; i++) model2_tgp_fifo_write(in[i]);
    for(i = 0; i < n_out; i++) out[i] = model2_tgp_fifo_read();
}

void sr_tgp_arm_course_handle(uint32_t handle) { model2_tgp_arm_course_handle(handle); }

void sr_tgp_arm_52_block(const uint32_t words[9]) { model2_tgp_arm_52_block(words); }

void sr_tgp_arm_ext_selector(uint32_t word) { model2_tgp_arm_ext_selector(word); }
