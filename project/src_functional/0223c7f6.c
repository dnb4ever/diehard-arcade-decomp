/* actor_step_effect_transition [0x0223c7f6]: Secondary effect type and interaction flags select followup action/animation state. */
#include "dh.h"
/* 0x0223c7f6 - advance an actor action/animation state machine; dispatches on
 * entity->unk6D. Returns 0 (no transition) or -1 (handled). entity = arg0.
 * If unk6D == 0: react only when unk6A == 1 -> unk84 = 0x00400000, unk43 = 41,
 *   return -1; else return 0.
 * If (unk8C & 0x039E) != 0: busy, return 0.
 * obj = g_060e0f4c[unk6F]; if obj->unk8->unk56 != 0 (still playing) return 0.
 * Else set unk84 = 0x00400000 and switch on unk6D:
 *   1: if unk72 -> unk43 = func_0223afc4(entity)+33 else func_0223ac50(entity);
 *   2: if unk73 -> unk43 = func_0223afc4(entity)+33 else func_0223ac50(entity);
 *   3: func_0223af0c(entity);  4: func_0223ac50(entity);
 *   default: if unk6C == 12 func_02239f78(entity); if unk6D == 8 then unk6D = 7,
 *     unk8C = (unk8C & ~0x100) | 0x80, g_060e0f0c[unk6F] = 7; func_0223ac50(entity).
 *   All non-zero cases return -1.
 * bsrf resolved: 0x23afc4, 0x23ac50, 0x23af0c, 0x239f78. fidelity: HIGH. */
extern void *g_060e0f4c[];
extern s32   g_060e0f0c[];
extern s32  actor_load_hit_pose(void *entity);
extern void actor_load_recovery_pose(void *entity);
extern void actor_enter_retarget_aim(void *entity);
extern void actor_restore_primary_channel(void *entity);

s32 actor_step_effect_transition(void *entity) {
    void *obj;
    void *sub;

    if (M8(entity, 0x6D) == 0) {
        if (M8(entity, 0x6A) != 1)
            return 0;
        M32(entity, 0x84) = 0x00400000;
        M8(entity, 0x43) = 41;
        return -1;
    }

    if ((M32(entity, 0x8C) & 0x039E) != 0)
        return 0;

    obj = g_060e0f4c[M8(entity, 0x6F)];
    sub = MP(obj, 8);
    if (M16(sub, 0x56) != 0)
        return 0;

    M32(entity, 0x84) = 0x00400000;

    switch (M8(entity, 0x6D)) {
    case 1:
        if (M8(entity, 0x72) != 0)
            M8(entity, 0x43) = (s8)(actor_load_hit_pose(entity) + 33);
        else
            actor_load_recovery_pose(entity);
        break;
    case 2:
        if (M8(entity, 0x73) != 0)
            M8(entity, 0x43) = (s8)(actor_load_hit_pose(entity) + 33);
        else
            actor_load_recovery_pose(entity);
        break;
    case 3:
        actor_enter_retarget_aim(entity);
        break;
    case 4:
        actor_load_recovery_pose(entity);
        break;
    default:
        if (M8(entity, 0x6C) == 12)
            actor_restore_primary_channel(entity);
        if (M8(entity, 0x6D) == 8) {
            M8(entity, 0x6D) = 7;
            M32(entity, 0x8C) = (M32(entity, 0x8C) & ~0x100) | 0x80;
            g_060e0f0c[M8(entity, 0x6F)] = 7;
        }
        actor_load_recovery_pose(entity);
        break;
    }
    return -1;
}
