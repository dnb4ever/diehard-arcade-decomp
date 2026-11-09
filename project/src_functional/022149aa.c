/* mtx_load_abc [0x022149aa]: Copies the twelve-long cached view matrix into the active matrix stack frame. */
/* 0x022149aa  mtx_load_abc: load the matrix cached at g_06098abc into the active matrix
 * (*g_06098878) via copy_longs(src=0x06098abc, dst=*g_06098878, 12).
 * fidelity: HIGH (extra - sibling merged into fn-unit member 0x2149a0). braf delta
 *   0xffffe8b6 resolves to 0x21326c = copy_longs; r6=12 set in the delay slot. */
#include "dh_band21.h"

void copy_longs(s32 *src, s32 *dst, s32 count);   /* 0x0221326c */

void mtx_load_abc(void) {
    copy_longs((s32 *)0x06098abc, (s32 *)(uintptr_t)MEMU32(DH_MATRIX_STACK_CURSOR_ADDR), 12);
}
