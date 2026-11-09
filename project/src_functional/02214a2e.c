/* 0x02214a2e  mtx_stack_push_dup: push the matrix stack and duplicate the current matrix
 * into the new top.
 *   old = *g_06098878; new = old + 48; *g_06098878 = new;   (push 0x30 bytes)
 *   copy_longs(old, new, 12);                                (tail: duplicate)
 * fidelity: HIGH (extra - sibling of fn-unit member 0x2149a0). braf delta 0xffffe82c
 *   resolves to 0x21326c = copy_longs; r6=12 in the delay slot; src=old top, dst=new top. */
#include "dh_band21.h"

void copy_longs(s32 *src, s32 *dst, s32 count);   /* 0x0221326c */

void mtx_stack_push_dup(void) {
    s32 *old = (s32 *)(uintptr_t)MEMU32(DH_MATRIX_STACK_CURSOR_ADDR);
    s32 *neu = old + 12;                      /* +0x30 bytes */
    MEMU32(DH_MATRIX_STACK_CURSOR_ADDR) = (u32)(uintptr_t)neu;
    copy_longs(old, neu, 12);
}
