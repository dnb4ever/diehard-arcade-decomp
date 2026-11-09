/* enemy_update_recovery [0x02247a8c]: Advances the recovery substate, timing gates and hit interrupts before animation. */
#include "dh_work.h"
#include "dh.h"
/* 0x02247a8c  actor behaviour handler, task-handler ABI (task, actor). Steps the
 * actor->unkD8 sub-state, then tail-calls animation_update (actor_anim_frame_update).
 * If actor->unk4C != 0: func_022489d0(actor); animation_update; return.
 * frame = g_060987BC; turn = actor->unk54 & 2; u58 = (u16)actor->unk58;
 * u90 = actor->unk90. switch (actor->unkD8):
 *  0: cooldown unkC4--. If it was still > 0: when unk104 in [1,2] and
 *       g_06099590[unk104]!=0, if the timing gate func_02245fc4(actor,peer) is
 *       busy run func_022469f2. If it expired: res = func_022460c8(actor);
 *       res 0/1 -> unkE4==6 ? (unkD8=2; actor_init_from_cfg(actor,&g_06086920)) :
 *                 (unkD8=1; unk41=1; unk5C = ((s16*)unk17C)[rng&3]);
 *       res 2 -> func_02247a16(actor,u90,turn); res 3 -> func_02247a3e(...).
 *  1: when turn set: if unk6D!=0 set unk114=frame and, when the grab target's
 *       unk56 < g_0608DDD4[unk6D] && unk6D<=2, func_02247a16 and skip the rest;
 *       if unk6D==0 set unk114 = g_0608DC04[unkE8+8]+frame; then
 *       func_02245eb8 ? func_022499a6 : func_022469f2.
 *  2: if actor_script_step==0: unk114=frame; grab target unk56<=0 -> func_02247a3e;
 *       else func_02245eb8 ? func_022499a6 : func_022469f2.
 *  3: if u58==u90: func_0223b01e; unkD8=4.
 *  4: if u58==u90: func_0223b1c0; unkD8=5.
 *  5: if turn: func_022469f2.
 *  6: if u58==u90: func_0223ac9c; unkD8=7.
 *  7: if turn: unk6A>=3 ? func_02245c88; func_022469f2.
 *  default: return without the shared animation_update.
 * fidelity: HIGH - all 17 computed branches resolved to fixed targets (none is a
 *   real jump table); jsr @r12 is the PIC animation_update 0x0222dc2c. */
extern s32 g_frame_tick;       /* global frame counter */
extern void *g_060E0F4C[];   /* grab-target wrappers (indexed by unk6F) */
extern s32 g_actor_slot_ids[];     /* slot-occupied flags (cluster 0x06099584+12) */
extern void *g_actor_slot_tasks[];   /* slot -> peer wrapper (cluster +44) */
extern s16 g_0608DDD4[];     /* per-unk6D reaction window table */
extern s32 g_0608DC04[];     /* per-unkE8 cooldown table */
extern s16 g_06086920[];     /* fixed actor config record */
void actor_anim_frame_update(void *task, void *actor);   /* animation_update 0x0222dc2c */
void actor_enter_hit_reaction(void *actor);                /* 0x022489d0 */
s32 enemy_classify_attack_timing(void *p, void *q);           /* 0x02245fc4 timing gate */
void enemy_reset_idle_or_walk(void *actor);               /* 0x022469f2 */
s32 enemy_classify_linked_effect(void *actor);                /* 0x022460c8 state picker */
s32 game_random_next(void);                       /* rng 0x02210020 */
void actor_init_from_cfg(void *actor, void *cfg);    /* 0x02244988 init-from-cfg */
void enemy_enter_dash_recovery_a(void *actor, void *peer, s32 turn); /* 0x02247a16 */
void enemy_enter_dash_recovery_b(void *actor, void *peer, s32 turn); /* 0x02247a3e */
s32 actor_script_step(void *actor);                /* 0x022449b2 */
s32 enemy_test_player_pair_lock(void *actor);                /* 0x02245eb8 */
void enemy_enter_optional_pose(void *actor);               /* 0x022499a6 */
void actor_begin_throw_effect(void *actor);               /* 0x0223b01e */
void actor_begin_hold_effect(void *actor);               /* 0x0223b1c0 */
void actor_spawn_special_debris(void *actor);               /* 0x0223ac9c */
void enemy_load_action_descriptor(void *actor);               /* 0x02245c88 */

