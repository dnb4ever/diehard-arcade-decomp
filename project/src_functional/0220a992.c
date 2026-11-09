/* stats_add_counter60 [0x0220a992]: Resident service phases 1/3 read and add to work-base+60. */
/* func_0220a992 (cart 0x20a992) - callC (tail-call).
 * Same shape as func_0220a962 but with field offset 0x60:
 *   r = fn(1, (*0x06098804)+0x60, 0);
 *        fn(3, (*0x06098804)+0x60, arg0 + r);   // tail call
 * Confidence: HIGH. Indirect calls via the system fn pointer at 0x06000644.
 */
#include "decomp.h"

void stats_add_counter60(s32 arg0) {
    s32 r = ((s32 (*)(s32, s32, s32))*(void **)0x06000644)(
                1, *(s32 *)0x06098804 + 0x60, 0);
    ((void (*)(s32, s32, s32))*(void **)0x06000644)(
                3, *(s32 *)0x06098804 + 0x60, arg0 + r);
}
