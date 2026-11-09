#include "dh_work.h"
/* enemy_update_combat_think [0x02248104]: Runs the three-phase combat decision machine and hit interrupts before animation. */
#include "dh.h"
/* 0x02248104  actor_combat_think: per-tick actor AI/think handler, a 3-state FSM
 * on entity[0xd8], then tail-calls animation_update (0x0222dc2c).
 * ABI (r4=task, r5=entity). Sibling of the turn/flip FSM at 0x02248436: identical
 * prologue, identical hit-interrupt (entity[0x4c] != 0 -> func_022489d0(entity)),
 * identical animation_update(task, entity) tail.
 *
 *   if entity[0x4c] != 0 -> func_022489d0(entity)          (hit/stun interrupt)
 *   else switch (entity[0xd8]):
 *     case 0: countdown entity[0xc4]; when it reaches/passes 0:
 *               entity[0xd8]=1; entity[0xe0]&=0x300; func_02247e14(entity);
 *     case 1: if (actor_script_step(entity) == 0) {
 *               if (entity[0x8c] & 0x3ffff) {
 *                 ent2 = g_060E0F4C[entity[0x6f]]->unk8;
 *                 if (entity[0xe4] == 6) {
 *                   if ((s16)ent2->unk56 <= 0) { func_02247a3e(entity); break; }
 *                 } else {
 *                   if (entity[0xe0] & 0x100000) entity[0x114] = *(s32*)0x060987bc;
 *                   idx = entity[0x6d];
 *                   if ((s16)ent2->unk56 < g_0608DDD4[idx] && idx <= 2) {
 *                     func_02247a16(entity, idx); break; }
 *                 }
 *               }
 *               (common tail) if (func_02245eb8(entity)) func_022499a6(entity);
 *                             else func_022469f2(entity);
 *             }
 *     case 2: r = actor_script_step(entity);
 *             if (r == 1) {
 *               if ((u16)entity[0x11c] == 2) {
 *                 i = entity[0x120];
 *                 if (g_06099590[i] != 0) {
 *                   p = g_060995B0[i];
 *                   if (func_02245fc4(entity, p->unk8)) func_022469f2(entity);
 *                 }
 *               }
 *             } else if (r == 0) {
 *               ent2 = g_060E0F4C[entity[0x6f]]->unk8;
 *               if (entity[0xe4] == 6 && (s16)ent2->unk56 <= 0) func_02247a3e(entity);
 *               else if (func_02245eb8(entity)) func_022499a6(entity);
 *               else func_022469f2(entity);
 *             }
 *   always: animation_update(task, entity)   [braf tail, resolved].
 *
 * All 10 computed branches resolved with braf_resolve (the 11th that the linear
 * scanner reports, a `bsrf r0` at 0x248258, is a false positive inside the
 * mid-function literal pool). func_02245eb8 overwrites r5 with 4 in its own
 * prologue, so it genuinely takes a single argument here (the sibling 0x248436
 * over-modelled it with a dead 2nd arg). The dead rts epilogue after the braf
 * tail (0x2482d4..0x2482da) is unreachable. fidelity: HIGH - hand-traced vs
 * m2c_fn --asm. */
extern void actor_enter_hit_reaction(void *entity);
extern void enemy_choose_engagement(void *entity);        /* 0x02247e14 */
extern s32  actor_script_step(void *entity);
extern s32  enemy_test_player_pair_lock(void *entity);        /* ignores r5 (sets it to 4) */
extern s32  enemy_classify_attack_timing(void *entity, void *arg);
extern void enemy_reset_idle_or_walk(void *entity);
extern void enemy_enter_dash_recovery_a(void *entity, s8 arg);
extern void enemy_enter_dash_recovery_b(void *entity);
extern void enemy_enter_optional_pose(void *entity);
extern void actor_anim_frame_update(void *task, void *entity);   /* animation_update */

void enemy_update_combat_think(void *task, void *entity) {
    if (ACTOR_M8(entity, hit_pending) != 0) {
        actor_enter_hit_reaction(entity);
        actor_anim_frame_update(task, entity);
        return;
    }

    switch (ACTOR_M32(entity, behavior_phase)) {
    case 0: {
        s32 c = M32(entity, 0xC4);
        M32(entity, 0xC4) = c - 1;
        if (c <= 0) {
            ACTOR_M32(entity, behavior_phase) = 1;
            ACTOR_M32(entity, state_flags) &= 0x300;
            enemy_choose_engagement(entity);
        }
        break;
    }
    case 1:
        if (actor_script_step(entity) == 0) {
            if (ACTOR_M32(entity, interaction_flags) & 0x3FFFF) {
                void *ent2 = MP(((void **)(uintptr_t)0x060E0F4C)[ACTOR_M8(entity, secondary_effect_slot)], 8);
                if (ACTOR_M32(entity, state_index) == 6) {
                    if (M16(ent2, 0x56) <= 0) {
                        enemy_enter_dash_recovery_b(entity);
                        break;
                    }
                } else {
                    s8 idx = ACTOR_M8(entity, secondary_effect_type);
                    if (ACTOR_M32(entity, state_flags) & 0x100000) {
                        ACTOR_M32(entity, action_start_tick) = *(s32 *)(uintptr_t)DH_FRAME_TICK_ADDR;
                    }
                    if (M16(ent2, 0x56) < ((s16 *)(uintptr_t)0x0608DDD4)[idx] && idx <= 2) {
                        enemy_enter_dash_recovery_a(entity, idx);
                        break;
                    }
                }
            }
            if (enemy_test_player_pair_lock(entity) != 0) {
                enemy_enter_optional_pose(entity);
            } else {
                enemy_reset_idle_or_walk(entity);
            }
        }
        break;
    case 2: {
        s32 r = actor_script_step(entity);
        if (r == 1) {
            if ((u16)M16(entity, 0x11C) == 2) {
                s32 i = ACTOR_M32(entity, target_slot);
                if (((s32 *)(uintptr_t)DH_ACTOR_SLOT_IDS_ADDR)[i] != 0) {
                    void *p = ((void **)(uintptr_t)DH_ACTOR_SLOT_TASKS_ADDR)[i];
                    if (enemy_classify_attack_timing(entity, MP(p, 8)) != 0) {
                        enemy_reset_idle_or_walk(entity);
                    }
                }
            }
        } else if (r == 0) {
            void *ent2 = MP(((void **)(uintptr_t)0x060E0F4C)[ACTOR_M8(entity, secondary_effect_slot)], 8);
            if (ACTOR_M32(entity, state_index) == 6 && M16(ent2, 0x56) <= 0) {
                enemy_enter_dash_recovery_b(entity);
            } else if (enemy_test_player_pair_lock(entity) != 0) {
                enemy_enter_optional_pose(entity);
            } else {
                enemy_reset_idle_or_walk(entity);
            }
        }
        break;
    }
    default:
        break;
    }

    actor_anim_frame_update(task, entity);
}