void enemy_update_recovery(void *task, void *actor) {
    s32 frame, turn;
    u16 u58;
    u32 u90;

    if (M8(actor, 0x4C) != 0) {
        actor_enter_hit_reaction(actor);
        actor_anim_frame_update(task, actor);
        return;
    }

    frame = g_frame_tick;
    turn = MU16(actor, 0x54) & 2;
    u58 = MU16(actor, 0x58);
    u90 = MU32(actor, 0x90);

    switch (M32(actor, 0xD8)) {
    case 0: {
        s32 c4 = M32(actor, 0xC4);
        M32(actor, 0xC4) = c4 - 1;
        if (c4 > 0) {
            s32 s = M32(actor, 0x104);
            if (s >= 1 && s <= 2 && g_actor_slot_ids[s] != 0) {
                void *peer = TASK_PTR(g_actor_slot_tasks[s], work);
                if (enemy_classify_attack_timing(actor, peer) != 0)
                    enemy_reset_idle_or_walk(actor);
            }
        } else {
            s16 *q = MP(actor, 0x17C);
            switch (enemy_classify_linked_effect(actor)) {
            case 0:
            case 1:
                if (M32(actor, 0xE4) == 6) {
                    M32(actor, 0xD8) = 2;
                    actor_init_from_cfg(actor, g_06086920);
                } else {
                    M32(actor, 0xD8) = 1;
                    M8(actor, 0x41) = 1;
                    M16(actor, 0x5C) = q[game_random_next() & 3];
                }
                break;
            case 2:
                enemy_enter_dash_recovery_a(actor, MP(actor, 0x90), turn);
                break;
            case 3:
                enemy_enter_dash_recovery_b(actor, MP(actor, 0x90), turn);
                break;
            }
        }
        break;
    }
    case 1:
        if (turn != 0) {
            s8 u6d = M8(actor, 0x6D);
            int do_tail = 1;
            if (u6d != 0) {
                void *tgt = MP(g_060E0F4C[M8(actor, 0x6F)], 8);
                M32(actor, 0x114) = frame;
                if (M16(tgt, 0x56) < g_0608DDD4[u6d] && u6d <= 2) {
                    enemy_enter_dash_recovery_a(actor, tgt, turn);
                    do_tail = 0;
                }
            } else {
                M32(actor, 0x114) = g_0608DC04[M32(actor, 0xE8) + 8] + frame;
            }
            if (do_tail) {
                if (enemy_test_player_pair_lock(actor) != 0)
                    enemy_enter_optional_pose(actor);
                else
                    enemy_reset_idle_or_walk(actor);
            }
        }
        break;
    case 2:
        if (actor_script_step(actor) == 0) {
            void *tgt = MP(g_060E0F4C[M8(actor, 0x6F)], 8);
            M32(actor, 0x114) = frame;
            if (M16(tgt, 0x56) <= 0)
                enemy_enter_dash_recovery_b(actor, MP(actor, 0x90), turn);
            else if (enemy_test_player_pair_lock(actor) != 0)
                enemy_enter_optional_pose(actor);
            else
                enemy_reset_idle_or_walk(actor);
        }
        break;
    case 3:
        if ((u32)u58 == u90) {
            actor_begin_throw_effect(actor);
            M32(actor, 0xD8) = 4;
        }
        break;
    case 4:
        if ((u32)u58 == u90) {
            actor_begin_hold_effect(actor);
            M32(actor, 0xD8) = 5;
        }
        break;
    case 5:
        if (turn != 0)
            enemy_reset_idle_or_walk(actor);
        break;
    case 6:
        if ((u32)u58 == u90) {
            actor_spawn_special_debris(actor);
            M32(actor, 0xD8) = 7;
        }
        break;
    case 7:
        if (turn != 0) {
            if (M8(actor, 0x6A) >= 3)
                enemy_load_action_descriptor(actor);
            enemy_reset_idle_or_walk(actor);
        }
        break;
    default:
        return;
    }

    actor_anim_frame_update(task, actor);
}
