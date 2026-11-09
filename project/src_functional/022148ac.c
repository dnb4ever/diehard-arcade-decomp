/* cache_clear_line_heads [0x022148ac]: Clears the first longword of each aligned sixteen-byte cache mirror line. */
#include "dh_band21.h"
/* 0x022148ac (zero_fill_aligned) - clear one 32-bit word at the start of every
 * 16-byte stride across [base,base+len). The base is first normalised to
 * (base & 0x1FFFFFF0) | 0x40000000 (16-byte aligned, mapped into the 0x4xxxxxxx
 * mirror), and the loop writes 0 then advances by 0x10. Note it writes only the
 * first longword of each 16-byte unit, not a full fill - faithful to the ROM.
 * fidelity: HIGH - mov.l store, unsigned address compare. Leaf. */
void cache_clear_line_heads(u32 base, u32 len) {
    u32 p = (base & 0x1FFFFFF0u) | 0x40000000u;
    u32 end = p + len;

    while (p < end) {
        MEM32(p) = 0;
        p += 0x10;
    }
}
