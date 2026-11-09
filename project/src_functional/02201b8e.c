/* task_noop [0x02201b8e]: Returns immediately; original entry is an exact rts/nop leaf. */
#include "dh.h"
/* 0x02201b8e (leafC) - no-op (rts; nop). An empty stub function.
 * fidelity: HIGH - exact. Leaf. */
void task_noop(void) {
}
