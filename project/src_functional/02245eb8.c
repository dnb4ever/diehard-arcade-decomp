/* enemy_test_player_pair_lock [0x02245eb8]: Scans player action/partner links and extends the enemy timestamp on a matching lock. */
#include "dh.h"
/* 0x02245eb8 (actor_scan_pair_slots) - scan paired-object slots 1..2 for a match with
 * actor `a`. The slot arrays live in g_06099584: active flags at +12 (= g_06099590[slot]),
 * paired-object pointers at +44 (= g_060995B0[slot]). For each active slot, follow pe->unk8
 * to entity e; if (e->unk84 & 0x20000000) bail returning 0. When a->unk8C has any of 0x39E:
 * a 0x3800-state hit with e->unk4A == a->unk34 bumps a->unk114 by 0x78 and returns 1.
 * Otherwise a 0x3000-state hit with the same id match returns 1. No match returns 0.
 * fidelity: HIGH - flags long, e->unk4A / a->unk34 s8, state masks per pooled consts. */
extern u8 g_actor_slot_registry[];

s32 enemy_test_player_pair_lock(void *a) {
    s32 r6 = 4, r7 = 4, i = 1;

    do {
        if (M32(g_actor_slot_registry, 12 + r6) != 0) {
            void *pe = MP(g_actor_slot_registry, 44 + r7);
            void *e = MP(pe, 8);
            s32 s = M32(e, 0x84);

            if (s & 0x20000000)
                return 0;
            if (M32(a, 0x8C) & 0x39E) {
                if ((s & 0x3800) && M8(e, 0x4A) == M8(a, 0x34)) {
                    M32(a, 0x114) += 0x78;
                    return 1;
                }
            } else {
                if ((s & 0x3000) && M8(e, 0x4A) == M8(a, 0x34))
                    return 1;
            }
        }
        i += 1;
        r6 += 4;
        r7 += 4;
    } while (i <= 2);
    return 0;
}
