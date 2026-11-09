/* enemy_update_grapple [0x022485b8]: Advances the five-phase grapple and eases the partner into the linked pose. */
#include "dh_work.h"
#include "dh.h"
/* 0x022485b8  actor grapple/tie-up handler (5-state FSM, states 0..4). ABI
 * (r4=task, r5=entity). animation_update (0x0222dc2c) is called through r12
 * (jsr @r12) mid-body and as the common tail. engaged = (u16)entity[0x54] & 2.
 *
 *   switch (entity[0xd8]) {
 *     case 0:
 *       animation_update(task, entity);
 *       if (engaged) entity[0xd8] = 1;
 *       partner = g_060995B0[entity[0x4b]]->unk8;
 *       partner[0x8]  += (entity[0x8]  + entity[0xc4] - partner[0x8])  >> 1;
 *       partner[0x10] += (entity[0x10] + entity[0xc8] - partner[0x10]) >> 1;
 *       return;
 *     case 1:
 *       partner = g_060995B0[entity[0x4b]]->unk8;
 *       if (partner[0x54] & 2) {                       (partner also grabbing)
 *         t = func_0220ff72(*(s32*)0x06099580, func_02226148);  (spawn tie-up task)
 *         t->unk8->unk10 = entity; t->unk8->unk14 = partner;
 *         entity[0xe0] &= ~0x4000;
 *         actor_init_from_cfg(entity, entity[0x104]);
 *         entity[0xd8] = 2;
 *       } else {                                       (ease partner toward entity)
 *         partner[0x8]  += (entity[0x8]  + entity[0xc4] - partner[0x8])  >> 1;
 *         partner[0x10] += (entity[0x10] + entity[0xc8] - partner[0x10]) >> 1;
 *         animation_update(task, entity);
 *       }
 *       return;
 *     case 2:
 *       if (entity[0x4b] != 0) return;
 *       switch (entity[0x88]) {                        (queued command)
 *         0x44: entity[0xbc]=entity[0x140]; entity[0x4b]=entity[0x67];
 *               animation_update(task,entity); entity[0x88]=0; func_022490a0(entity); return;
 *         0x49: entity[0xbc]=entity[0x140]; entity[0x4a]=entity[0x34];
 *               entity[0xd4]=0x11; entity[0xd8]=func_0223ffcc(task,entity); return;
 *         0x4a: entity[0xd8]=3; entity[0x84]=0x201; entity[0x88]=0;
 *               entity[0xe0]&=0x300; entity[0x108]=0; animation_update(task,entity); return;
 *         0x41/default: entity[0xbc]=entity[0x140]; func_02227704(entity);
 *               func_022499a6(entity); animation_update(task,entity); return;
 *       }
 *     case 3:
 *       if (engaged) {
 *         entity[0xbc]=entity[0x140]; func_02227704(entity);
 *         p = func_022461d8(entity[0x100]);
 *         side = (p && p[0x8] > entity[0x8]) ? 0 : 1;
 *         if (p && entity[0x44] != side) {
 *           entity[0xd8]=4; entity[0xe0]|=0x10000; entity[0x84]|=0x10; entity[0x5c]=entity[0x14a];
 *         } else func_022499a6(entity);
 *       }
 *       animation_update(task, entity); return;        (common tail)
 *     case 4:
 *       if (engaged) {
 *         entity[0x44]^=1; entity[0x26]+=0x8000; entity[0xe0]&=~0x10000; entity[0x5c]=entity[0x148];
 *         animation_update(task,entity); func_022499a6(entity);
 *       }
 *       animation_update(task, entity); return;        (common tail)
 *   }
 *
 * All computed branches resolved (braf_resolve) and the two PIC pointers
 * (animation_update, spawned handler func_02226148) with ptr_resolve. func_02227704
 * is 1-arg (verified). func_022461d8 takes entity[0x100] and returns a ptr (as in
 * 0x24852c). States >4 return with no action. fidelity: HIGH - hand-traced vs
 * m2c_fn --asm. */
extern void *task_spawn_child_work40(s32 id, void *handler);
extern void  actor_init_from_cfg(void *entity, s32 arg);
extern void  actor_resolve_facing(void *entity);
extern void  enemy_enter_optional_pose(void *entity);
extern void *player_get_active_work(s32 key);
extern s32   actor_dispatch_action_and_finish(void *task, void *entity);
extern void  enemy_spawn_clash_task(void *entity);                 /* 0x022490a0 */
extern void  actor_tieup_task_raw_entry(void *task);                 /* tie-up task handler */
extern void  actor_anim_frame_update(void *task, void *entity);   /* animation_update */

