/* CGM catalog staging + format compile (interim semantic port).
 * Uplift pending: libc_printf_dispatch @ 0x05CF50 (see out/lift/libc_printf_dispatch.raw.c),
 * cgm_header_match/cgm_catalog_stage @ 0x05CE18 (libc_scanf_setup uplift base). */
// @rom 0x5ce18 cgm_catalog_stage

#include "i960_lift.h"
#include "i960_mem.h"
#include "cgm_format.h"
#include "cgm_merge.h"
#include "lift_syms.h"
#include "model2_rom.h"
#include "model2_memory.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define CGM_STAGE_BASE  0x005CA000u
#define CGM_STAGE_DATA  0x005CA010u
#define CGM_STAGE_SIZE  0x00002000u

#define CGM_HANDLER_NOP     0x005FC9B4u
#define CGM_HANDLER_HASH    0x005FC1C8u
#define CGM_HANDLER_D       0x005FC3DCu
#define CGM_HANDLER_U       0x005FC714u
#define CGM_HANDLER_E       0x005FC448u
#define CGM_HANDLER_L       0x005FC348u
#define CGM_HANDLER_PCT     0x005FC3C8u
#define CGM_SLOT_CURSOR     0x20C950u
#define CGM_G13_MASK        0x20C958u

static u32 format_handler_rom(u8 code)
{
    const u8 *rom;

    if (code > 0x78u)
        return 0x0005D9B4u;
    rom = model2_maincpu_rom + CGM_FORMAT_TABLE_ROM + ((u32)code << 2);
    return (u32)rom[0] | ((u32)rom[1] << 8) | ((u32)rom[2] << 16) | ((u32)rom[3] << 24);
}

static u8 script_byte(u32 vaddr)
{
    const u8 *p;

    if (vaddr >= 0x005ca000u && vaddr < 0x005cc000u)
        return (u8)i960_ld_u8(I960_WORKRAM, vaddr, 0);
    p = model2_rom_at(vaddr);
    if (p)
        return *p;
    return model2_workram_mirror_u8(vaddr);
}

static u16 script_u16(u32 vaddr)
{
    return (u16)((u16)script_byte(vaddr) | ((u16)script_byte(vaddr + 1u) << 8));
}

void cgm_workram_runtime_init(void)
{
    static const u8 cgm_hdr[8] = { 'C', 'G', 'M', ' ', '1', '.', '0', ' ' };
    u32 off;

    for (off = 0; off < 8u; off++)
        i960_st_u8(I960_WORKRAM, CGM_HEADER_TEMPLATE, off, cgm_hdr[off]);

    /* Jump table @ 0x005FBFD0 mirrors ROM @ 0x005CFD0 (ld 0x5fbfd0[g4*4] in printf). */
    for (off = 0; off < 0x334u; off++) {
        u8 b = model2_maincpu_rom[CGM_FORMAT_TABLE_ROM + off];
        i960_st_u8(I960_WORKRAM, 0x005fbfd0u, off, b);
    }
    cgm_record_dispatch_reset();
}

int cgm_header_match(u32 catalog_vaddr)
{
    u8 tmpl[8];
    u32 idx;
    const u8 *cat;
    void *cmp;

    cat = model2_rom_at(catalog_vaddr);
    if (!cat)
        return -1;
    for (idx = 0; idx < 8u; idx++)
        tmpl[idx] = model2_workram_mirror_u8(CGM_HEADER_TEMPLATE + idx);

    /* @0x05CE18 via catalog_draw_setup @0x29EDC: 8-byte prefix compare. */
    g0 = (uintptr_t)cat;
    g1 = (uintptr_t)tmpl;
    g2 = 8;
    cmp = libc_scanf_setup(cat, tmpl, 8u);
    return (int)(intptr_t)cmp;
}

