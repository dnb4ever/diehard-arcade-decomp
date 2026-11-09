/* effect_cancel_task [0x02231b0e]: Clears task handler word at +0. */
#include "dh.h"
/* 0x02231b0e (leafC) - clear *arg0 (write 0 to 32-bit slot at arg0+0).
 * fidelity: HIGH - exact (mov #0,r3; mov.l r3,@r4). */
void effect_cancel_task(void *arg0) {
    M32(arg0, 0) = 0;
}
