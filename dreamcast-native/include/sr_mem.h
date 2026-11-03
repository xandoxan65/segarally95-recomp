/* The game's memory as the native port sees it: the arcade's RAM blocks at their i960 addresses, so ported
 * routines keep the original's data layout and their state compares word for word with the host reference.
 *
 *   0x000000  program ROM (1 MiB, read-only)
 *   0x200000  CRX RAM (256 KiB): game globals
 *   0x500000  work RAM (1 MiB): object nodes from 0x500100; from 0x59f000 it mirrors program ROM tables
 *   0x900000  buffer RAM (128 KiB)
 *   0x1a00000 comm board shared RAM (16 KiB, mirrored at +0x10000): the race reads its peer tables in practice
 *   0x1d00000 backup SRAM (16 KiB): records, lap bests
 *   0x2000000 main data ROM (and its 0x6000000 mirror of the upper 16 MiB)
 *
 * The host reference reads a work RAM word above 0x59f000 that is still zero from the ROM mirror instead
 * (lib/model2/host/i960_mem.c); sr_ld* do the same so both sides read the same values.
 * i960 and SH-4 are both little-endian, so words are used as stored. */
#ifndef SR_MEM_H
#define SR_MEM_H
#include <stdint.h>
#include <string.h>

#define SR_ROM_SIZE        0x100000u
#define SR_CRX_BASE        0x200000u
#define SR_CRX_SIZE        0x40000u
#define SR_WORK_BASE       0x500000u
#define SR_WORK_SIZE       0x100000u
#define SR_WORK_ROM_MIRROR 0x59f000u
#define SR_BUFFER_BASE     0x900000u
#define SR_BUFFER_SIZE     0x20000u
#define SR_COMM_BASE       0x1a00000u
#define SR_COMM_SIZE       0x4000u
#define SR_COMM_MIRROR     0x10000u
#define SR_BACKUP_BASE     0x1d00000u
#define SR_BACKUP_SIZE     0x4000u
#define SR_MAIN_DATA_A     0x2000000u
#define SR_MAIN_DATA_B     0x6000000u
#define SR_MAIN_DATA_B_OFF 0x1000000u

/* The fast path's pages: 64 KiB pages below main data. */
#define SR_PAGE_SHIFT 16u
#define SR_PAGES      (SR_MAIN_DATA_A >> SR_PAGE_SHIFT)

typedef struct {
    const uint8_t *rom;
    uint8_t *crx;
    uint8_t *work;
    uint8_t *buffer;
    uint8_t *comm;
    uint8_t *backup;
    const uint8_t *main_data;
    uint32_t main_data_size;
    /* When set, main data is only these 4 KiB pages (a race pack on the Dreamcast); NULL entries read as absent. */
    const uint8_t *const *main_pages;
    /* page[va >> 16]: the host address of va less va, for the pages wholly inside one plain block (no mirror rule,
     * no hole); 0 elsewhere, and everywhere until sr_mem_map: those take sr_at's walk over the blocks. */
    uintptr_t page[SR_PAGES];
} sr_mem_t;

extern sr_mem_t sr_mem;

#ifdef SR_MEM_TRACE
/* Host tool builds only: called with each main data byte offset read (pack sizing, tests/native_run). */
void sr_mem_trace_main(uint32_t off);
#define SR_MEM_TRACE_MAIN(off) sr_mem_trace_main(off)
#else
#define SR_MEM_TRACE_MAIN(off) ((void)0)
#endif

static inline uint8_t *sr_main_at(uint32_t off) {
    SR_MEM_TRACE_MAIN(off);
    if(sr_mem.main_pages) {
        const uint8_t *page = sr_mem.main_pages[off >> 12];
        return page ? (uint8_t *)(uintptr_t)(page + (off & 0xfffu)) : 0;
    }
    return (uint8_t *)(uintptr_t)(sr_mem.main_data + off);
}

