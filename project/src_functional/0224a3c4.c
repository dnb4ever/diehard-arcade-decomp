/* shc_shift_left27 [0x0224a3c4]: Runtime shift leaf returns input shifted twenty-seven. */
#include "dh.h"

extern s32 shc_shift_left27(s32 r0);

s32 shc_shift_left27(s32 r0) {
    r0 <<= 3;
    r0 <<= 8;
    r0 <<= 16;
    return r0;
} /* fidelity: high - function performs a 27-bit left shift on input r0 via chained shift instructions */
