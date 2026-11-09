/* actor_compare_xz_quadrant [0x02243fba]: Returns the two-bit X/Z ordering mask between two actor positions. */
#include "dh.h"
/* 0x02243fba  compare two objects on fields [8] and [0x10], return a 2-bit mask.
 * bit0 set if a->field[8]  < b->field[8];
 * bit1 set if a->field[0x10] < b->field[0x10].
 * fidelity: HIGH - exact (cmp/ge = signed >=). */
s32 actor_compare_xz_quadrant(void *a, void *b) {
    s32 r = (M32(a, 8) < M32(b, 8)) ? 1 : 0;
    if (M32(a, 0x10) < M32(b, 0x10))
        r |= 2;
    return r;
}
