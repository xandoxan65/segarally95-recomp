#include "race/checkpoint.h"
#include "sr_mem.h"

uint32_t sr_checkpoint_fold(uint32_t h) {
    uint32_t i;
    for(i = 0; i < 0xc0; i++) {
        h ^= sr_ld32(0x500000u + i * 4);
        h *= 16777619u; /* FNV-1a over words */
    }
    return h;
}
