/* player_join_next [0x02229df4]: Existing map corroborated by source: (extra) next state installed by player_join_commit_fsm. Dispatch on */
#include "dh_work.h"
#include "dh.h"
/* 0x02229df4 (player_join_next) - the player-actor FSM state installed by the join-commit state
 * func_02229c00 (it morphs *task here). Handler ABI (task): entity = task->unk8; idx = entity->unk34.
 * Dispatches on the signed byte g_0609868a[idx-1] (which func_02229c00 cleared to 0):
 *   case  1 : COMMIT the join - program the player's HUD/timer fields, publish the active-player
 *             slot g_06099590[idx]=idx, clear this player's bits in g_0609977c, update the paired
 *             object (g_060995b0), and morph *task -> func_0222863e.
 *   case -1 : CANCEL/leave - release_slot_entry, optional g_060e0f2c[unk6b]=5, func_0223dcc2(idx),
 *             func_0223e2ea, clear bits; if g_0609979a==1 just clear g_0609868a[idx-1] else
 *             func_02227c5c + *task = NULL.
 *   default : nothing extra.
 * All paths tail-call func_0222ca98(entity).
 * Branches resolved (braf_resolve): bsrf->0x23dcc2/0x23a6dc/0x227704/0x21707c/0x227704/0x22dc2c/
 *   0x2170b4/0x23dcc2/0x23e2ea/0x227c5c, braf->0x22ca98; morph fn-ptr mova+add->0x22863e (ptr_resolve).
 * r11 = task, r12 pinned 1, r13 pinned 0. Bounded at 0x22a032 (fn.extent over-runs into the two
 * trailing sibling functions). Cross-checked statement-for-statement vs m2c_fn --asm. fidelity: HIGH. */
#define R16A(a) (*(s16 *)(uintptr_t)(a))
#define RU16A(a) (*(u16 *)(uintptr_t)(a))
#define R32A(a) (*(s32 *)(uintptr_t)(a))
#define RU32A(a) (*(u32 *)(uintptr_t)(a))
#define R8A(a)  (*(s8  *)(uintptr_t)(a))
#define PA(a)   (*(void **)(uintptr_t)(a))

extern void player_clear_saved_action(s32 idx);              /* 0x0223dcc2 (undecompiled) */
extern void actor_load_pose_animation(void *entity);         /* 0x0223a6dc (undecompiled) */
extern void actor_resolve_facing(void *entity);         /* 0x02227704 (undecompiled) */
extern void actor_register_movement_slot(void *entity, s32 a1, s32 a2, s32 a3, s32 a4); /* actor_slot_record_reg */
extern s32  actor_anim_frame_update(void *task, void *entity); /* animation_update */
extern void release_slot_entry(void *entity);         /* 0x022170b4 (undecompiled) */
extern void hud_score_slots_init(void *entity);         /* 0x0223e2ea (undecompiled) */
extern void actor_clear_slot_without_count(void *entity);          /* 0x0227c5c (undecompiled) */
extern void draw_actor_hier_shadowed_entry(void *entity);         /* 0x0222ca98 (undecompiled; tail) */
extern void actor_update_task(void *task);            /* 0x022863e next FSM state */

void player_join_next(void *task) {
    void *entity = TASK_PTR(task, work);
    s32 idx = ACTOR_WORK(entity)->slot_id;
    s8 sel = R8A(0x0609868a + (idx - 1));

    if (sel == 1) {
        R8A(0x0609868a + (idx - 1)) = 0;
        player_clear_saved_action(idx);
        ACTOR_M8(entity, action_pending) = 0;
        if (RU16A(DH_ACTOR_POSE_SET_ADDR) == 1) {
            ACTOR_M16(entity, health) = RU16A(0x06099794) * 2;
            RU16A(0x0609978c + (idx - 1) * 2) = 1;
        } else {
            ACTOR_M16(entity, health) = R16A(0x06099794);
            RU16A(0x0609978c + (idx - 1) * 2) = RU16A(0x06099796);
        }
        ACTOR_WORK(entity)->animation_requested = R32A(0x06096c74 + idx * 4);
        M16(entity, 0x20) = 0;
        ACTOR_M16(entity, interaction_latch) = 0;
        ACTOR_WORK(entity)->hit_shake_ticks = 0x5a;
        if (R32A(DH_PLAY_SESSION_MODE_ADDR) == 1 && R32A(DH_VIEW_REQUEST_PRIORITY_ADDR) >= 3) {
            void *obj = MP(PA(DH_VIEW_TASK_ADDR), 0x8);
            R32A(DH_VIEW_REQUEST_PRIORITY_ADDR) = 3;
            M8(PA(DH_VIEW_TASK_ADDR), 0xd) = 1;
            M16(obj, 0xae) = 0;
            M16(obj, 0xb0) = 0;
            M16(obj, 0xb2) = 0;
        }
        M32(entity, 0xbc) = -1;
        R32A(0x0609975c + (idx - 1) * 4) = 0x1e;
        RU16A(0x06097420 + (idx - 1) * 2) = 8;
        if (ACTOR_M8(entity, secondary_effect_type) == 0 && (ACTOR_M32(entity, interaction_flags) & 0x02000000)) {
            actor_load_pose_animation(entity);
            actor_resolve_facing(entity);
            ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_SPECIAL_POSE;
        } else {
            ACTOR_WORK(entity)->action_category = 1;
            ACTOR_WORK(entity)->action_state = 0;
            ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_IDLE;
        }
        actor_register_movement_slot(entity, 0x5999, (s32)0xffffb334, 0x4000, 0x14ccc);
        actor_resolve_facing(entity);
        actor_anim_frame_update(task, entity);
        R32A(DH_ACTOR_SLOT_IDS_ADDR + idx * 4) = idx;
        R32A(0x0609977c) &= ~R32A(DH_ACTOR_SLOT_MASKS_ADDR + (idx - 1) * 4);
        if (RU16A(DH_ACTOR_POSE_SET_ADDR) != 0) {
            void *p = PA(DH_ACTOR_SLOT_TASKS_ADDR + (idx & 1) * 4 + 4);
            M16(MP(p, 0x8), 0x56) = RU16A(0x06099794) * 2;
        }
        *(void **)task = (void *)actor_update_task;
    } else if (sel == -1) {
        release_slot_entry(entity);
        if (ACTOR_M8(entity, saved_primary_effect_type) != 0)
            R32A(0x060e0f2c + ACTOR_M8(entity, saved_primary_effect_slot) * 4) = 5;
        player_clear_saved_action(idx);
        hud_score_slots_init(entity);
        R32A(0x0609977c) &= ~R32A(DH_ACTOR_SLOT_MASKS_ADDR + (idx - 1) * 4);
        if (RU16A(DH_ACTOR_POSE_SET_ADDR) == 1) {
            R8A(0x0609868a + (idx - 1)) = 0;
        } else {
            actor_clear_slot_without_count(entity);
            *(void **)task = 0;
        }
    }
    draw_actor_hier_shadowed_entry(entity);
}
