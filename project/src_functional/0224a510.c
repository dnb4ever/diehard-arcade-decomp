/* shc_shift_sled_negative_one [0x0224a510]: Runtime sled leaf returns -1. */
#include "dh.h"

s32 shc_shift_sled_negative_one(s32 r0) {
    (void)r0;
    return -1;
}

/* fidelity: high - direct translation of mov #-1,r0; rts; mov.l @r15+,r2 (return value -1, pop return address) */
