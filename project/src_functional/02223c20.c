/* display_clear_work_heads [0x02223c20]: Clears the first longword of each line in four shared render work buffers. */
#include "dh.h"
/* 0x02223c20  zero four HWRAM work buffers via zero_fill_aligned (0x022148ac), reached through
 * a mova+displacement function pointer (jsr @r14).
 * fidelity: HIGH - exact; counts: 0xC800, 0x7D00, 0x1400, 0xC80. */
extern void zero_fill_aligned(void *dst, s32 count);
void display_clear_work_heads(void) {
    zero_fill_aligned((void *)0x0609F850, 0xC800);
    zero_fill_aligned((void *)0x060AC050, 0x7D00);
    zero_fill_aligned((void *)0x0609D7D0, 0x1400);
    zero_fill_aligned((void *)0x0609EBD0, 0xC80);
}