u32 cgm_catalog_stage(u32 catalog_vaddr)
{
    u32 copy_len;
    u32 off;
    const u8 *src;

    copy_len = CGM_STAGE_SIZE - (CGM_STAGE_DATA - CGM_STAGE_BASE);
    src = model2_rom_at(catalog_vaddr);
    if (!src)
        return catalog_vaddr;

    for (off = 0; off < copy_len; off++)
        i960_st_u8(I960_WORKRAM, CGM_STAGE_DATA, off, src[off]);

    /* @0x29EF0/0x29F00: inner cursor at staged copy base; stream_prime skips 8-byte header. */
    i960_st_u32(I960_WORKRAM, CGM_STAGE_BASE, 0, CGM_STAGE_DATA);
    i960_st_u32(I960_WORKRAM, CGM_STAGE_BASE, 4, 0);
    return CGM_STAGE_BASE;
}

static void cgm_format_state_init(cgm_format_state_t *st, u32 catalog_ctrl)
{
    u32 cursor;

    st->catalog_base = catalog_ctrl;
    cursor = i960_ld_u32(I960_WORKRAM, catalog_ctrl, 0);
    if (cursor == 0)
        cursor = CGM_STAGE_DATA;
    st->stream_cursor = cursor;
}

static u8 cgm_stream_peek(cgm_format_state_t *st)
{
    return script_byte(st->stream_cursor);
}

static void cgm_stream_advance(cgm_format_state_t *st)
{
    st->stream_cursor++;
    i960_st_u32(I960_WORKRAM, st->catalog_base, 0, st->stream_cursor);
}

static void cgm_batch_seed_from_cursor(void)
{
    u32 cursor;
    u32 anchor;

    /* @0x2A29C–0x2A2A8: 0x20C958 = 0x20C950 << 7; @0x2A2B0: cursor += 24. */
    cursor = i960_ld_u32(I960_WORKRAM, CGM_SLOT_CURSOR, 0);
    anchor = (cursor & 0x3ffu) << 7;
    i960_st_u32(I960_WORKRAM, CGM_G13_MASK, 0, anchor);
    cursor += 24u;
    if (cursor > 0x7fu)
        cursor = 0;
    i960_st_u32(I960_WORKRAM, CGM_SLOT_CURSOR, 0, cursor);
}

static u32 cgm_tile_map_base(void)
{
    u32 flags;

    /* @0x29C74 bbc 0,g4 → layer @ 0x01004000 when flags bit 0 set. */
    flags = (u32)g4;
    if ((flags & 1u) != 0)
        return 0x01004000u;
    return 0x01000000u;
}

static u32 s_cgm_tile_place_count;

typedef enum {
    CGM_FIFO_D = 0,
    CGM_FIFO_U = 1,
} cgm_fifo_kind_t;

typedef struct {
    u8 kind;
    u8 count;
} cgm_fifo_op_t;

#define CGM_FIFO_MAX  512u

static cgm_fifo_op_t s_fifo[CGM_FIFO_MAX];
static u32 s_fifo_count;
static u32 s_fifo_stream;
static u32 s_fifo_catalog;
static u32 s_r9_format_flags;
static u32 s_r14_repeat; /* @0x05CFA8: mov 0,r14 — digit repeat; not 0x20B1A8 column cursor */
static u32 s_binary_fifo_base; /* binary chunk before each format fragment (@ 0x1111 interleave) */
static i32 s_printf_int_arg; /* @0x05D42C ld arg — value for 0x05D7E8 remo/divo */

/*
 * Post-compile u16 atom.
 * U (@ 0x29CFC): ADD g13 → tile-map place.
 * D: raw → ``cgm_d_merge_commit`` (@ 0x2A4E0 merge + scratch). Wrapper
 * ``0x02A6D0`` still has no static callers — this is the disasm-proven merge
 * edge with default ``0x02A2E0`` bounds, not ``xor_table``.
 */
