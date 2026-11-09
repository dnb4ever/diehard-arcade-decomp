/* task_step_work_x_left_half [0x0220686a]: Follows task+8 and subtracts 0x8000 from work X+8. */
/* func_0220686a (cart 0x20686a) - leaf.
 * e = arg0->[8]; e->[8] -= 0x8000.
 * Confidence: HIGH. No calls. Faithful to disasm.
 */
#include "decomp.h"

void task_step_work_x_left_half(void *arg0) {
    void *e = FPTR(arg0, 8);
    FS32(e, 8) -= 0x8000;
}
