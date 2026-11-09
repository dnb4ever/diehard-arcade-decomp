/* 0x02216310  clear_displist_slots: zero the first two longwords of 4 records (24-byte stride)
 * starting at 0x06099514.
 * fidelity: faithful - r7=4 bound, r6 counter +2/iter, two records unrolled per dt-less loop
 *   (cmp/ge r7,r6 + bf/s), stores r4=0 at +0/+4, base += 24. Leaf. */
#include "dh_band21.h"

void clear_displist_slots(void) {
    s8 *p = PTR(s8, 0x06099514);
    int i;
    for (i = 0; i < 4; i++) {
        *(s32 *)(p + 0) = 0;
        *(s32 *)(p + 4) = 0;
        p += 24;
    }
}