static void cgm_thunk_place_u16(u32 *cursor, int add_g13)
{
    u32 g13;
    u16 raw;
    u16 entry;
    u32 map_row;
    u32 map_col;
    u32 map_index;
    u32 tile_base;

    raw = script_u16(*cursor);
    *cursor += 2u;
    g13 = i960_ld_u32(I960_WORKRAM, CGM_G13_MASK, 0);
    if (!add_g13) {
        (void)cgm_d_merge_commit(raw);
        return;
    }
    /* @0x29CFC U / splash ADD path. */
    entry = (u16)((raw + (u16)g13) & 0xffffu);
    if ((entry & 0x8000u) == 0)
        entry |= 0x8000u;
    map_row = i960_ld_u32(I960_WORKRAM, 0x20b1ac, 0);
    map_col = i960_ld_u32(I960_WORKRAM, 0x20b1a8, 0);
    /* @0x2710C/0x27118: col stops at 60; @0x2707C op 10 starts next row. */
    if (map_col > 60u) {
        boot_tile_opcode_dispatch(10u);
        map_row = i960_ld_u32(I960_WORKRAM, 0x20b1ac, 0);
        map_col = i960_ld_u32(I960_WORKRAM, 0x20b1a8, 0);
    }
    tile_base = cgm_tile_map_base();
    if (map_row <= 63u && map_col <= 63u) {
        map_index = (map_row << 6) + map_col;
        i960_st_u16(I960_ABS, tile_base, map_index * 2u, entry);
        s_cgm_tile_place_count++;
        if (map_col <= 60u)
            i960_st_u32(I960_WORKRAM, 0x20b1a8, 0, map_col + 1u);
    }
}

static void cgm_fifo_record(u8 kind, u32 repeat)
{
    u32 count;
    u32 n;

    count = repeat;
    if (count == 0)
        count = 1;
    for (n = 0; n < count && s_fifo_count < CGM_FIFO_MAX; n++) {
        s_fifo[s_fifo_count].kind = kind;
        s_fifo[s_fifo_count].count = 1;
        s_fifo_count++;
    }
}

void cgm_fifo_drain(void)
{
    u32 idx;
    u32 n;
    u32 cursor;

    if (s_fifo_count == 0 || s_fifo_catalog == 0)
        return;

    cursor = s_fifo_stream;
    for (idx = 0; idx < s_fifo_count; idx++) {
        for (n = 0; n < s_fifo[idx].count; n++)
            cgm_thunk_place_u16(&cursor, s_fifo[idx].kind == CGM_FIFO_U);
    }
    i960_st_u32(I960_WORKRAM, s_fifo_catalog, 0, cursor);
    if (getenv("I960_TRACE_SPLASH")) {
        fprintf(stderr, "lift: cgm fifo drain ops=%u cursor+%u\n",
                s_fifo_count, cursor - s_fifo_stream);
    }
    s_fifo_count = 0;
    s_fifo_stream = 0;
    s_fifo_catalog = 0;
}

/* @0x05D7E8 / @0x05D860: template emit only — no u16 in D/U handlers. */

static void cgm_handler_digit(cgm_format_state_t *st, u8 ch)
{
    u32 digit;

    (void)st;
    /* @0x05D304: setbit 5,r9; digit value accumulates in r14 (not 0x20B1A8). */
    digit = (u32)(ch - (u8)'0');
    if (s_r14_repeat < 1000u)
        s_r14_repeat = s_r14_repeat * 10u + digit;
    else
        s_r14_repeat += digit;
    s_r9_format_flags |= 0x20u;
}

static void cgm_format_fragment_reset(void)
{
    /* @0x05CFA0–0x05CFB0: r9/r14/r15 cleared at each format-spec entry. */
    s_r9_format_flags = 0;
    s_r14_repeat = 0;
}

