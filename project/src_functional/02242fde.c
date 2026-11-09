/* enemy_can_lunge_target [0x02242fde]: Z-distance, attack timing and grabbed-target eligibility gate lunge. */
#include "dh.h"
/* 0x02242fde  decide whether actor arg0 may lunge/attack at target arg1.
 * Reject if their z (field 0x10) differ by more than 0x6666 (signed window).
 * Reject if func_02245fc4 (timing/state gate) reports busy (non-zero).
 * Reject if the object returned by target-select (func_022452be) is a grabbed/
 * downed actor: unk34 in [3,6] and not flagged (unkE0 & 0x200000).
 * Otherwise return 1 (go).
 * fidelity: HIGH - signed z-window; func_022452be returns a pointer. */
void *collision_probe_between_actors(void *a, void *b);  /* 0x022452be range/aim select */
s32 enemy_classify_attack_timing(void *p, void *q);    /* 0x02245fc4 timing/state gate */

s32 enemy_can_lunge_target(void *arg0, void *arg1) {
    s32 za = M32(arg0, 0x10);
    s32 zb = M32(arg1, 0x10);
    if ((za - 0x6666) > zb || zb > (za + 0x6666))
        return 0;
    if (enemy_classify_attack_timing(arg0, arg1) != 0)
        return 0;
    {
        void *o = collision_probe_between_actors(arg0, arg1);
        if (o != 0) {
            s8 t = M8(o, 0x34);
            if (t >= 3 && t <= 6 && !(M32(o, 0xE0) & 0x200000))
                return 0;
        }
    }
    return 1;
}
