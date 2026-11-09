/* stats_increment_indexed_counter [0x0220ac0c]: Index masked to 63 selects table offset plus work-base+100; resident read/increment/write. */
#include "dh.h"
/* 0x0220ac0c (resident_cmd_issue_100) - issue a 2-phase command through the resident
 * dispatcher fp at g_06000644. A = g_06051230[idx & 0x3F] + g_06098804 + 0x100 (real entry; fn member 0x20abfe is pool-contaminated).
 *   n = (u16)(fp(0, A, 0) + 1);  then  fp(2, A, n).
 * Both A expressions are recomputed (table + work-base reread) per phase, as in the ROM.
 * fidelity: HIGH - fp args (phase, addr, u16 count); addr base offset 0x100. */
typedef s32 (*resident_fp)(s32, s32, u16);
extern resident_fp g_06000644;   /* resident dispatcher fp */
extern s32 g_06051230[];          /* per-index base table */
extern s32 g_06098804;            /* work-area base */

s32 stats_increment_indexed_counter(s32 idx) {
    resident_fp fp = g_06000644;
    u16 n = (u16)(fp(0, g_06051230[idx & 0x3F] + g_06098804 + 0x100, 0) + 1);
    return fp(2, g_06051230[idx & 0x3F] + g_06098804 + 0x100, n);
}