static void cgm_ctrl_cursor_advance(u32 catalog_ctrl)
{
    u32 arena_base;
    u32 node;
    u32 limit_small;
    u32 limit_large;
    u32 next;

    if (catalog_ctrl == 0)
        return;

    /* @0x05D3E0 / @0x05D714: advance linked cell at catalog_ctrl+4. */
    arena_base = i960_ld_u32(I960_WORKRAM, catalog_ctrl, 0);
    node = i960_ld_u32(I960_WORKRAM, catalog_ctrl, 4);
    limit_small = 31u + 17u;
    limit_large = 31u + 21u;

    if (node > limit_small) {
        next = (node + 3u) & ~3u;
        if (arena_base != 0 && next + 4u < CGM_STAGE_BASE + CGM_STAGE_SIZE)
            next = i960_ld_u32(I960_WORKRAM, arena_base, next + 4u);
        if (next <= limit_small)
            next = limit_large;
    } else {
        next = node + 3u;
        if (arena_base != 0 && next + 4u < CGM_STAGE_BASE + CGM_STAGE_SIZE) {
            next = i960_ld_u32(I960_WORKRAM, arena_base, next);
            next &= ~3u;
            next = i960_ld_u32(I960_WORKRAM, arena_base, next + 4u);
        } else {
            next = (node + 7u) & ~7u;
        }
    }
    i960_st_u32(I960_WORKRAM, catalog_ctrl, 4, next);
}

/* @0x05D42C: ``ld`` 32-bit arg from va_list cell (not halfword atom). */
static i32 cgm_ctrl_format_i32(u32 catalog_ctrl)
{
    u32 arena_base;
    u32 node;
    u32 ea;

    arena_base = i960_ld_u32(I960_WORKRAM, catalog_ctrl, 0);
    node = i960_ld_u32(I960_WORKRAM, catalog_ctrl, 4);
    if (arena_base == 0 || node < 4u)
        return 0;
    ea = arena_base + node - 4u;
    if (ea >= WORKRAM_BASE && ea < WORKRAM_BASE + WORKRAM_SIZE)
        return (i32)i960_ld_u32(I960_WORKRAM, ea, 0);
    if (ea >= CGM_STAGE_DATA && ea < CGM_STAGE_DATA + CGM_STAGE_SIZE)
        return (i32)i960_ld_u32(I960_WORKRAM, ea, 0);
    {
        const u8 *p = model2_rom_at(ea);

        if (p)
            return (i32)((u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16)
                         | ((u32)p[3] << 24));
    }
    if (arena_base > 0x01000000u)
        return *(const i32 *)(uintptr_t)ea;
    return (i32)(int16_t)script_u16(ea);
}

static void cgm_handler_d_template(cgm_format_state_t *st, u32 tpl_vaddr, u32 g6)
{
    u32 repeat;
    u32 emit_val;

    (void)st;
    /* @0x05D3DC→0x05D7E8: decimal emit; FIFO u16×repeat drained after spec. */
    repeat = s_r14_repeat;
    emit_val = (u32)(s_printf_int_arg < 0 ? -s_printf_int_arg : s_printf_int_arg);
    if (emit_val == 0 && repeat != 0)
        emit_val = repeat;
    else if (emit_val == 0)
        emit_val = 1;
    /* Queue lone-D atoms for ``cgm_d_merge_commit`` @ drain (not xor_table). */
    cgm_fifo_record(CGM_FIFO_D, repeat);
    cgm_template_emit(tpl_vaddr, g6, s_r9_format_flags | 0x01u, emit_val);
    s_r14_repeat = 0;
}

static void cgm_handler_u_template(cgm_format_state_t *st, u32 tpl_vaddr, u32 g6)
{
    u32 repeat;
    u32 emit_val;

    (void)st;
    /* @0x05D714→0x05D7E4: emit; ADD @ 0x29CFC applied in drain place. */
    repeat = s_r14_repeat;
    emit_val = (u32)(s_printf_int_arg < 0 ? -s_printf_int_arg : s_printf_int_arg);
    if (emit_val == 0 && repeat != 0)
        emit_val = repeat;
    else if (emit_val == 0)
        emit_val = 1;
    cgm_fifo_record(CGM_FIFO_U, repeat);
    cgm_template_emit(tpl_vaddr, g6, s_r9_format_flags, emit_val);
    s_r14_repeat = 0;
}

