/* Helpers for wrap_<module>.c files (see sr_diff.h). */
#ifndef SR_WRAP_COMMON_H
#define SR_WRAP_COMMON_H
#include "sr_diff.h"
#include "sr_mem.h"
#include "i960_lift.h"
#include "model2_rom.h"

#include <stddef.h>
#include <stdint.h>

/* A routine taking and returning nothing beyond g0..g2: run the original, then `call` from the same state. */
#define SR_DIFF_WRAP3(name, call)                                         \
    void lifted_##name(u32 a0, u32 a1, u32 a2);                          \
    void name(u32 a0, u32 a1, u32 a2) {                                  \
        SR_DIFF_BEGIN();                                                 \
        lifted_##name(a0, a1, a2);                                       \
        SR_DIFF_NATIVE();                                                \
        call;                                                            \
        SR_DIFF_END(#name);                                              \
    }

/* Lifted helpers take either a guest address or a host frame pointer (geo_view_table_index_a's rule). */
static inline int sr_wrap_is_host(const void *p) {
    return (uintptr_t)p > 0xffffffffull || model2_ram_mut((u32)(uintptr_t)p) == NULL;
}

static inline uint8_t *sr_wrap_host_ptr(const void *p) {
    return sr_wrap_is_host(p) ? (uint8_t *)(uintptr_t)p : sr_at((u32)(uintptr_t)p);
}

#endif
