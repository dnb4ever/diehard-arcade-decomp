/* task_clear_handler [0x0222a5ca]: Clears the first word of the supplied task or control record. */
#include "dh.h"
/* 0x0222a5ca  store 0 at *arg0 (mov #0,r3; rts; mov.l r3,@r4)
 * fidelity: HIGH - exact. */
void task_clear_handler(void *p) {
    M32(p, 0x0) = 0;
}
