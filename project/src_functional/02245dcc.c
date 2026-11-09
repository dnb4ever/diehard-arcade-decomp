#include "dh_work.h"
/* enemy_attack_is_armed [0x02245dcc]: Ticks hit cooldown and rejects the armed attack under inactive-target and state-flag gates. */
#include "dh.h"
/* 0x02245dcc - actor pre-attack gate. Decrement cooldown +0x4E (clamp to >=0). If the
 * "armed" byte +0x48 is set: cancel (set +0x48=0, return 0) when weapon id +0x4A is 1..2
 * and g_06099590[id]==0; else by state flags +0xE0 - if bit 0x2000 set, clear +0x48 unless
 * 0x4000 also set, return 0; if bit 0x08000000 set and (+0xBA & 0x3F) < 11, clear +0x48,
 * set +0xBA |= 0x100, return 0; otherwise return 1. Returns 0 when not armed.
 * fidelity: HIGH - mov.b (signed id/cooldown), mov.l +0xE0, mov.w +0xBA. */
extern s32 g_actor_slot_ids[];

s32 enemy_attack_is_armed(void *a) {
    s8  k;
    s32 e;

    if (ACTOR_M8(a, hit_shake_ticks) != 0) {
        s8 v = ACTOR_M8(a, hit_shake_ticks) - 1;
        ACTOR_M8(a, hit_shake_ticks) = v;
        if (v < 0)
            ACTOR_M8(a, hit_shake_ticks) = 0;
    }
    if (ACTOR_M8(a, action_pending) == 0)
        return 0;

    k = ACTOR_M8(a, hit_source_slot);
    if (k >= 1 && k <= 2 && g_actor_slot_ids[k] == 0) {
        ACTOR_M8(a, action_pending) = 0;
        return 0;
    }

    e = ACTOR_M32(a, state_flags);
    if (e & 0x2000) {
        if (!(e & 0x4000))
            ACTOR_M8(a, action_pending) = 0;
        return 0;
    }
    if ((e & 0x08000000) && (ACTOR_M16(a, type_flags) & DH_ACTOR_TYPE_MASK) < 11) {
        ACTOR_M8(a, action_pending) = 0;
        ACTOR_M16(a, type_flags) |= 0x100;
        return 0;
    }
    return 1;
}
