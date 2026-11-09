/* projectile_spark_arc_task [0x0222fd54]: Existing map corroborated by source: per-frame arc spark: actor_emit_sparks; kill if unkC<=0x500 or unk58 expiry; integrate unk8+=unk14,unkC+=unk18; drawlist_prepend; tail renderer 0x2154fc(ent,0x20000, (src symbols_gen7.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x0022fd54  projectile_spark_arc_task
 *
 * Per-frame task for a spark-emitting arc projectile.  arg0 = task.
 *
 * Emits sparks (actor_emit_sparks), then dies if it has fallen below height
 * 0x500 (entity->unkC) or its lifetime counter unk58 has run out. Otherwise it
 * saves the previous position (unk4C/50/54), integrates x/y by velocity
 * (unk8 += unk14, unkC += unk18), queues itself on the draw list, and tail-draws
 * a 2.0 x 1.0 scaled sprite.
 *
 * Previously deferred: two computed branches. braf_resolve.py pins them:
 *   22fda4 bsrf -> 0x22eea2 (drawlist_prepend)
 *   22fdb6 braf -> 0x2154fc (sprite draw/scale, tail call)
 *
 * fidelity: HIGH - the height/lifetime kill guards, the post-decrement of unk58,
 * the velocity integration and the (0x20000, 0x10000, 1) draw args all read from
 * ROM asm; both computed branches confirmed by braf_resolve.py.
 */
extern void  actor_emit_spark_children(void *task, void *entity);     /* actor_emit_sparks */
extern s32   collision_queue_actor(void *entity);                 /* drawlist_prepend */
extern void  sprite_draw_billboard(void *ent, s32 sx, s32 sy, s32 flag); /* sprite draw */

void projectile_spark_arc_task(void *task) {
    void *entity = TASK_PTR(task, work);
    s16 count;

    actor_emit_spark_children(task, entity);

    if (M32(entity, 0x0C) <= 0x500) {
        TASK_S32(task, handler) = 0;
        return;
    }
    count = M16(entity, 0x58);
    M16(entity, 0x58) = count - 1;
    if (count <= 0) {
        TASK_S32(task, handler) = 0;
        return;
    }

    M32(entity, 0x4C) = M32(entity, 0x08);
    M32(entity, 0x50) = M32(entity, 0x0C);
    M32(entity, 0x54) = M32(entity, 0x10);
    M32(entity, 0x08) = M32(entity, 0x08) + M32(entity, 0x14);
    M32(entity, 0x0C) = M32(entity, 0x0C) + M32(entity, 0x18);

    collision_queue_actor(entity);
    sprite_draw_billboard(entity, 0x20000, 0x10000, 1);
}
