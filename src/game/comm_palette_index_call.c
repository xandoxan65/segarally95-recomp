/* Sound command by table index @ 0x26140 (misnamed palette in the lift). */
// @rom 0x26140 +0x10 comm_palette_index_call

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"
#include "model2_memory.h"
#include "model2_snd.h"

extern void comm_palette_staging_upload(void);

void comm_palette_index_call(u32 slot_index)
{
    u8 b0;
    u8 b1;
    u8 b2;

    /* lda 0x5c3ce0(g0)[g0*2] → pointer to 3-byte packet, then call 0x26090. */
    g0 = SOUND_CMD_TABLE + slot_index + (slot_index * 2u);
    b0 = (u8)i960_ld_u8(I960_ABS, (u32)g0, 0);
    b1 = (u8)i960_ld_u8(I960_ABS, (u32)g0, 1u);
    b2 = (u8)i960_ld_u8(I960_ABS, (u32)g0, 2u);
    model2_snd_log_cmd(slot_index, b0, b1, b2);
    comm_palette_staging_upload();
}
