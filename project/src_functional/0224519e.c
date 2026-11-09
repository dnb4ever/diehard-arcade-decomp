/* enemy_try_latched_attack [0x0224519e]: Target lock, per-distance kind and timing gates commit queued attack. */
#include "dh_work.h"
#include "dh.h"
/* 0x0224519e  per-frame "should I start an attack against my latched target?"
 * idx = arg0->unk120 (target slot). Tick the rng. Early-out 0 if the slot is
 *   empty (g_06099590[idx]==0) or we are not locked on (!(unkE0 & 0x200)).
 * dx = |target->unk8 - arg0->unk8|; reject if >= 0x80000.
 * kind = arg0->unk168[(dx>>16) & 0xF]  (per-distance attack table); reject if 0.
 * Reject if the func_02245fc4 timing-gate is busy. Then dispatch on kind:
 *   1,2      -> commit: unk100 = kind, return 1
 *   6        -> require cooldown (now - unk114) >= 0xF0 frames; if a grabbed enemy
 *               of matching side is in range (func_022452be) return 2 (throw),
 *               else commit (unk100 = kind, return 1)
 *   3,7,else -> return 0.
 * fidelity: HIGH - unsigned distance/cooldown tests; g_06099584 cluster base; the
 *   dead 3rd reg (0x06099584) to func_02245fc4 is omitted (callee takes 2 args). */
extern s32 g_actor_slot_ids[];     /* slot-occupied flags (cluster 0x06099584 + 12) */
extern void *g_actor_slot_tasks[];   /* slot -> target wrapper (cluster + 44) */
extern u32 g_frame_tick;       /* frame counter */
extern s32 game_random_next(void);          /* rng tick 0x02210020 */
s32 enemy_classify_attack_timing(void *p, void *q);     /* 0x02245fc4 timing gate */
void *collision_probe_between_actors(void *a, void *b);   /* 0x022452be range/aim select */

s32 enemy_try_latched_attack(void *arg0) {
    u8 *dirtab = ACTOR_MP(arg0, distance_attack_table);
    game_random_next();
    s32 idx = ACTOR_M32(arg0, target_slot);
    if (g_actor_slot_ids[idx] == 0)
        return 0;
    void *target = TASK_PTR(g_actor_slot_tasks[idx], work);
    if (!(ACTOR_M32(arg0, state_flags) & 0x200))
        return 0;
    s32 dx = ACTOR_M32(target, position_x) - ACTOR_M32(arg0, position_x);
    if (dx < 0)
        dx = -dx;
    if (dx >= 0x80000)
        return 0;
    s8 kind = M8(dirtab, (s16)((u32)dx >> 16) & 0xF);
    if (kind == 0)
        return 0;
    if (enemy_classify_attack_timing(arg0, target) != 0)
        return 0;
    switch (kind) {
    case 1:
    case 2:
        ACTOR_M32(arg0, queued_attack_kind) = kind;
        return 1;
    case 6:
        if ((u32)(g_frame_tick - ACTOR_MU32(arg0, action_start_tick)) < 0xF0u)
            return 0;
        {
            void *o = collision_probe_between_actors(arg0, target);
            if (o != 0) {
                s8 t = M8(o, 0x34);
                if (t >= 3 && t <= 6 && !(M32(o, 0xE0) & 0x200000) &&
                    M8(o, 0x44) == ACTOR_M8(arg0, facing_right))
                    return 2;
            }
            ACTOR_M32(arg0, queued_attack_kind) = kind;
            return 1;
        }
    default:
        return 0;
    }
}
