#include "dh_work.h"
/* actor_enter_hit_reaction [0x022489d0]: Resets hit state/velocity and aligns position and reaction animation with attacker. */
#include "dh.h"
/* 0x022489d0  actor_hit_react: the hit/stun interrupt handler. Called (as a plain
 * 1-arg bsrf) by the per-tick actor handlers (0x24852c / 0x248104 / 0x248436 ...)
 * when entity[0x4c] != 0, i.e. the actor has just been struck. It resets the
 * actor into a hit-reaction state, snaps it to a position relative to the
 * attacker's slot, and picks the reaction animation from the hit command byte.
 * ABI: single arg r4 = entity (NOT the (task,entity) handler ABI).
 *
 *   attacker = g_060995B0[entity[0x4c]]->unk8;   (attacker actor record)
 *   entity[0xd8]=0; entity[0x10]=attacker[0x10];  (copy Y)
 *   entity[0x41]=1; entity[0xe0]=0x2000; entity[0x108]=2; entity[0xbc]=-1;
 *   entity[0x14]=entity[0x18]=entity[0x1c]=0;      (clear velocity)
 *   switch (entity[0x88]) {    (hit command)
 *     0x2b: entity[0xd4]=0xb;  ... anim 0x95/0x93
 *     0x2c: entity[0xd4]=0xc;  ... anim 0x79/0x77
 *     0x2d: entity[0xd4]=0xb;  ... anim 0x2e5/0x2e2 (keeps X in xpos)
 *   }   each: if facings match -> anim A + X=attacker.x +(-43950|+43438), flip;
 *            else -> angle[0x26]+=0x8000, anim B, X=attacker.x+(-50060|+50060).
 *            the +dx sign is chosen by the ATTACKER's facing (attacker[0x44]).
 *   if (entity[0x8c] & 0x3ffff) {      (actor is on-screen / collidable)
 *     entity[0x74]=0; entity[0x62]= facing?0xc000:0x4000; entity[0x60]=0x2000;
 *     entity[0x45]=0;
 *     func_0223a9d0(entity, 3, 0x2000, xpos);    (spawn hit effect / knockback)
 *     func_02245c88(entity);                      (braf tail, resolved)
 *   }
 * Both computed branches resolved with braf_resolve: bsrf->func_0223a9d0 (4-arg,
 * r4..r7) and braf tail->func_02245c88 (1-arg). xpos (r7) defaults to 0x8000 and
 * is only overwritten in the 0x2d equal-facing case. The dead rts epilogue at
 * 0x248bb2 (taken when entity[0x8c]&0x3ffff == 0) is the normal return.
 * fidelity: HIGH - hand-traced statement-by-statement vs m2c_fn --asm. */
extern void actor_commit_effect_handoffs(void *entity, s32 kind, s32 a, s32 xpos);
extern void enemy_load_action_descriptor(void *entity);

void actor_enter_hit_reaction(void *entity) {
    void *attacker = TASK_PTR(((void **)(uintptr_t)DH_ACTOR_SLOT_TASKS_ADDR)[ACTOR_M8(entity, hit_pending)], work);
    s32 xpos = 0x8000;

    ACTOR_M32(entity, behavior_phase) = 0;
    ACTOR_WORK(entity)->position_z = ACTOR_WORK(attacker)->position_z;
    ACTOR_WORK(entity)->animation_control = DH_ANIMATION_CONTROL_RESTART;
    ACTOR_WORK(entity)->state_flags = 0x2000;
    ACTOR_M32(entity, action_phase) = 2;
    M32(entity, 0xBC) = -1;
    ACTOR_WORK(entity)->movement_x = 0;
    ACTOR_WORK(entity)->movement_y = 0;
    ACTOR_WORK(entity)->movement_z = 0;

    switch (ACTOR_WORK(entity)->action_state) {
    case 0x2B:
        ACTOR_M32(entity, behavior_state) = 0xB;
        if (ACTOR_WORK(entity)->facing_right == ACTOR_WORK(attacker)->facing_right) {
            ACTOR_U16(entity, animation_requested) = 0x95;
            ACTOR_WORK(entity)->position_x = ACTOR_WORK(attacker)->position_x +
                (ACTOR_WORK(attacker)->facing_right != 0 ? (s32)0xFFFF5652 : 0xA9AE);
            ACTOR_WORK(entity)->facing_right ^= 1;
        } else {
            ACTOR_U16(entity, angle_y) += 0x8000;
            ACTOR_U16(entity, animation_requested) = 0x93;
            ACTOR_WORK(entity)->position_x = ACTOR_WORK(attacker)->position_x +
                (ACTOR_WORK(attacker)->facing_right != 0 ? (s32)0xFFFF3C74 : 0xC38C);
        }
        break;
    case 0x2C:
        ACTOR_M32(entity, behavior_state) = 0xC;
        if (ACTOR_WORK(entity)->facing_right == ACTOR_WORK(attacker)->facing_right) {
            ACTOR_U16(entity, animation_requested) = 0x79;
            ACTOR_WORK(entity)->facing_right ^= 1;
            ACTOR_WORK(entity)->position_x = ACTOR_WORK(attacker)->position_x +
                (ACTOR_WORK(attacker)->facing_right != 0 ? (s32)0xFFFF5652 : 0xA9AE);
        } else {
            ACTOR_U16(entity, angle_y) += 0x8000;
            ACTOR_U16(entity, animation_requested) = 0x77;
            ACTOR_WORK(entity)->position_x = ACTOR_WORK(attacker)->position_x +
                (ACTOR_WORK(attacker)->facing_right != 0 ? (s32)0xFFFF3C74 : 0xC38C);
        }
        break;
    case 0x2D:
        ACTOR_M32(entity, behavior_state) = 0xB;
        if (ACTOR_WORK(entity)->facing_right == ACTOR_WORK(attacker)->facing_right) {
            ACTOR_U16(entity, animation_requested) = 0x2E5;
            xpos = ACTOR_WORK(attacker)->position_x +
                (ACTOR_WORK(attacker)->facing_right != 0 ? (s32)0xFFFF5652 : 0xA9AE);
            ACTOR_WORK(entity)->position_x = xpos;
        } else {
            ACTOR_U16(entity, angle_y) += 0x8000;
            ACTOR_WORK(entity)->facing_right ^= 1;
            ACTOR_U16(entity, animation_requested) = 0x2E2;
            ACTOR_WORK(entity)->position_x = ACTOR_WORK(attacker)->position_x +
                (ACTOR_WORK(attacker)->facing_right != 0 ? (s32)0xFFFF3C74 : 0xC38C);
        }
        break;
    }

    if (ACTOR_M32(entity, interaction_flags) & 0x3FFFF) {
        ACTOR_M32(entity, motion_speed) = 0;
        ACTOR_MU16(entity, motion_heading) = (ACTOR_WORK(entity)->facing_right != 0) ? 0xC000 : 0x4000;
        ACTOR_MU16(entity, motion_pitch) = 0x2000;
        M8(entity, 0x45) = 0;
        actor_commit_effect_handoffs(entity, 3, 0x2000, xpos);
        enemy_load_action_descriptor(entity);
    }
}
