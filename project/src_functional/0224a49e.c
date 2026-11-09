/* shc_swap_and_signfill [0x0224a49e]: Runtime byte/halfword swaps and OR with -128; nonstandard ABI retained. */
#include "dh.h"

s32 shc_swap_and_signfill(s32 arg0) {
    u32 temp;

    /* swap.w r0,r1 - swap high/low 16-bit words */
    temp = ((arg0 & 0xFFFF) << 16) | ((arg0 >> 16) & 0xFFFF);

    /* swap.b r1,r0 - swap high/low 8-bit bytes of lower 16 bits */
    temp = ((temp & 0xFF00) >> 8) | ((temp & 0x00FF) << 8) | (temp & 0xFFFF0000);

    /* mov #-128,r1; or r1,r0 */
    temp |= 0xFFFFFF80;

    return (s32)temp;
}

/* fidelity: high - direct translation of swap.w, swap.b, and OR operations */
