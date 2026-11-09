/* 0x02213404  clear_record_heads: zero the first longword of 256 records (stride 0xc)
 * in two parallel arrays @0x060bacc0 and @0x060bf4c0.
 * fidelity: faithful - r0=0 stored to @r1/@r2, both bases += 0xc, 256-iteration dt loop. Leaf. */
#include "dh_band21.h"

void clear_record_heads(void) {
    s8 *a = PTR(s8, 0x060bacc0);
    s8 *b = PTR(s8, 0x060bf4c0);
    int i;
    for (i = 0; i < 256; i++) {
        *(s32 *)a = 0;
        *(s32 *)b = 0;
        a += 0xc;
        b += 0xc;
    }
}
