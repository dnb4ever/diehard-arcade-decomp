/* task_cancel_word [0x0220fe98]: Stores zero through caller pointer; cancellation primitive. */
#include "dh.h"
/* 0x0220fe98 - store zero through the pointer arg0 (*arg0 = 0). Trivial leaf.
 * fidelity: HIGH - transcribed from m2c. */
void task_cancel_word(s32 *arg0) {
    *arg0 = 0;
}
