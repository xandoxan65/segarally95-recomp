/* Auto-lifted ROM constant block — not executable code. */
/* source: /Users/craigs/Documents/segamod2/decomp/disasm/maincpu/maincpu_000000_50.asm */
// @rom 0x0 +0x50 boot_rom_header

#include "i960_lift.h"

/* i960 reset/boot header (SAT, PRCB pointer, initial IP) */
const u32 boot_rom_header[] = {
    0x00000000u, /* +0x000 */
    0x000000b0u, /* +0x004 */
    0x00000000u, /* +0x008 */
    0x00000420u, /* +0x00c */
    0xfffffb30u, /* +0x010 */
    0x00000000u, /* +0x014 */
    0x00000000u, /* +0x018 */
    0xffffffffu, /* +0x01c */
    0x00000000u, /* +0x020 */
    0x00000000u, /* +0x024 */
    0x00000000u, /* +0x028 */
    0x00000000u, /* +0x02c */
    0x00000000u, /* +0x030 */
    0x00000000u, /* +0x034 */
    0x00000000u, /* +0x038 */
    0x00000000u, /* +0x03c */
    0x00000000u, /* +0x040 */
    0x00000000u, /* +0x044 */
    0x00000000u, /* +0x048 */
    0x00000000u, /* +0x04c */
};

#define BOOT_ROM_HEADER_WORD_COUNT (sizeof(boot_rom_header) / sizeof(boot_rom_header[0]))
#define BOOT_ROM_HEADER_ROM_ADDR 0x00000000u