/* Host pointer for an i960 address, or NULL if unmapped. */
static inline uint8_t *sr_at(uint32_t va) {
    if(va < SR_ROM_SIZE) return (uint8_t *)(uintptr_t)(sr_mem.rom + va);
    if(va - SR_CRX_BASE < SR_CRX_SIZE) return sr_mem.crx + (va - SR_CRX_BASE);
    if(va - SR_WORK_BASE < SR_WORK_SIZE) return sr_mem.work + (va - SR_WORK_BASE);
    if(va - SR_BUFFER_BASE < SR_BUFFER_SIZE) return sr_mem.buffer + (va - SR_BUFFER_BASE);
    if(va - SR_COMM_BASE < SR_COMM_MIRROR + SR_COMM_SIZE) {
        const uint32_t in_bank = (va - SR_COMM_BASE) % SR_COMM_MIRROR;
        return in_bank < SR_COMM_SIZE ? sr_mem.comm + in_bank : 0;
    }
    if(va - SR_BACKUP_BASE < SR_BACKUP_SIZE) return sr_mem.backup + (va - SR_BACKUP_BASE);
    if(va - SR_MAIN_DATA_A < sr_mem.main_data_size) return sr_main_at(va - SR_MAIN_DATA_A);
    if(va - SR_MAIN_DATA_B < SR_MAIN_DATA_B_OFF && va - SR_MAIN_DATA_B + SR_MAIN_DATA_B_OFF < sr_mem.main_data_size)
        return sr_main_at(va - SR_MAIN_DATA_B + SR_MAIN_DATA_B_OFF);
    return 0;
}

/* Fill sr_mem.page from the blocks: after setting them (sr_at is the reference: a mapped page gives its result). */
static inline void sr_mem_map(void) {
    uint32_t p;
    for(p = 0; p < SR_PAGES; p++) {
        const uint32_t va = p << SR_PAGE_SHIFT, last = va + ((1u << SR_PAGE_SHIFT) - 1u);
        const uint8_t *h = 0;
        if(last < SR_ROM_SIZE) h = sr_mem.rom ? sr_mem.rom + va : 0;
        else if(va >= SR_CRX_BASE && last < SR_CRX_BASE + SR_CRX_SIZE) h = sr_mem.crx ? sr_mem.crx + (va - SR_CRX_BASE) : 0;
        else if(va >= SR_WORK_BASE && last < SR_WORK_ROM_MIRROR) h = sr_mem.work ? sr_mem.work + (va - SR_WORK_BASE) : 0;
        else if(va >= SR_BUFFER_BASE && last < SR_BUFFER_BASE + SR_BUFFER_SIZE)
            h = sr_mem.buffer ? sr_mem.buffer + (va - SR_BUFFER_BASE) : 0;
        sr_mem.page[p] = h ? (uintptr_t)h - va : 0;
    }
}

/* The host address of va from the page table or main data's pages, or NULL: then sr_at's walk decides. */
static inline uint8_t *sr_page_at(uint32_t va) {
    if(va < (SR_PAGES << SR_PAGE_SHIFT)) {
        const uintptr_t b = sr_mem.page[va >> SR_PAGE_SHIFT];
        if(b) return (uint8_t *)(b + va);
        return 0;
    }
    if(va - SR_MAIN_DATA_A < sr_mem.main_data_size) return sr_main_at(va - SR_MAIN_DATA_A);
    return 0;
}

static inline const uint8_t *sr_rom_mirror(uint32_t va) {
    return va - SR_WORK_ROM_MIRROR < SR_ROM_SIZE ? sr_mem.rom + (va - SR_WORK_ROM_MIRROR) : 0;
}

/* Word access to the RAM images. An aligned address is one load or store; GCC turns a 4-byte memcpy from a
 * pointer of unknown alignment into a library call on the SH-4, and these run hundreds of thousands of times a
 * tick. The images are malloc'ed, so the host's alignment matches the original's. */
typedef uint32_t sr_u32_alias __attribute__((may_alias));
typedef uint16_t sr_u16_alias __attribute__((may_alias));