static void cgm_handler_d_u16(cgm_format_state_t *st, u32 tpl_vaddr, u32 width)
{
    cgm_handler_d_template(st, tpl_vaddr, width);
}

static void cgm_handler_u_u16(cgm_format_state_t *st, u32 tpl_vaddr, u32 width)
{
    cgm_handler_u_template(st, tpl_vaddr, width);
}

static void cgm_handler_e_u32(cgm_format_state_t *st, u32 tpl_vaddr, u32 width)
{
    (void)st;
    (void)tpl_vaddr;
    (void)width;
    /* E reads u32 stream atoms in thunk path — not lifted here. */
}

static void cgm_dispatch_handler(cgm_format_state_t *st, u8 ch)
{
    u32 handler;

    handler = format_handler_rom(ch);
    switch (handler) {
    case 0x005FC30Cu: /* digits @ 0x05D304 */
        cgm_handler_digit(st, ch);
        return;
    case CGM_HANDLER_HASH: /* # @ 0x05D1C8 → 0x2A290 batch seed */
        s_r9_format_flags |= 0x08u;
        cgm_batch_seed_from_cursor();
        return;
    case CGM_HANDLER_D: /* D @ 0x05D3DC */
        cgm_handler_d_u16(st, 0x005FBF10u, 10u);
        return;
    case CGM_HANDLER_U: /* U @ 0x05D714 — ADD g13 @ 0x29CFC */
        cgm_handler_u_u16(st, 0x005FBF10u, 10u);
        return;
    case CGM_HANDLER_E: /* E @ 0x05D348 — u32 stream atom */
        cgm_handler_e_u32(st, 0x005FBF10u, 10u);
        return;
    case CGM_HANDLER_PCT: /* % @ 0x05D3C8 */
        cgm_template_emit(0x005FBF22u, 1u, s_r9_format_flags, s_r14_repeat);
        return;
    case CGM_HANDLER_NOP: /* T + default @ 0x05D9B4 */
        return;
    default:
        return;
    }
}

static int cgm_format_is_spec_letter(u8 ch)
{
    if (ch >= (u8)'0' && ch <= (u8)'9')
        return 1;
    if (format_handler_rom(ch) != CGM_HANDLER_NOP)
        return 1;
    return 0;
}

static void cgm_skip_span_fillers(cgm_format_state_t *st)
{
    while (cgm_stream_peek(st) == (u8)0x11u)
        cgm_stream_advance(st);
}

static int cgm_is_format_entry(cgm_format_state_t *st)
{
    u8 ch;
    u8 spec;

    ch = cgm_stream_peek(st);
    if (ch < (u8)'0' || ch > (u8)'9')
        return 0;
    if (script_byte(st->stream_cursor + 1u) != (u8)'#')
        return 0;
    spec = script_byte(st->stream_cursor + 2u);
    if (spec == 0x11u || spec == (u8)'#')
        return 0;
    return (spec >= (u8)' ' && spec <= (u8)'~') ? 1 : 0;
}

static void cgm_format_run_spec_stream(cgm_format_state_t *st)
{
    u32 spec_end;

    while (cgm_format_is_spec_letter(cgm_stream_peek(st))) {
        u8 letter = cgm_stream_peek(st);

        cgm_dispatch_handler(st, letter);
        cgm_stream_advance(st);
    }
    cgm_skip_span_fillers(st);
    spec_end = st->stream_cursor;
    s_fifo_stream = s_binary_fifo_base;
    s_fifo_catalog = st->catalog_base;
    cgm_fifo_drain();
    s_binary_fifo_base = spec_end;
    st->stream_cursor = spec_end;
    i960_st_u32(I960_WORKRAM, st->catalog_base, 0, spec_end);
}

static void cgm_format_consume_ascii_spec(cgm_format_state_t *st)
{
    cgm_format_run_spec_stream(st);
}

