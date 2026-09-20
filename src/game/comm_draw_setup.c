/* Sound command enqueue @ 0x26150 / 0x26090 / 0x261B0.
 * Table 0x5C3CE0 is 3-byte MIDI packets (misnamed "palette" in the lift). */
// @rom 0x26150 +0xb0 comm_draw_setup

#include "i960_lift.h"
#include "i960_mem.h"
#include "lift_syms.h"
#include "model2_memory.h"
#include "model2_rom.h"
#include "model2_snd.h"

#include <stdio.h>

#define SOUND_CMD_SCRIPT 0x005C5070u

void comm_palette_staging_upload(void)
{
    u32 pending;
    u32 src;
    u32 cursor;
    u32 step;

    /* @0x26090: 0x20B180 is pending byte count; enqueue if pending+3 <= 0x80. */
    pending = i960_ld_u32(I960_WORKRAM, 0x20b180, 0);
    if (pending + 3u > 0x80u) {
        /* @0x260A4–0x260C0: ring full — tile script fallback, not UART. */
        if (model2_snd_log_enabled())
            fprintf(stderr, "lift: sound ring full pending=%u — skip UART\n",
                    (unsigned)pending);
        scene_view_scale(8, 0, 0);
        tile_cursor_seed(10, 10);
        boot_tile_script_run(SOUND_CMD_SCRIPT);
        return;
    }

    src = (u32)g0;
    cursor = i960_ld_u32(I960_WORKRAM, 0x20b184, 0);
    i960_st_u32(I960_WORKRAM, 0x20b188, 0, 1u);

    for (step = 0; step <= 2u; step++) {
        u8 byte = (u8)i960_ld_u8(I960_ABS, src, step);

        i960_st_u8(I960_WORKRAM, 0x20b100, cursor, byte);
        cursor = (cursor + 1u) & 0x7fu;
    }

    i960_st_u32(I960_WORKRAM, 0x20b184, 0, cursor);
    i960_st_u32(I960_WORKRAM, 0x20b188, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x20b180, 0, pending + 3u);
    g0 = 1;
}

static void sound_cmd_slot_bytes(u32 slot_index, u32 fill_byte)
{
    u32 desc_ptr;
    u8 b0;
    u8 b1;
    u8 b2;

    desc_ptr = SOUND_CMD_TABLE + slot_index + (slot_index * 2u);
    b0 = (u8)i960_ld_u8(I960_ABS, desc_ptr, 0);
    b1 = (u8)i960_ld_u8(I960_ABS, desc_ptr, 1u);
    b2 = (u8)i960_ld_u8(I960_ABS, desc_ptr, 2u);

    i960_st_u8(I960_WORKRAM, 0x20b18c, 0, b0);
    i960_st_u8(I960_WORKRAM, 0x20b18d, 0, b1);
    if (b2 == 0xffu)
        i960_st_u8(I960_WORKRAM, 0x20b18e, 0, (u8)fill_byte);
    else
        i960_st_u8(I960_WORKRAM, 0x20b18e, 0, b2);
    model2_snd_log_cmd(slot_index,
                       b0,
                       b1,
                       b2 == 0xffu ? (u8)fill_byte : b2);
}

void tile_texture_descriptor_apply(u32 slot_index, u32 fill_byte, u32 arg2)
{
    (void)arg2;

    /* @0x261B0: table entry → staging @ 0x20B18C (0xFF fill), then @0x26090. */
    sound_cmd_slot_bytes(slot_index, fill_byte);
    g0 = 0x20b18cu;
    comm_palette_staging_upload();
}

void comm_palette_slot_upload(u32 slot_index, u32 fill_byte)
{
    sound_cmd_slot_bytes(slot_index, fill_byte);
    g0 = 0x20b18cu;
    comm_palette_staging_upload();
}

void comm_draw_setup(u32 arg0, u32 arg1, u32 arg2)
{
    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x26150: six slots via 0x261B0, then index 0x46 via 0x26140. */
    comm_palette_slot_upload(0x9au, 0);
    comm_palette_slot_upload(0xadu, 0);
    comm_palette_slot_upload(0xb5u, 0);
    comm_palette_slot_upload(0xb8u, 0);
    comm_palette_slot_upload(0xbau, 0);
    comm_palette_slot_upload(0x97u, 0);
    comm_palette_slot_upload(0x46u, 0);
    i960_st_u32(I960_WORKRAM, 0x20b190, 0, (u32)g14);
}
