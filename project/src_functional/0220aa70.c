/* stats_increment_pair_counters70 [0x0220aa70]: Resident service phases 0/2 increment four fields +70/+78/+90/+98. */
#include "dh.h"
/* 0x0220aa70 (resident-cmd: register 4 actor coordinate fields) - sibling of func_0220ab34
 * (0x0220ab34) with the same two-phase resident command via the fp at 0x06000644, but at
 * field offsets 0x70/0x78/0x90/0x98. For each address A: n=(u16)(fp(0,A,0)+1); fp(2,A,n).
 *   g_06098804 + (arg0&1)*0x10 + 0x70
 *   g_06098804 + (arg0&1)*0x10 + 0x78
 *   g_06098804 + g_06098800*0x10 + 0x90
 *   g_06098804 + g_06098800*0x10 + 0x98
 * fidelity: HIGH - counts u16 (extu.w); fp cached once. Verified vs asm. */
typedef s32 (*resident_fp)(s32, s32, u16);
extern resident_fp g_06000644;
extern s32 g_06098800;
extern s32 g_06098804;

void stats_increment_pair_counters70(s32 arg0) {
    resident_fp fp = g_06000644;
    s32 off = (arg0 & 1) * 0x10;
    s32 a;

    a = g_06098804 + off + 0x70;
    fp(2, a, (u16)(fp(0, a, 0) + 1));
    a = g_06098804 + off + 0x78;
    fp(2, a, (u16)(fp(0, a, 0) + 1));
    a = g_06098800 * 0x10 + g_06098804 + 0x90;
    fp(2, a, (u16)(fp(0, a, 0) + 1));
    a = g_06098800 * 0x10 + g_06098804 + 0x98;
    fp(2, a, (u16)(fp(0, a, 0) + 1));
}
