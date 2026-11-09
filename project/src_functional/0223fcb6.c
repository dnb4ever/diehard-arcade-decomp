/* enemy_route_collision_response [0x0223fcb6]: Chooses guard, hit and collision responses from the active descriptor and combat flags. */
#include "dh.h"
/* 0x0223fcb6 - actor "take a hit / resolve collision" decision handler.
 * node = *(actor->unk158)->unk4 (saved as r13). Walks several sub-decisions,
 * tail-calling specialised reaction handlers when one fires:
 *
 *  - if (actor->unkE0 & 0x100): res = func_02247d6e(actor); if res!=0 return,
 *    else tail-call func_022469f2(actor).
 *  - else if (actor->unk8C & 0x3FFFF)==0 and (actor->unkE0 & 0x40):
 *      t = func_02245d1c(actor); if t != 0 tail-call func_02249836(actor, t).
 *  - dispatch on actor_state_dispatch(actor): 1 -> tail func_02247036; 3 -> tail
 *    func_022470a2; 2 -> func_022470a2 then continue; 0/other continue.
 *  - gate: if ((actor->unk54 & 2)==0) require node!=0, actor->unk40==0, and
 *    (u16)node[actor->unk100 * 2] == (u16)actor->unk58, else return.
 *  - dispatch on func_02243fe0(actor): 2 -> tail func_02249fb0; 1 -> call
 *    actor_set_state_7, actor->unk5C = actor->unk14A, done; 0/other -> block below.
 *  - block (fe24): idx52=actor->unk34, idx=actor->unk128. v =
 *    g_06099584[76 + idx52*32 + idx*4]; require 0x90000 <= v < 0x400000 and
 *    (actor->unkE0 & 0x10). base = g_0609740c (+8 if actor->unk44!=0); n =
 *    (u16)base; require 10 <= n <= 14 and g_06099584[12 + idx*4] != 0. Store
 *    actor->unk120 = idx; p = g_06099584[44 + idx*4]; if func_02245fc4(p->unk8,
 *    actor)==0 then commit the "stagger" state: unkD4=18, unk84=1, unk88=0,
 *    unkE0=0x100000, unk41=1, unk5C=0x3DC.
 *
 * m2c failed: five braf tail-calls (unresolved targets) + mid-function literal
 * pools (one pool word 0x014A mis-decodes as `stc tbr`; it is data, not code).
 * branches resolved (braf/bsrf deltas via braf_resolve):
 *   0247d6e,02469f2,0245d1c,0249836,0244e2e,02470a2,0247036,0243fe0,
 *   024840a,0249fb0,0245fc4. fidelity HIGH. */
extern u8  g_actor_slot_registry[];
extern s32 g_0609740c;

extern s32  enemy_enter_guard(void *actor);
extern void enemy_reset_idle_or_walk(void *actor);
extern void *enemy_find_latched_effect_target(void *actor);
extern void enemy_begin_combo(void *actor, void *target);
extern s32  actor_state_dispatch(void *actor);
extern void enemy_enter_descriptor_state18(void *actor);
extern void enemy_dispatch_script(void *actor);
extern s32  enemy_sweep_toward_target(void *actor);
extern void actor_set_state_7(void *actor);
extern void enemy_enter_state19(void *actor);
extern s32  enemy_classify_attack_timing(void *a, void *actor);

void enemy_route_collision_response(void *actor) {
    void *node = MP(MP(actor, 0x158), 4);
    s32 st;

    if (M32(actor, 0xE0) & 0x100) {
        if (enemy_enter_guard(actor) != 0)
            return;
        enemy_reset_idle_or_walk(actor);
        return;
    }

    if ((M32(actor, 0x8C) & 0x3FFFF) == 0 && (M32(actor, 0xE0) & 0x40)) {
        void *target = enemy_find_latched_effect_target(actor);
        if (target != 0) {
            enemy_begin_combo(actor, target);
            return;
        }
    }

    st = actor_state_dispatch(actor);
    if (st == 1) { enemy_dispatch_script(actor); return; }
    if (st == 3) { enemy_enter_descriptor_state18(actor); return; }
    if (st == 2) { enemy_enter_descriptor_state18(actor); }

    if ((MU16(actor, 84) & 2) == 0) {
        if (node == 0)
            return;
        if (M8(actor, 64) != 0)
            return;
        if ((u16)M16(node, M32(actor, 0x100) << 1) != (u16)M16(actor, 88))
            return;
    }

    st = enemy_sweep_toward_target(actor);
    if (st == 2) { enemy_enter_state19(actor); return; }
    if (st == 1) {
        actor_set_state_7(actor);
        M16(actor, 92) = M16(actor, 0x14A);
        return;
    }

    /* fe24 block (st == 0 or default) */
    {
        s32 idx52 = M8(actor, 52);
        s32 idx   = M32(actor, 0x128);
        u32 v = MU32(g_actor_slot_registry, 76 + idx52 * 32 + idx * 4);
        s32 base, n;
        void *p;

        if (v < 0x90000u || v >= 0x400000u)
            return;
        if ((M32(actor, 0xE0) & 0x10) == 0)
            return;

        base = g_0609740c;
        if (M8(actor, 68) != 0)
            base += 8;
        n = (u16)base;
        if (n < 10 || n > 14)
            return;
        if (M32(g_actor_slot_registry, 12 + idx * 4) == 0)
            return;

        M32(actor, 0x120) = idx;
        p = MP(g_actor_slot_registry, 44 + idx * 4);
        if (enemy_classify_attack_timing(MP(p, 8), actor) != 0)
            return;

        M32(actor, 0xD4) = DH_ENEMY_BEHAVIOR_POSE;
        M32(actor, 0x84) = 1;
        M32(actor, 0x88) = 0;
        M32(actor, 0xE0) = 0x100000;
        M8(actor, 0x41) = 1;
        M16(actor, 92) = 0x3DC;
    }
}
