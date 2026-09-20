/* Timer0 UART drain @ 0x25C68 — one MIDI byte when TxRDY. */
// @rom 0x25c68 +0x70 irq_timer0_latch

#include "i960_lift.h"
#include "i960_mem.h"
#include "model2_memory.h"

void irq_timer0_latch(u32 arg0, u32 arg1)
{
    u32 pending;
    u32 status;
    u32 cursor;
    u32 idx;
    u8 byte;

    (void)arg0;
    (void)arg1;

    /* convention: kind=leaf_bx  link g14→g1 bx */
    g1 = g14;
    g14 = 0;

    pending = i960_ld_u32(I960_WORKRAM, 0x20b180, 0);
    if (pending != 0u) {
        status = i960_mmio_read_u8(SOUND_UART_STATUS);
        if (status & 1u) {
            cursor = i960_ld_u32(I960_WORKRAM, 0x20b184, 0);
            idx = (cursor - pending) & 0x7fu;
            byte = (u8)i960_ld_u8(I960_ABS, 0x20b100u, idx);
            i960_st_u16(I960_ABS, SOUND_UART, 0, (u16)byte);
            i960_st_u32(I960_WORKRAM, 0x20b180, 0, pending - 1u);
        }
    }
    /* lda 0xc350,g0 — caller also reloads timer0 with this period. */
    g0 = 0xc350u;
}

/*
 * Host has no 25 MHz timer0 IRQ. Hardware BAL 0x25C68 every 0xC350 clocks
 * (~2 ms / byte). UART TxRDY is always set in HLE, so drain the ring once
 * per vblank — same ISR, not a bypass of the 0x20B100 software TX path.
 */
void irq_timer0_drain_pending(void)
{
    uintptr_t save_g0 = g0;
    uintptr_t save_g1 = g1;
    uintptr_t save_g14 = g14;
    unsigned guard;

    for (guard = 0; guard < 0x80u; guard++) {
        u32 pending = i960_ld_u32(I960_WORKRAM, 0x20b180, 0);

        if (pending == 0u)
            break;
        irq_timer0_latch(0, 0);
        if (i960_ld_u32(I960_WORKRAM, 0x20b180, 0) >= pending)
            break;
    }
    g0 = save_g0;
    g1 = save_g1;
    g14 = save_g14;
}
