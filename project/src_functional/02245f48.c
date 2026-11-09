/* actor_scan_pair_slots_simple [0x02245f48]: Existing map corroborated by source: shorter sibling of 0x245eb8: 0x3800-hit with e->unk4A==a->unk34 -> ret 1 */
#include "dh.h"
/* 0x02245f48 (actor_scan_pair_slots_simple) - shorter sibling of actor_scan_pair_slots
 * (0x02245eb8). Scans paired-object slots 1..2 in g_06099584 (flags +12, pointers +44);
 * for each active slot, e = pe->unk8. If (e->unk84 & 0x20000000) return 0. A 0x3800-state
 * hit with e->unk4A == a->unk34 returns 1; otherwise 0.
 * fidelity: HIGH - flags long, e->unk4A / a->unk34 s8, state mask 0x3800. */
extern u8 g_actor_slot_registry[];

s32 actor_scan_pair_slots_simple(void *a) {
    s32 r5 = 4, r6 = 4, i = 1;

    do {
        if (M32(g_actor_slot_registry, 12 + r5) != 0) {
            void *pe = MP(g_actor_slot_registry, 44 + r6);
            void *e = MP(pe, 8);
            s32 s = M32(e, 0x84);

            if (s & 0x20000000)
                return 0;
            if ((s & 0x3800) && M8(e, 0x4A) == M8(a, 0x34))
                return 1;
        }
        i += 1;
        r6 += 4;
        r5 += 4;
    } while (i <= 2);
    return 0;
}
