/* shc_shift_left15 [0x0224a39c]: Runtime shift leaf returns input shifted fifteen. */
#include "dh.h"

/* This function takes its input value in r0 (per the nonstandard ABI for this pocket)
   and shifts it left by 15 bits (8 shll + 1 shll8). */
u32 shc_shift_left15(u32 r0) {
    r0 <<= 15;
    return r0;
}

/* fidelity: high - direct translation of 8x shll and 1x shll8 on input r0 */