void enemy_update_grapple(void *task, void *entity) {
    s32 engaged = 2 & (u16)ACTOR_M16(entity, animation_events);
    void *partner;

    switch (ACTOR_M32(entity, behavior_phase)) {
    case 0:
        actor_anim_frame_update(task, entity);
        if (engaged) {
            ACTOR_M32(entity, behavior_phase) = 1;
        }
        partner = TASK_PTR(((void **)(uintptr_t)DH_ACTOR_SLOT_TASKS_ADDR)[ACTOR_M8(entity, paired_slot)], work);
        M32(partner, 0x8)  += (ACTOR_M32(entity, position_x)  + M32(entity, 0xC4) - M32(partner, 0x8))  >> 1;
        M32(partner, 0x10) += (ACTOR_M32(entity, position_z) + M32(entity, 0xC8) - M32(partner, 0x10)) >> 1;
        return;

    case 1:
        partner = TASK_PTR(((void **)(uintptr_t)DH_ACTOR_SLOT_TASKS_ADDR)[ACTOR_M8(entity, paired_slot)], work);
        if (MU16(partner, 0x54) & 2) {
            void *t = task_spawn_child_work40(*(s32 *)(uintptr_t)0x06099580, (void *)actor_tieup_task_raw_entry);
            MP(MP(t, 8), 0x10) = entity;
            MP(MP(t, 8), 0x14) = partner;
            ACTOR_M32(entity, state_flags) &= ~0x4000;
            actor_init_from_cfg(entity, ACTOR_M32(entity, queued_attack_slot));
            ACTOR_M32(entity, behavior_phase) = 2;
        } else {
            M32(partner, 0x8)  += (ACTOR_M32(entity, position_x)  + M32(entity, 0xC4) - M32(partner, 0x8))  >> 1;
            M32(partner, 0x10) += (ACTOR_M32(entity, position_z) + M32(entity, 0xC8) - M32(partner, 0x10)) >> 1;
            actor_anim_frame_update(task, entity);
        }
        return;

    case 2:
        if (ACTOR_M8(entity, paired_slot) != 0) {
            return;
        }
        switch (ACTOR_M32(entity, action_state)) {
        case 0x44:
            M32(entity, 0xBC) = M32(entity, 0x140);
            ACTOR_M8(entity, paired_slot) = M8(entity, 0x67);
            actor_anim_frame_update(task, entity);
            ACTOR_M32(entity, action_state) = 0;
            enemy_spawn_clash_task(entity);
            return;
        case 0x49:
            M32(entity, 0xBC) = M32(entity, 0x140);
            ACTOR_M8(entity, hit_source_slot) = ACTOR_M8(entity, slot_id);
            ACTOR_M32(entity, behavior_state) = DH_ENEMY_BEHAVIOR_REACTION;
            ACTOR_M32(entity, behavior_phase) = actor_dispatch_action_and_finish(task, entity);
            return;
        case 0x4A:
            ACTOR_M32(entity, behavior_phase) = 3;
            ACTOR_M32(entity, action_category) = 0x201;
            ACTOR_M32(entity, action_state) = 0;
            ACTOR_M32(entity, state_flags) &= 0x300;
            ACTOR_M32(entity, action_phase) = 0;
            actor_anim_frame_update(task, entity);
            return;
        case 0x41:
        default:
            M32(entity, 0xBC) = M32(entity, 0x140);
            actor_resolve_facing(entity);
            enemy_enter_optional_pose(entity);
            actor_anim_frame_update(task, entity);
            return;
        }

    case 3:
        if (engaged) {
            void *p;
            M32(entity, 0xBC) = M32(entity, 0x140);
            actor_resolve_facing(entity);
            p = player_get_active_work(ACTOR_M32(entity, queued_attack_kind));
            if (p != 0 && ACTOR_M8(entity, facing_right) != ((M32(p, 0x8) > ACTOR_M32(entity, position_x)) ? 0 : 1)) {
                ACTOR_M32(entity, behavior_phase) = 4;
                ACTOR_M32(entity, state_flags) |= 0x10000;
                ACTOR_M32(entity, action_category) |= 0x10;
                ACTOR_MU16(entity, animation_requested) = ACTOR_M16(entity, approach_animation);
            } else {
                enemy_enter_optional_pose(entity);
            }
        }
        actor_anim_frame_update(task, entity);
        return;

    case 4:
        if (engaged) {
            ACTOR_M8(entity, facing_right) ^= 1;
            ACTOR_MU16(entity, angle_y) += 0x8000;
            ACTOR_M32(entity, state_flags) &= ~0x10000;
            ACTOR_MU16(entity, animation_requested) = ACTOR_M16(entity, idle_animation);
            actor_anim_frame_update(task, entity);
            enemy_enter_optional_pose(entity);
        }
        actor_anim_frame_update(task, entity);
        return;
    }
}
