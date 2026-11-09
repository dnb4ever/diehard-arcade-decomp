/* player_actor_anim_step [0x0222a418]: Existing map corroborated by source: ALT name; symbols.txt also lists actor_anim_step @ same addr - unresolved, keep both (src symbols_post81.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x0222a418 (actor FSM anim-script step, player-actor family) - advance the actor's config-record
 * script (cfg = entity->unk d0, 8-byte records) one step per matched key, driving an fp-table
 * animation dispatch. Handler ABI (task): entity = task->unk8.
 *   animation_update(task, entity);                            (actor_anim_frame_update)
 *   ((void(**)(void*))0x060911b4)[entity->unk ce](entity);     (per-anim fp table)
 *   idx = (s8)entity->unk34;  m = (s16)cfg[4];
 *   if (m != 0 && m == (u16)entity->unk58) {          // matched key: run the record
 *       if (entity->unk c4 != 0) { entity->unk c4--; entity->unk41 = 1; }   // still waiting
 *       else advance cfg += 8 and apply the next record (sets unk41 = 1)
 *   } else if (entity->unk54 & 2) {                    // unmatched but armed: same advance w/o unk41
 *       if (entity->unk c4 != 0) entity->unk c4--;
 *       else advance cfg += 8 (sets entity->unk d0 but not unk41)
 *   }
 *   Record dispatch (both arms): cfg[0]==-1 -> if idx<3 morph *task=&func_02228508 & entity->unk d0=0,
 *     else entity->unk3c=16; cfg[0]==-2 -> entity->unk3c=16; else load cfg: entity->unk5c=cfg[0],
 *     entity->unk c4=(s16)cfg[2]-1, if (u16)cfg[6]!=0xffff entity->unk26=cfg[6], entity->unk d0=cfg.
 *   Tail: if entity->unk3c==16 -> (idx<3?func_02227c5c:func_02227c22)(entity), morph *task=&func_0222a5ca.
 *   Then a unk98 countdown: if entity->unk6d && entity->unk98 && --entity->unk98==0 -> set up the
 *     unk60/62/64/74 block and tail func_0223a9d0(entity).
 * Branches resolved (braf_resolve): bsrf->0x22dc2c/0x227c5c/0x227c22, braf->0x23a9d0; morph fn-ptrs
 * mova+add->0x228508 (both arms) and ->0x22a5ca (ptr_resolve). r11 pinned 0.
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. cfg fields word (sign-extended), countdowns long. */
extern s32  actor_anim_frame_update(void *task, void *entity);  /* 0x0222dc2c animation_update */
extern void actor_clear_slot_without_count(void *entity);               /* 0x0227c5c (undecompiled) */
extern void actor_release_slot(void *entity);               /* 0x0227c22 (undecompiled) */
extern void actor_commit_effect_handoffs(void *entity);              /* 0x0223a9d0 (undecompiled) */
extern void player_initialize_task(void *task);                 /* 0x0228508 next FSM state */
extern void task_clear_handler(void *task);                /* 0x022a5ca next FSM state */

void player_actor_anim_step(void *task) {
    void *entity = TASK_PTR(task, work);
    void *cfg = ACTOR_MP(entity, animation_script);
    s32 idx;
    s32 m;

    actor_anim_frame_update(task, entity);
    ((void (**)(void *))(uintptr_t)0x060911b4)[MU16(entity, 0xce)](entity);
    idx = ACTOR_WORK(entity)->slot_id;
    m = (s16)M16(cfg, 4);

    if (m != 0 && m == ACTOR_U16(entity, animation_frame)) {
        if (M32(entity, 0xc4) != 0) {
            M32(entity, 0xc4) -= 1;
            ACTOR_WORK(entity)->animation_control = DH_ANIMATION_CONTROL_RESTART;
        } else {
            cfg = (u8 *)cfg + 8;
            if (M16(cfg, 0) == -1) {
                if (idx >= 3) {
                    M32(entity, 0x3c) = 16;
                } else {
                    *(void **)task = (void *)player_initialize_task;
                    ACTOR_MP(entity, animation_script) = 0;
                }
            } else if (M16(cfg, 0) == -2) {
                M32(entity, 0x3c) = 16;
            } else {
                ACTOR_WORK(entity)->animation_requested = M16(cfg, 0);
                M32(entity, 0xc4) = M16(cfg, 2) - 1;
                if (MU16(cfg, 6) != 0xffff)
                    ACTOR_WORK(entity)->angle_y = M16(cfg, 6);
                ACTOR_MP(entity, animation_script) = cfg;
                ACTOR_WORK(entity)->animation_control = DH_ANIMATION_CONTROL_RESTART;
            }
        }
    } else if (ACTOR_U16(entity, animation_events) & 2) {
        if (M32(entity, 0xc4) != 0) {
            M32(entity, 0xc4) -= 1;
        } else {
            cfg = (u8 *)cfg + 8;
            if (M16(cfg, 0) == -1) {
                if (idx >= 3) {
                    M32(entity, 0x3c) = 16;
                } else {
                    *(void **)task = (void *)player_initialize_task;
                    ACTOR_MP(entity, animation_script) = 0;
                }
            } else if (M16(cfg, 0) == -2) {
                M32(entity, 0x3c) = 16;
            } else {
                ACTOR_WORK(entity)->animation_requested = M16(cfg, 0);
                M32(entity, 0xc4) = M16(cfg, 2) - 1;
                if (MU16(cfg, 6) != 0xffff)
                    ACTOR_WORK(entity)->angle_y = M16(cfg, 6);
                ACTOR_MP(entity, animation_script) = cfg;
            }
        }
    }

    if (M32(entity, 0x3c) == 16) {
        if (idx >= 3)
            actor_release_slot(entity);
        else
            actor_clear_slot_without_count(entity);
        *(void **)task = (void *)task_clear_handler;
    }

    if (ACTOR_M8(entity, secondary_effect_type) != 0 && M32(entity, 0x98) != 0 && (M32(entity, 0x98) -= 1) == 0) {
        ACTOR_M16(entity, motion_heading) = ACTOR_WORK(entity)->angle_y;
        ACTOR_M16(entity, motion_pitch) = 0x1000;
        M16(entity, 0x64) = 0;
        ACTOR_M32(entity, motion_speed) = 0x2000;
        M8(entity, 0x45) = 0;
        actor_commit_effect_handoffs(entity);
    }
}
