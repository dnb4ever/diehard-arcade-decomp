/* 0x022149a0  mtx_stack_reset: reset the active-matrix stack pointer to its base
 * (0x0609887c) and tail-call load_matrix_default to fill it with the default matrix.
 *   *g_06098878 = 0x0609887c;        (braf delay slot stores the base ptr)
 *   load_matrix_default();            (tail jump -> load_matrix_default)
 * fidelity: HIGH - the store to *0x06098878 runs in the braf delay slot; the braf
 *   (delta 0xfffff60a) resolves to 0x213fb4 = load_matrix_default (reads *0x06098878). */
#include "dh_band21.h"

void load_matrix_default(void);                   /* 0x02213fb4 */

void mtx_stack_reset(void) {
    MEMU32(DH_MATRIX_STACK_CURSOR_ADDR) = 0x0609887c;
    load_matrix_default();
}
