/* text_write_attributed_block [0x022100c8]: Stack argument block contains destination, attribute and inline string; writes halfword cells. */
#include "dh.h"
/* 0x022100c8 - write a byte string (passed by value in the stack arg block) into a
 * halfword destination, OR-ing an attribute into each cell. The arg block is
 * {s16 *dst @0; s16 attr @4; char str[] @6}. For each character:
 * *dst++ = (u16)((s8)c | attr), continuing while the following character is nonzero
 * (do/while: at least one cell is written). Sibling of str_to_hw_table (0x022100a2).
 * fidelity: HIGH - mov.b sign-extended load, mov.w store. */
struct textcmd { s16 *dst; s16 attr; char str[2]; };

void text_write_attributed_block(struct textcmd cmd) {
    s16 *dst = cmd.dst;
    char *p = cmd.str;
    do {
        *dst++ = (s16)((s8)*p++ | cmd.attr);
    } while (*p != 0);
}