static inline uint32_t sr_get32(const uint8_t *p) {
    if(!((uintptr_t)p & 3u)) return *(const sr_u32_alias *)p;
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static inline uint32_t sr_get16(const uint8_t *p) {
    if(!((uintptr_t)p & 1u)) return *(const sr_u16_alias *)p;
    return (uint32_t)p[0] | (uint32_t)p[1] << 8;
}

static inline void sr_put32(uint8_t *p, uint32_t v) {
    if(!((uintptr_t)p & 3u)) *(sr_u32_alias *)p = v;
    else p[0] = (uint8_t)v, p[1] = (uint8_t)(v >> 8), p[2] = (uint8_t)(v >> 16), p[3] = (uint8_t)(v >> 24);
}

static inline void sr_put16(uint8_t *p, uint32_t v) {
    if(!((uintptr_t)p & 1u)) *(sr_u16_alias *)p = (uint16_t)v;
    else p[0] = (uint8_t)v, p[1] = (uint8_t)(v >> 8);
}

static __attribute__((noinline, unused)) uint32_t sr_ld32_walk(uint32_t va) {
    const uint8_t *p = sr_at(va);
    uint32_t v = p ? sr_get32(p) : 0;
    if(v == 0 && va >= SR_WORK_ROM_MIRROR && va - SR_WORK_BASE < SR_WORK_SIZE) {
        const uint8_t *m = sr_rom_mirror(va);
        if(m) v = sr_get32(m);
    }
    return v;
}

static __attribute__((noinline, unused)) uint32_t sr_ld16_walk(uint32_t va) {
    const uint8_t *p = sr_at(va);
    uint32_t v = p ? sr_get16(p) : 0;
    if(v == 0 && va >= SR_WORK_ROM_MIRROR && va - SR_WORK_BASE < SR_WORK_SIZE) {
        const uint8_t *m = sr_rom_mirror(va);
        if(m) v = sr_get16(m);
    }
    return v;
}

static __attribute__((noinline, unused)) uint32_t sr_ld8_walk(uint32_t va) {
    const uint8_t *p = sr_at(va);
    uint8_t v = p ? *p : 0;
    if(v == 0 && va >= SR_WORK_ROM_MIRROR && va - SR_WORK_BASE < SR_WORK_SIZE) {
        const uint8_t *m = sr_rom_mirror(va);
        if(m) v = *m;
    }
    return v;
}

/* Loads: a mapped page directly (a plain block has no mirror rule), else the walk with its rules. */
static inline uint32_t sr_ld32(uint32_t va) {
    const uint8_t *p = sr_page_at(va);
    return p ? sr_get32(p) : sr_ld32_walk(va);
}

static inline uint32_t sr_ld16(uint32_t va) {
    const uint8_t *p = sr_page_at(va);
    return p ? sr_get16(p) : sr_ld16_walk(va);
}

static inline uint32_t sr_ld8(uint32_t va) {
    const uint8_t *p = sr_page_at(va);
    return p ? *p : sr_ld8_walk(va);
}

/* Stores: nothing below CRX RAM (program ROM) is written. */
static __attribute__((noinline, unused)) void sr_st_walk(uint32_t va, uint32_t v, uint32_t bytes) {
    uint8_t *p = sr_at(va);
    if(!p || va < SR_CRX_BASE) return;
    if(bytes == 4u) sr_put32(p, v);
    else if(bytes == 2u) sr_put16(p, v);
    else *p = (uint8_t)v;
}

static inline void sr_st32(uint32_t va, uint32_t v) {
    uint8_t *p = va >= SR_CRX_BASE ? sr_page_at(va) : 0;
    if(p) sr_put32(p, v);
    else sr_st_walk(va, v, 4u);
}

static inline void sr_st16(uint32_t va, uint32_t v) {
    uint8_t *p = va >= SR_CRX_BASE ? sr_page_at(va) : 0;
    if(p) sr_put16(p, v);
    else sr_st_walk(va, v, 2u);
}

static inline void sr_st8(uint32_t va, uint32_t v) {
    uint8_t *p = va >= SR_CRX_BASE ? sr_page_at(va) : 0;
    if(p) *p = (uint8_t)v;
    else sr_st_walk(va, v, 1u);
}

/* The ROM mirror reads model2_workram_mirror_* does (no RAM overlay). */
static inline uint32_t sr_rom_mirror32(uint32_t va) {
    uint32_t v = 0;
    const uint8_t *m = sr_rom_mirror(va);
    if(m) v = sr_get32(m);
    else { const uint8_t *p = sr_at(va); if(p) v = sr_get32(p); }
    return v;
}

/* Float words are kept as raw bits in RAM; these convert without aliasing. */
static inline float sr_f(uint32_t bits) { union { uint32_t u; float f; } c = {bits}; return c.f; }
static inline uint32_t sr_b(float f) { union { float f; uint32_t u; } c = {f}; return c.u; }
static inline double sr_d(uint32_t lo, uint32_t hi) {
    union { uint64_t u; double d; } c = {(uint64_t)lo | ((uint64_t)hi << 32)};
    return c.d;
}

#endif
