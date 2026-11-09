/* text_write_attributed_cells [0x022100a2]: Row*0x80+column*2 destination; ORs each string byte with attribute. */
#include "dh.h"
/* 0x022100a2 - write a NUL-terminated byte string into a 16-bit entry table, OR-ing each byte with
 * a flag/attribute word. Destination = g_060997B0 + row*0x80 + col*2 (row = arg1, col = arg0), each
 * table slot a halfword. For each source byte b (until the next byte is 0): store (b | flags) as a
 * halfword and advance one slot. The terminating NUL is not stored.
 * fidelity: HIGH - source byte read (mov.b), dest halfword write (mov.w); loop tests the NEXT source
 * byte for 0. */
extern u8 g_060997B0[];   /* halfword entry table */

void text_write_attributed_cells(s32 col, s32 row, s32 flags, s8 *src) {
    s16 *dst = (s16 *)(g_060997B0 + (row << 7) + (col << 1));

    do {
        *dst = (s16)(*src++ | flags);
        dst++;
    } while (*src != 0);
}
