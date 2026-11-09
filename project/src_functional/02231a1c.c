/* attach_follow_task [0x02231a1c]: Existing map corroborated by source: follow/attach: copies partner-entity pos; kill on flags&1 disarm or flags&2 counter; drawlist tail */
#include "dh_work.h"
#include "dh.h"
/* 0x02231a1c (task handler) - projectile/attach follow step. task->unk8=entity,
 * task->unk14=partner node; src = partner->unk8 (partner entity).
 * Kill (task->unk0=0) if: (entity->unk3C & 1) and the partner entity is not armed
 *   (!(src->unk3C & 0x20) || partner->unk0==0); OR (entity->unk3C & 2) and the
 *   down-counter entity->unk58 reaches <=0. Otherwise snapshot pos (unk8/C/10 ->
 *   unk4C/50/54), copy partner-entity pos into entity, tail-call drawlist_prepend.
 * fidelity: HIGH - unk3C/unk8.. long; unk58 word (s16). braf 0x22eea2 -> func_0222eea2. */
extern void collision_queue_actor(void *entity);   /* drawlist_prepend */

void attach_follow_task(void *task) {
    void *ent = TASK_PTR(task, work);
    void *partner = TASK_PTR(task, parent);
    s32 flags = M32(ent, 0x3C);
    void *src = MP(partner, 8);
    if ((flags & 1) && (!(M32(src, 0x3C) & 0x20) || M32(partner, 0) == 0)) {
        TASK_S32(task, handler) = 0;
        return;
    }
    if (flags & 2) {
        s16 n = M16(ent, 0x58);
        M16(ent, 0x58) = (s16)(n - 1);
        if (n <= 0) {
            TASK_S32(task, handler) = 0;
            return;
        }
    }
    M32(ent, 0x4C) = M32(ent, 0x8);
    M32(ent, 0x50) = M32(ent, 0xC);
    M32(ent, 0x54) = M32(ent, 0x10);
    M32(ent, 0x8)  = M32(src, 0x8);
    M32(ent, 0xC)  = M32(src, 0xC);
    M32(ent, 0x10) = M32(src, 0x10);
    collision_queue_actor(ent);
}
