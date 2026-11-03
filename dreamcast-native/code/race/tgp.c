/* sr_tgp_* on the Dreamcast and in the standalone native run: the host reference's TGP HLE (lib/model2/tgp,
 * built unchanged from code/tgp/), so both sides compute the same bits. RAM reads it makes (the 0x20a290 list
 * cursor, the 0x52 block) go through sr_mem. */
#include "sr_tgp.h"
#include "sr_mem.h"
#include "model2_tgp.h"
#include "model2_tgp_fw.h"

static u32 tgp_workram_ld(u32 vaddr) { return sr_ld32(vaddr); }

void sr_tgp_init(const uint32_t *copro_words, uint32_t n_words) {
    static const model2_tgp_host_ops_t ops = {.workram_ld_u32 = tgp_workram_ld, .emit_geo_matrix = 0};
    model2_tgp_bind_host(&ops);
    model2_tgp_reset();
    model2_tgp_fw_bind_copro_data((const u32 *)copro_words, n_words); /* same width, u32 is unsigned int */
}

void sr_tgp_cmd(uint32_t cmd, const uint32_t *in, unsigned n_in, uint32_t *out, unsigned n_out) {
    unsigned i;
    model2_tgp_fifo_write(cmd);
    for(i = 0; i < n_in; i++) model2_tgp_fifo_write(in[i]);
    for(i = 0; i < n_out; i++) out[i] = model2_tgp_fifo_read();
}

void sr_tgp_arm_course_handle(uint32_t handle) { model2_tgp_arm_course_handle(handle); }

void sr_tgp_arm_52_block(const uint32_t words[9]) { model2_tgp_arm_52_block((const u32 *)words); }

void sr_tgp_arm_ext_selector(uint32_t word) { model2_tgp_arm_ext_selector(word); }
