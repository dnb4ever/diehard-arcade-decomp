/* enemy_select_mode_attack [0x02244fdc]: Secondary-type/team path selects peer by combat mode, else latched-attack selector. */
#include "dh_work.h"
#include "dh.h"
/* 0x02244fdc  per-frame "should I start an attack?" keyed on the global combat
 * mode g_06098564 (sibling of 0x0224519e / 0x02244e54). Tick rng first.
 * If actor->unk6D == 0 -> tail-call func_02244e54(actor) (the non-team path).
 * Otherwise resolve the active side/target from the mode:
 *   mode 0 -> return 0; mode 1 -> side 1; mode 2 -> side 2;
 *   mode 3 -> side = (g_060995D0[unk34].+4 >= .+8) ? 2 : 1 (unsigned);
 *   target = g_060995B0[side]->unk8.
 * dz = |target->unk10 - actor->unk10|; quad bit1 set when target is below;
 *   reject if dz > 0xC000.
 * dx = |target->unk8 - actor->unk8|; quad bit0 set when target is left;
 *   xside = quad & 1. kind = actor->unk168[(dx>>16) & 15]; reject if 0.
 * If dz > 0x1999 (far): require timing gate clear (func_02245fc4==0), the grab
 *   target g_060E0F4C[unk6F]->unk8->unk56 > 6, and facing == xside; then commit
 *   a dash: unk120=side, unk124=quad, unk100=5, return 1.
 * Else (near): if gate busy return 2; if func_022452be finds a grabbed enemy
 *   (unk34 in [3,6], !(unkE0&0x200000)) return 2; if target is open
 *   (unk8C & 0x16), facing differs, and !(rng&1) -> unk100=4, return 1;
 *   otherwise unk120=side, unk100=3, return 1.
 * fidelity: HIGH - the single bsrf (0x210020 rng) resolved; unsigned dx>>16;
 *   g_06098564 is only 0..3 so the asm's unset-reg default drops to return 0. */
extern s32 g_ready_player_mask;       /* active combat mode */
extern void *g_actor_slot_tasks[];   /* slot -> target wrapper (cluster 0x06099584+44) */
extern u8 g_060995D0[];      /* per-type side records, 0x20 stride (cluster +76) */
extern void *g_060E0F4C[];   /* grab-target table (indexed by unk6F) */
s32 game_random_next(void);                  /* rng tick 0x02210020 */
s32 enemy_classify_attack_timing(void *p, void *q);      /* 0x02245fc4 timing gate */
void *collision_probe_between_actors(void *a, void *b);    /* 0x022452be range/aim select */
s32 enemy_select_latched_attack(void *arg0);            /* 0x02244e54 non-team attack picker */

s32 enemy_select_mode_attack(void *actor) {
    u8 *atktab = ACTOR_MP(actor, distance_attack_table);
    s32 rng = game_random_next();
    s32 side;
    void *target;
    s32 dz, dx, quad, xside, base;

    if (ACTOR_M8(actor, secondary_effect_type) == 0)
        return enemy_select_latched_attack(actor);

    switch (g_ready_player_mask) {
    case 0:
        return 0;
    case 1:
        side = 1;
        target = TASK_PTR(g_actor_slot_tasks[1], work);
        break;
    case 2:
        side = 2;
        target = TASK_PTR(g_actor_slot_tasks[2], work);
        break;
    case 3:
        base = ACTOR_M8(actor, slot_id) * 0x20;
        side = (MU32(g_060995D0, base + 4) >= MU32(g_060995D0, base + 8)) ? 2 : 1;
        target = TASK_PTR(g_actor_slot_tasks[side], work);
        break;
    default:
        return 0; /* g_06098564 is only 0..3; asm default path is dead */
    }

    quad = 0;
    dz = ACTOR_M32(target, position_z) - ACTOR_M32(actor, position_z);
    if (dz < 0) {
        quad = 2;
        dz = -dz;
    }
    if (dz > 0xC000)
        return 0;
    dx = ACTOR_M32(target, position_x) - ACTOR_M32(actor, position_x);
    if (dx < 0) {
        quad |= 1;
        dx = -dx;
    }
    xside = quad & 1;
    if (M8(atktab, (s32)(((u32)dx >> 16) & 15)) == 0)
        return 0;

    if (dz > 0x1999) {
        void *g;
        if (enemy_classify_attack_timing(actor, target) != 0)
            return 0;
        g = MP(g_060E0F4C[ACTOR_M8(actor, secondary_effect_slot)], 8);
        if (M16(g, 0x56) <= 6)
            return 0;
        if (ACTOR_M8(actor, facing_right) != xside)
            return 0;
        ACTOR_M32(actor, target_slot) = side;
        M32(actor, 0x124) = quad;
        ACTOR_M32(actor, queued_attack_kind) = 5;
        return 1;
    } else {
        void *obj;
        if (enemy_classify_attack_timing(actor, target) != 0)
            return 2;
        obj = collision_probe_between_actors(actor, target);
        if (obj != 0) {
            s8 t = ACTOR_M8(obj, slot_id);
            if (t >= 3 && t <= 6 && !(ACTOR_M32(obj, state_flags) & 0x200000))
                return 2;
        }
        if ((ACTOR_M32(target, interaction_flags) & 0x16) && ACTOR_M8(target, facing_right) != xside &&
            !(rng & 1)) {
            ACTOR_M32(actor, queued_attack_kind) = 4;
            return 1;
        }
        ACTOR_M32(actor, target_slot) = side;
        ACTOR_M32(actor, queued_attack_kind) = 3;
        return 1;
    }
}