static int cgm_is_ascii_format_marker(u32 data_base, u32 off)
{
    u8 b2;

    if (script_byte(data_base + off) != (u8)'3' ||
        script_byte(data_base + off + 1u) != (u8)'#')
        return 0;
    b2 = script_byte(data_base + off + 2u);
    if (b2 == 0x11u)
        return 0;
    return (b2 >= (u8)' ' && b2 <= (u8)'~') ? 1 : 0;
}

static int cgm_is_hash_format_marker(u32 data_base, u32 off)
{
    u8 spec;

    if (script_byte(data_base + off) != (u8)'#')
        return 0;
    spec = script_byte(data_base + off + 1u);
    if (spec == 0x11u)
        return 0;
    return cgm_format_is_spec_letter(spec) ? 1 : 0;
}

static u32 cgm_find_format_cursor(u32 data_base, u32 scan_start, u32 scan_limit)
{
    u32 off;

    for (off = scan_start; off + 2u < scan_limit; off++) {
        if (cgm_is_ascii_format_marker(data_base, off))
            return data_base + off;
        if (cgm_is_hash_format_marker(data_base, off))
            return data_base + off;
    }
    return 0;
}

static void cgm_format_compile_catalog(cgm_format_state_t *st)
{
    u8 ch;
    u32 guard = 0;

    if (!st || st->catalog_base == 0)
        return;

    if (s_binary_fifo_base == 0)
        s_binary_fifo_base = st->stream_cursor;

    while ((ch = cgm_stream_peek(st)) != 0 && guard < 8192u) {
        guard++;
        if (cgm_is_format_entry(st)) {
            if (getenv("I960_TRACE_SPLASH")) {
                u8 digit = cgm_stream_peek(st);

                fprintf(stderr,
                        "lift: cgm %c# fragment staged+%u bin_base+%u spec=%#x\n",
                        (char)digit,
                        st->stream_cursor - CGM_STAGE_DATA,
                        s_binary_fifo_base - CGM_STAGE_DATA,
                        (unsigned)script_byte(st->stream_cursor + 2u));
            }

            cgm_format_fragment_reset();
            cgm_handler_digit(st, ch);
            cgm_stream_advance(st);
            cgm_stream_advance(st);
            s_r9_format_flags |= 0x08u;
            cgm_batch_seed_from_cursor();
            if (script_byte(st->stream_cursor) == (u8)0x11u)
                cgm_skip_span_fillers(st);
            else
                cgm_format_consume_ascii_spec(st);
            continue;
        }
        if (ch == (u8)0x11u) {
            cgm_stream_advance(st);
            continue;
        }
        if (ch == 0) {
            cgm_stream_advance(st);
            continue;
        }
        if (ch == (u8)'"') {
            cgm_stream_advance(st);
            continue;
        }
        /* Binary FIFO chunk — not 0x027008 opcode dispatch (@ 0x05CF50 path). */
        cgm_stream_advance(st);
    }
}

static void cgm_format_compile_all_fragments(cgm_format_state_t *st)
{
    if (!st || st->catalog_base == 0)
        return;

    s_binary_fifo_base = st->stream_cursor;
    cgm_format_compile_catalog(st);
}

void cgm_catalog_seek_format(u32 catalog_ctrl)
{
    u32 cursor;
    u32 rel;
    u32 format_cursor;

    if (catalog_ctrl == 0)
        return;

    cursor = i960_ld_u32(I960_WORKRAM, catalog_ctrl, 0);
    if (cursor < CGM_STAGE_DATA)
        cursor = CGM_STAGE_DATA;
    rel = cursor - CGM_STAGE_DATA;

    format_cursor = cgm_find_format_cursor(CGM_STAGE_DATA, rel, 0x2000u);
    if (format_cursor == 0 && rel > 0u)
        format_cursor = cgm_find_format_cursor(CGM_STAGE_DATA, 0u, rel);
    if (format_cursor == 0)
        format_cursor = cgm_find_format_cursor(CGM_STAGE_DATA, 10u, 0x2000u);
    if (format_cursor != 0)
        i960_st_u32(I960_WORKRAM, catalog_ctrl, 0, format_cursor);
}

