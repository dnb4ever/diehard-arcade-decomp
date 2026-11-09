/* shc_shift_left23 [0x0224a3b0]: Runtime shift leaf returns input shifted twenty-three. */
#include "dh.h"

u32 shc_shift_left23(u32 r0) {
    r0 <<= 23;
    return r0;
} /* fidelity: high - direct translation of shll*7 + shll16 = 23-bit shift (Mac-corrected from model <<24) */
