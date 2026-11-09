/* enemy_select_latched_attack [0x02244e54]: Target lock, distance table, facing and timing choose action 0..3 and queue kind. */
#include "dh_work.h"
#include "dh.h"
/* 0x02244e54  select an attack vs the latched target (sibling of 0x0224519e, but
 * returns the chosen action 0..3 and keys off the facing side).
 * idx = arg0->unk120; early-out 0 if slot empty (g_06099590[idx]==0) or not locked
 *   on (!(unkE0 & 0x200)). dx = target->unk8 - arg0->unk8; side = (dx<0)?1:0;
 *   dx = |dx|; reject if >= 0x80000.
 * kind = arg0->unk168[(dx>>16)&0xF] (per-distance table); reject if 0.
 * If the timing gate func_02245fc4 is busy -> return 2.
 * Dispatch on kind (r = rng()):
 *   1 -> if r>0 return 3; require unk44==side, target->unk84 & 0x40D, and NOT in
 *        func_022452be range; commit unk100=kind, unk104=idx, return 1.
 *   2 -> if r>0 return 3; then (fallthrough) require unk44==side, unk100=kind, ret 1.
 *   7 -> require unk44==side; unk100=kind; return 1.
 *   3 -> throw if a grabbed matching-side enemy in range (func_022452be) -> return 2;
 *        else commit unk100=kind, return 1.
 *   6,else -> return 0.
 * fidelity: HIGH - sign flag from cmp/pz; unsigned distance test; widths from asm. */
extern s32 g_actor_slot_ids[];     /* slot-occupied flags (cluster 0x06099584 + 12) */
extern void *g_actor_slot_tasks[];   /* slot -> target wrapper (cluster + 44) */
extern s32 game_random_next(void);          /* rng 0x02210020 */
s32 enemy_classify_attack_timing(void *p, void *q);     /* 0x02245fc4 timing gate */
void *collision_probe_between_actors(void *a, void *b);   /* 0x022452be range/aim select */

s32 enemy_select_latched_attack(void *arg0) {
    u8 *dirtab = ACTOR_MP(arg0, distance_attack_table);
    s32 r = game_random_next();
    s32 idx = ACTOR_M32(arg0, target_slot);
    if (g_actor_slot_ids[idx] == 0)
        return 0;
    void *target = TASK_PTR(g_actor_slot_tasks[idx], work);
    if (!(ACTOR_M32(arg0, state_flags) & 0x200))
        return 0;
    s32 dx = ACTOR_M32(target, position_x) - ACTOR_M32(arg0, position_x);
    s32 side;
    if (dx >= 0) {
        side = 0;
    } else {
        side = 1;
        dx = -dx;
    }
    if (dx >= 0x80000)
        return 0;
    s8 kind = M8(dirtab, (s16)((u32)dx >> 16) & 0xF);
    if (kind == 0)
        return 0;
    if (enemy_classify_attack_timing(arg0, target) != 0)
        return 2;
    s8 facing = ACTOR_M8(arg0, facing_right);
    switch (kind) {
    case 1:
        if (r > 0)
            return 3;
        if (facing != side)
            return 0;
        if (!(ACTOR_M32(target, action_category) & 0x40D))
            return 0;
        if (collision_probe_between_actors(arg0, target) != 0)
            return 0;
        ACTOR_M32(arg0, queued_attack_kind) = kind;
        ACTOR_M32(arg0, queued_attack_slot) = idx;
        return 1;
    case 2:
        if (r > 0)
            return 3;
        /* fallthrough */
    case 7:
        if (facing != side)
            return 0;
        ACTOR_M32(arg0, queued_attack_kind) = kind;
        return 1;
    case 3: {
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
