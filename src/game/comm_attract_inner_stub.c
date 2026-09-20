/* Mode-2 comm-attract inner handlers — temporary no-op stubs.
 *
 * Jump table @ workram 0x5AE480 (ROM staging @ maincpu 0xF480), indexed by
 * 0x20209C inner mode in comm_attract_board_dispatch @ 0xFA00 / 0xFCD8.
 *
 * Lifted: inner_0..9. inner_9 is a thin trampoline to board_tick. */

#include "i960_lift.h"
#include "lift_syms.h"

/*
 * Disasm maincpu_013810_40.asm:
 *   call 0x16540
 *   ret
 */
void comm_attract_inner_9(u32 arg0, u32 arg1, u32 arg2)
{
    comm_attract_board_tick(arg0, arg1, arg2);
}
