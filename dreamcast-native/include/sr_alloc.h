/* Allocation and block copies for the native code: on the console 32-byte aligned blocks (a cache line, the PVR's
 * and the store queues' unit) and SH4ZAM's copies, on the host the C library. Only bytes move, so results are the
 * same on both. */
#ifndef SR_ALLOC_H
#define SR_ALLOC_H
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#ifdef _arch_dreamcast
#include <malloc.h>
#include <sh4zam/shz_mem.h>
#endif

/* n bytes, 32-byte aligned (rounded up to whole lines). */
static inline void *sr_alloc(size_t n) {
    const size_t size = (n + 31u) & ~(size_t)31u;
#ifdef _arch_dreamcast
    return memalign(32, size ? size : 32);
#else
    return aligned_alloc(32, size ? size : 32);
#endif
}

static inline void sr_copy(void *dst, const void *src, size_t n) {
#ifdef _arch_dreamcast
    shz_memcpy(dst, src, n);
#else
    memcpy(dst, src, n);
#endif
}

/* Fill with one byte value. */
static inline void sr_fill(void *dst, uint8_t value, size_t n) {
#ifdef _arch_dreamcast
    if(!(((uintptr_t)dst | n) & 7u)) {
        shz_memset8(dst, value * 0x0101010101010101ull, n);
        return;
    }
#endif
    memset(dst, value, n);
}

/* n bytes, 32-byte aligned, zeroed. */
static inline void *sr_alloc_zero(size_t n) {
    void *p = sr_alloc(n);
    if(p) sr_fill(p, 0, (n + 31u) & ~(size_t)31u);
    return p;
}

#endif
