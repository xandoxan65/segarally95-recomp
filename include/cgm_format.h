/* CGM format compiler (@ 0x05CF50) and catalog staging for draw_scene @ 0x29C10. */
#ifndef CGM_FORMAT_H
#define CGM_FORMAT_H

#include "i960_lift.h"

#define CGM_FORMAT_TABLE_ROM  0x0005CFD0u
#define CGM_HEADER_TEMPLATE   0x005C8E60u
#define CGM_HEADER_BYTES      8u
#define CGM_STAGE_BASE        0x005CA000u
#define CGM_STAGE_DATA        0x005CA010u
#define CGM_SLOT_CURSOR       0x0020C950u
#define CGM_G13_MASK          0x0020C958u

typedef struct {
    u32 catalog_base;   /* outer node / catalog ctrl (r5 in handlers) */
    u32 stream_cursor;  /* inner cursor; mirrored at *(catalog_base) */
} cgm_format_state_t;

u32 cgm_catalog_stage(u32 catalog_vaddr);
int cgm_header_match(u32 catalog_vaddr);
void cgm_catalog_seek_format(u32 catalog_ctrl);
void cgm_format_compile_ctrl(u32 catalog_ctrl);
u32 cgm_dispatch_record_build(u32 handler_rom);
void cgm_record_dispatch(u32 arg0, u32 arg1);
void cgm_record_dispatch_reset(void);
u32 cgm_thunk_walk(u32 arg0, u32 arg1);
void cgm_template_emit(u32 tpl_vaddr, u32 g6, u32 r9_flags, u32 r14_repeat);
void cgm_workram_runtime_init(void);

/* libc_printf @ 0x05CF50 handler surface (shared with cgm_format_compile). */
void cgm_printf_spec_open(u32 catalog_ctrl);
void cgm_printf_spec_close(void);
void cgm_printf_hash(void);
void cgm_printf_digit(u8 digit_ch);
void cgm_printf_d(void);
void cgm_printf_u(void);

#endif
