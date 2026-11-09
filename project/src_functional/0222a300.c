/* actor_spawn_arm_task [0x0222a300]: Loads the spawn configuration, movement box and initial pose before installing the actor continuation. */
#include "dh_work.h"
#include "dh.h"
/* 0x0222a300 (actor FSM state, player-actor family) - a spawn/arm state. Handler ABI (task):
 * entity = task->unk8; cfg = entity->unk d0 (a config/record pointer).
 *   if ((u16)cfg[6] != 0xffff) { entity->unk26 = cfg[6]; entity->unk28 = 0; entity->unk24 = 0; }
 *   entity->unk20 = entity->unk22 = 0;
 *   entity->unk84 = 0x80000000;
 *   entity->unk c4 = (s16)cfg[2] - 1;                 (a countdown)
 *   func_0222db44(entity, (s16)cfg[0]);
 *   actor_slot_record_reg(entity, 0x5999, 0xffffb334, 0x4000, 0x14ccc);   (0x0221707c)
 *   if ((s8)entity->unk34 < 3 && *(s32*)0x06098668 != 52) {               (player index gate)
 *       if (*(u8*)(0x060e0e38 + (entity->unk34 - 1) * 16) == 1) func_0223dd96();
 *       func_0223df30(entity); func_0223e17c(entity);
 *   }
 *   entity->unk ce = (entity->unk5e == 46) ? 1 : 0;
 *   entity->unk5c = (s16)cfg[0];
 *   entity->unk41 = 1;
 *   animation_update(task, entity);         (actor_anim_frame_update)
 *   func_0222a418(task);                     (run the next state once)
 *   *task = &func_0222a418;                  (morph to it; mova+add -> 0x22a418)
 * Branches resolved (braf_resolve): bsrf->0x22db44/0x21707c/0x23dd96/0x23df30/0x23e17c/0x22dc2c;
 * morph fn-ptr mova+add->0x22a418 (ptr_resolve). r11 pinned 0.
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. cfg fields word (sign-extended), entity flags word. */
#define R32A(a) (*(s32 *)(uintptr_t)(a))
#define R8A(a)  (*(u8  *)(uintptr_t)(a))

extern void actor_load_pose_buffers(void *entity, s32 a1);  /* 0x0222db44 (undecompiled) */
extern void actor_register_movement_slot(void *entity, s32 a1, s32 a2, s32 a3, s32 a4); /* actor_slot_record_reg */
extern void player_restore_saved_action(void);                  /* 0x0223dd96 (undecompiled) */
extern void hud_update_player_counters(void *entity);          /* 0x0223df30 (undecompiled) */
extern void hud_rebuild_player_counters(void *entity);          /* 0x0223e17c (undecompiled) */
extern s32  actor_anim_frame_update(void *task, void *entity); /* 0x0222dc2c animation_update */
extern void player_actor_anim_step(void *task);            /* 0x0222a418 next FSM state */

void actor_spawn_arm_task(void *task) {
    void *entity = TASK_PTR(task, work);
    void *cfg = ACTOR_MP(entity, animation_script);

    if (MU16(cfg, 6) != 0xffff) {
        ACTOR_WORK(entity)->angle_y = M16(cfg, 6);
        ACTOR_WORK(entity)->angle_z = 0;
        ACTOR_WORK(entity)->angle_x = 0;
    }
    M16(entity, 0x20) = 0;
    M16(entity, 0x22) = 0;
    ACTOR_WORK(entity)->action_category = (s32)0x80000000;
    M32(entity, 0xc4) = M16(cfg, 2) - 1;
    actor_load_pose_buffers(entity, M16(cfg, 0));
    actor_register_movement_slot(entity, 0x5999, (s32)0xffffb334, 0x4000, 0x14ccc);
    if (ACTOR_WORK(entity)->slot_id < 3 && R32A(DH_SCENE_INDEX_ADDR) != DH_SCENE_SAVED_ACTION_BYPASS) {
        if (R8A(0x060e0e38 + (ACTOR_WORK(entity)->slot_id - 1) * 16) == 1)
            player_restore_saved_action();
        hud_update_player_counters(entity);
        hud_rebuild_player_counters(entity);
    }
    M16(entity, 0xce) = ACTOR_M16(entity, model_index) == 46 ? 1 : 0;
    ACTOR_WORK(entity)->animation_requested = M16(cfg, 0);
    ACTOR_WORK(entity)->animation_control = DH_ANIMATION_CONTROL_RESTART;
    actor_anim_frame_update(task, entity);
    player_actor_anim_step(task);
    *(void **)task = (void *)player_actor_anim_step;
}
