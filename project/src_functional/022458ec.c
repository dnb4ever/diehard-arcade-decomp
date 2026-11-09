/* enemy_select_pending_reaction [0x022458ec]: Grab/lock flags and descriptor animation select behavior/action reaction. */
#include "dh_work.h"
#include "dh.h"
/* 0x022458ec  reaction/attack-type selector run when a pending state is set.
 * If actor->unkE0 bit 0x02000000 (already in a grab): stamp unkD4=0x12,
 *   unk84=0x200, unk88=0, unkE0|=0x08000000, unk108=0, unk41=1,
 *   unk5C = cfg->unkC (cfg = actor->unk180), return 2.
 * Else require unkE0 & 0x100 (locked on); else return 0.
 * target = g_060995B0[unk120]->unk8; return 0 if the slot is empty
 *   (g_06099590[unk120]==0) or we face the same way as the target.
 * Then dice func_02210020() and switch on rng&3:
 *   0: if (unk38 & 0xBE70) && (rng&7) <= unkE8 -> func_02249214(actor); return 1;
 *      else fall into case 1.
 *   1: set the "hurt/guard" pose (unkD4=0x13, unk84=1, unk88=0, unkE0&=0x300,
 *      unk108=0, unk41=1) and run proximity dispatch func_0224806a(actor,target);
 *      return 2.
 *   2: if target->unk84 & 0x400 (throwable): unk41=1; pick a throw anim -
 *      use cfg->unkA (and set unkE0|=0x02000000) when already grabbing or when
 *      (unk38 & 0x7800) && !(rng&7); otherwise cfg->unk4; return 3.
 *      else fall into case 3.
 *   3: unk41=1; unk5C = (s16)unk14C; return 4.
 * fidelity: HIGH - all three bsrf resolved (0x210020 rng / 0x249214 / 0x24806a);
 *   case 0->1 and 2->3 fallthroughs preserved; unk5C writes are s16. */
extern s32 g_actor_slot_ids[];     /* slot-occupied flags (cluster 0x06099584+12) */
extern void *g_actor_slot_tasks[];   /* slot -> target wrapper (cluster +44) */
s32 game_random_next(void);                  /* rng tick 0x02210020 */
void enemy_enter_state8(void *actor);          /* 0x02249214 special-attack start */
s32 enemy_dispatch_distance_response(void *self, void *other); /* 0x0224806a proximity dispatch */

s32 enemy_select_pending_reaction(void *actor) {
    s32 e0 = ACTOR_M32(actor, state_flags);
    s32 rng;
    s32 slot;
    void *target;

    if (e0 & 0x02000000) {
        void *cfg = ACTOR_MP(actor, reaction_descriptor);
        ACTOR_M32(actor, behavior_state) = DH_ENEMY_BEHAVIOR_POSE;
        ACTOR_M32(actor, action_category) = 0x200;
        ACTOR_M32(actor, action_state) = 0;
        ACTOR_M32(actor, state_flags) |= 0x08000000;
        ACTOR_M32(actor, action_phase) = 0;
        ACTOR_M8(actor, animation_control) = DH_ANIMATION_CONTROL_RESTART;
        ACTOR_M16(actor, animation_requested) = M16(cfg, 0xC);
        return 2;
    }
    if (!(e0 & 0x100))
        return 0;

    slot = ACTOR_M32(actor, target_slot);
    if (g_actor_slot_ids[slot] == 0)
        return 0;
    target = TASK_PTR(g_actor_slot_tasks[slot], work);
    if (ACTOR_M8(actor, facing_right) == M8(target, 0x44))
        return 0;

    rng = game_random_next();
    switch (rng & 3) {
    case 0:
        if ((ACTOR_M32(actor, render_flags) & 0xBE70) && (s32)(rng & 7) <= M32(actor, 0xE8)) {
            enemy_enter_state8(actor);
            return 1;
        }
        /* fall through */
    case 1:
        ACTOR_M32(actor, behavior_state) = DH_ENEMY_BEHAVIOR_STOP;
        ACTOR_M32(actor, action_category) = 1;
        ACTOR_M32(actor, action_state) = 0;
        ACTOR_M32(actor, state_flags) &= 0x300;
        ACTOR_M32(actor, action_phase) = 0;
        ACTOR_M8(actor, animation_control) = DH_ANIMATION_CONTROL_RESTART;
        enemy_dispatch_distance_response(actor, target);
        return 2;
    case 2:
        if (M32(target, 0x84) & 0x400) {
            void *cfg = ACTOR_MP(actor, reaction_descriptor);
            ACTOR_M8(actor, animation_control) = DH_ANIMATION_CONTROL_RESTART;
            if ((ACTOR_M32(actor, state_flags) & 0x02000000) ||
                ((ACTOR_M32(actor, render_flags) & 0x7800) && !(rng & 7))) {
                ACTOR_M32(actor, state_flags) |= 0x02000000;
                ACTOR_M16(actor, animation_requested) = M16(cfg, 0xA);
            } else {
                ACTOR_M16(actor, animation_requested) = M16(cfg, 4);
            }
            return 3;
        }
        /* fall through */
    case 3:
        ACTOR_M8(actor, animation_control) = DH_ANIMATION_CONTROL_RESTART;
        ACTOR_M16(actor, animation_requested) = ACTOR_M16(actor, state2_animation);
        return 4;
    default:
        return 0;
    }
}
