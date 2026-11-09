/* projectile_arc_task [0x0223026a]: Existing map corroborated by source: thrown-object arc: func_0222fe14, lifetime unk58, floor/collision(func_02217ae4) kill, drawlist + physics func_060194fc */
#include "dh_work.h"
#include "dh.h"
/* 0x0223026a (task handler) - thrown-object arc step. entity = task->unk8.
 * Run per-frame helper func_0222fe14(task, entity), then decrement lifetime
 * entity->unk58. On expiry (<=0) OR when it has fallen below the floor
 * (entity->unkC <= 0x500) OR collision func_02217ae4(&entity->unk8, g_06088064):
 *   on the expiry and collision paths copy entity->unk77/unk5B into partner
 *   (task->unk14) at +15/+18, then clear task. Otherwise snapshot pos, integrate
 *   (unk8+=unk14, unkC+=unk18), drawlist_prepend, physics func_022154fc, ++unk5A.
 * fidelity: HIGH - unk58/unk5A word; unk8.. long; partner bytes +15/+18.
 * bsrf 0x217ae4/0x22eea2; 0x215f6c -> func_022154fc. */
extern void actor_emit_spark_children(void *task, void *entity);
extern s32  grid_find_overlap_type_zero(void *pos, void *arg);
extern void collision_queue_actor(void *entity);           /* drawlist_prepend */
extern void sprite_draw_billboard(void *entity, s32, s32, s32);
extern u8   g_06088064[];

void projectile_arc_task(void *task) {
    void *ent = TASK_PTR(task, work);
    void *partner;
    s16 n;
    actor_emit_spark_children(task, ent);
    n = M16(ent, 0x58);
    M16(ent, 0x58) = (s16)(n - 1);
    if (n <= 0) {
        partner = TASK_PTR(task, parent);
        M8(partner, 15) = M8(ent, 0x77);
        M8(partner, 18) = M8(ent, 0x5B);
        TASK_S32(task, handler) = 0;
        return;
    }
    if (M32(ent, 0xC) <= 0x500) {
        TASK_S32(task, handler) = 0;
        return;
    }
    if (grid_find_overlap_type_zero((void *)((u8 *)ent + 8), g_06088064) != 0) {
        partner = TASK_PTR(task, parent);
        M8(partner, 15) = M8(ent, 0x77);
        M8(partner, 18) = M8(ent, 0x5B);
        TASK_S32(task, handler) = 0;
        return;
    }
    M32(ent, 0x4C) = M32(ent, 0x8);
    M32(ent, 0x50) = M32(ent, 0xC);
    M32(ent, 0x54) = M32(ent, 0x10);
    M32(ent, 0x8) += M32(ent, 0x14);
    M32(ent, 0xC) += M32(ent, 0x18);
    collision_queue_actor(ent);
    sprite_draw_billboard(ent, 0x20000, 0x10000, 1);
    M16(ent, 0x5A) = (s16)(M16(ent, 0x5A) + 1);
}