void cgm_format_compile_ctrl(u32 catalog_ctrl)
{
    cgm_format_state_t st;

    if (catalog_ctrl == 0)
        return;
    s_cgm_tile_place_count = 0;
    s_fifo_count = 0;
    s_fifo_stream = 0;
    s_fifo_catalog = 0;
    s_r9_format_flags = 0;
    s_r14_repeat = 0;
    s_binary_fifo_base = 0;
    cgm_format_state_init(&st, catalog_ctrl);
    cgm_format_compile_all_fragments(&st);
    if (getenv("I960_TRACE_SPLASH")) {
        fprintf(stderr, "lift: cgm_format_compile placed %u L2 tiles\n",
                s_cgm_tile_place_count);
    }
}

void cgm_printf_spec_open(u32 catalog_ctrl)
{
    if (catalog_ctrl == 0)
        return;
    s_fifo_catalog = catalog_ctrl;
    s_fifo_stream = i960_ld_u32(I960_WORKRAM, catalog_ctrl, 0);
    s_fifo_count = 0;
    cgm_format_fragment_reset();
}

void cgm_printf_spec_close(void)
{
    if (s_fifo_catalog != 0)
        s_fifo_stream = i960_ld_u32(I960_WORKRAM, s_fifo_catalog, 0);
    cgm_fifo_drain();
}

void cgm_printf_hash(void)
{
    r9 = (u32)r9 | 0x08u;
    s_r9_format_flags |= 0x08u;
    cgm_batch_seed_from_cursor();
}

void cgm_printf_digit(u8 digit_ch)
{
    u32 digit;

    if (digit_ch < (u8)'0' || digit_ch > (u8)'9')
        return;
    digit = (u32)(digit_ch - (u8)'0');
    if (s_r14_repeat < 1000u)
        s_r14_repeat = s_r14_repeat * 10u + digit;
    else
        s_r14_repeat += digit;
    r9 = (u32)r9 | 0x20u;
    s_r9_format_flags |= 0x20u;
}

void cgm_printf_d(void)
{
    u32 ctrl = (u32)r5;

    /* @0x05D3DC: setbit 0,r9; load signed i32; always ``b 0x5d7e8`` (g6=10). */
    cgm_ctrl_cursor_advance(ctrl);
    r9 = (u32)r9 | 0x01u;
    s_r9_format_flags |= 0x01u;
    s_printf_int_arg = cgm_ctrl_format_i32(ctrl);
    if (s_printf_int_arg < 0)
        r13 = 1;
    cgm_handler_d_template(0, 0x005FBF10u, 10u);
    r14 = s_r14_repeat;
    s_r14_repeat = 0;
}

void cgm_printf_u(void)
{
    u32 ctrl = (u32)r5;

    /* @0x05D714: setbit 0,r9; load; ``b 0x5d7e4`` → decimal emit (g6=10). */
    cgm_ctrl_cursor_advance(ctrl);
    r9 = (u32)r9 | 0x01u;
    s_r9_format_flags |= 0x01u;
    s_printf_int_arg = cgm_ctrl_format_i32(ctrl);
    if ((r9 & 0x08u) != 0) {
        if ((r9 & 0x20u) != 0 && s_r14_repeat != 0)
            s_r9_format_flags |= 0x40u;
        else if (s_printf_int_arg != 0)
            s_r9_format_flags |= 0x40u;
    }
    if ((r9 & 0x08u) != 0 && s_r14_repeat != 0)
        r9 = (u32)r9 | 0x40u;

    cgm_handler_u_template(0, 0x005FBF10u, 10u);
    r14 = s_r14_repeat;
    s_r14_repeat = 0;
}
