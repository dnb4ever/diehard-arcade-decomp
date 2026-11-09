/* 0x02215630  clip_test: bounds/visibility test on the current matrix @*(void**)0x06098878.
 * Returns -1 (reject) if m[+44]<0, or |m[+12]| > m[+44], or |m[+28]| > m[+44]; else 0.
 * fidelity: faithful - cmp/pz sign tests + neg for abs, cmp/gt compares. Leaf.
 * (The O0 r15 spill of the matrix ptr is a dead store, omitted.) */
#include "dh_band21.h"

s32 clip_test(void) {
    s32 *m = *PTR(s32 *, DH_MATRIX_STACK_CURSOR_ADDR);
    s32 lim = m[11];          /* +44 */
    s32 a;

    if (lim < 0) return -1;
    a = m[3];  if (a < 0) a = -a;  if (a > lim) return -1;   /* +12 */
    a = m[7];  if (a < 0) a = -a;  if (a > lim) return -1;   /* +28 */
    return 0;
}
