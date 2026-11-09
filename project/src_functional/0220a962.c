/* resident_add_session_value [0x0220a962]: Reads and increments the resident session word at context offset 0x58. */
/* func_0220a962 (cart 0x20a962) - callC (tail-call).
 * Two calls through the BIOS/system function pointer at 0x06000644:
 *   r = fn(1, (*0x06098804)+0x58, 0);
 *        fn(3, (*0x06098804)+0x58, arg0 + r);   // tail call
 * Confidence: HIGH. Indirect calls via a system fn pointer.
 */
#include "decomp.h"

void resident_add_session_value(s32 arg0) {
    s32 r = ((s32 (*)(s32, s32, s32))*(void **)0x06000644)(
                1, *(s32 *)0x06098804 + 0x58, 0);
    ((void (*)(s32, s32, s32))*(void **)0x06000644)(
                3, *(s32 *)0x06098804 + 0x58, arg0 + r);
}
