/* resident_increment_counter108 [0x0220aca8]: Increments the selected resident halfword counter at context offset 0x108. */
#include "dh.h"
/* 0x0220aca8 (resident_cmd_issue_108) - sibling of resident_cmd_issue_100 (0x0220ac0c)
 * with address base offset 0x108. A = g_06051230[idx & 0x3F] + g_06098804 + 0x108.
 *   n = (u16)(fp(0, A, 0) + 1);  then  fp(2, A, n).
 * fidelity: HIGH - identical shape; offset 0x108. */
typedef s32 (*resident_fp)(s32, s32, u16);
extern resident_fp g_06000644;
extern s32 g_06051230[];
extern s32 g_06098804;

s32 resident_increment_counter108(s32 idx) {
    resident_fp fp = g_06000644;
    u16 n = (u16)(fp(0, g_06051230[idx & 0x3F] + g_06098804 + 0x108, 0) + 1);
    return fp(2, g_06051230[idx & 0x3F] + g_06098804 + 0x108, n);
}
