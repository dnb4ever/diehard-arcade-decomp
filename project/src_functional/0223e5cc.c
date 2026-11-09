/* scu_timer0_set_short_interval [0x0223e5cc]: Writes 0x10 to SCU timer-0 compare register 25fe0090. */
#include "dh.h"
/* 0x0223e5cc (leafC) - write 0x10 to SCU register 0x25FE0090.
 * fidelity: HIGH - exact (mov.l #0x10 -> @0x25FE0090). */
void scu_timer0_set_short_interval(void) {
    *(volatile u32 *)SCU_T0C_ADDR = 0x10;
}
