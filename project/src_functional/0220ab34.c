/* stats_increment_pair_counters74 [0x0220ab34]: Resident service phases 0/2 increment four fields +74/+78/+94/+98. */
#include "dh.h"
/* 0x0220ab34 (resident-cmd: register 4 actor coordinate fields) - issues the two-phase
 * resident command via the fp at 0x06000644 for four field addresses derived from the
 * actor base g_06098804. For each address A the sequence is
 *   n = (u16)(fp(0, A, 0) + 1);  fp(2, A, n);
 * (query current count, bump by one, commit). Addresses:
 *   g_06098804 + (arg0 & 1)*0x10 + 0x74   (per-player X-ish field)
 *   g_06098804 + (arg0 & 1)*0x10 + 0x78
 *   g_06098804 + g_06098800*0x10 + 0x94
 *   g_06098804 + g_06098800*0x10 + 0x98
 * fidelity: HIGH - identical shape to resident_cmd_issue_100 (0x0220ac0c); counts u16
 * (extu.w); fp cached once in r8. Verified vs asm. */
typedef s32 (*resident_fp)(s32, s32, u16);
extern resident_fp g_06000644;
extern s32 g_06098800;
extern s32 g_06098804;

void stats_increment_pair_counters74(s32 arg0) {
    resident_fp fp = g_06000644;
    s32 off = (arg0 & 1) * 0x10;
    s32 a;

    a = g_06098804 + off + 0x74;
    fp(2, a, (u16)(fp(0, a, 0) + 1));
    a = g_06098804 + off + 0x78;
    fp(2, a, (u16)(fp(0, a, 0) + 1));
    a = g_06098800 * 0x10 + g_06098804 + 0x94;
    fp(2, a, (u16)(fp(0, a, 0) + 1));
    a = g_06098800 * 0x10 + g_06098804 + 0x98;
    fp(2, a, (u16)(fp(0, a, 0) + 1));
}
