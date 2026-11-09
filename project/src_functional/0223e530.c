/* enemy_assign_minimum_cost_requests [0x0223e530]: Chooses per-priority minimum-cost request entries and marks their assignments. */
#include "dh.h"
/* 0x0223e530 (resource arbiter) - for each priority level 3..6, scan the active
 * request records in g_06096EC4[] (44B stride, count g_06097050; +0 active flag,
 * +8 entry count, entries start at +12 with {level@0, cost@4} 8B stride) and find
 * the minimum cost at that level. Write to g_06097058[level] (8B stride): if none
 * found leave word0 = -1; else word0 = 0, word1(+4) = winning record's +4 field,
 * then word0 is overwritten with the id(+0) of the last entry whose cost < the
 * winning minimum.
 * fidelity: HIGH - all fields long; cost compares unsigned (cmp/hi, cmp/hs).
 * No computed branches; fully static. Record count is assumed >= 1 (dt/bf.s loops). */
extern s32 g_06097050;
extern u8  g_06096ec4[];
extern u8  g_06097058[];

void enemy_assign_minimum_cost_requests(void) {
    s32 level;
    for (level = 3; level < 7; level++) {
        u32 best = 0xFFFFFFFF;
        u8 *best_rec = (u8 *)0;
        s32 recs = g_06097050;
        u8 *rec = g_06096ec4;
        do {
            if (M32(rec, 0) != 0) {
                u8 *e = rec + 12;
                s32 m = M32(rec, 8);
                do {
                    if (M32(e, 0) == level && best > (u32)M32(e, 4)) {
                        best = (u32)M32(e, 4);
                        best_rec = rec;
                    }
                    e += 8;
                } while (--m != 0);
            }
            rec += 44;
        } while (--recs != 0);
        {
            u8 *out = &g_06097058[level * 8];
            M32(out, 0) = (s32)0xFFFFFFFF;
            if (best != 0xFFFFFFFF) {
                s32 m = M32(best_rec, 8);
                u8 *e = best_rec + 12;
                M32(out, 0) = 0;
                M32(out, 4) = M32(best_rec, 4);
                do {
                    if ((u32)M32(e, 4) < best)
                        M32(out, 0) = M32(e, 0);
                    e += 8;
                } while (--m != 0);
            }
        }
    }
}
