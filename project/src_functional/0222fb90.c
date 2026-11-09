/* spark_projectile_slide_task [0x0222fb90]: Existing map corroborated by source: per-frame horiz slide spark: actor_emit_sparks; kill on grid_overlap_find_type0(&ent.unk8,g_06088064) hit or unk58 expiry; x-only integrate unk8+=unk14; drawlist_pre (src symbols_gen7.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x0022fb90  spark_projectile_slide_task
 *
 * Per-frame task for a horizontally sliding spark projectile; sibling of
 * projectile_spark_arc_task (0x22fd54).  arg0 = task.
 *
 * Emits sparks (actor_emit_sparks), then dies if a collision query
 * grid_overlap_find_type0(&entity->unk8, g_06088064) reports a hit or the
 * lifetime counter unk58 runs out. Otherwise it saves the previous position
 * (unk4C/50/54), advances only in x (unk8 += unk14), queues itself on the draw
 * list and tail-draws a 2.0 x 1.0 scaled sprite.
 *
 * Previously deferred: three computed branches. braf_resolve.py pins them:
 *   22fba4 bsrf -> 0x217ae4 (grid_overlap_find_type0)
 *   22fbe4 bsrf -> 0x22eea2 (drawlist_prepend)
 *   22fbf6 braf -> 0x2154fc (sprite draw/scale, tail)
 *
 * fidelity: HIGH - collision/lifetime kill guards, the post-decrement of unk58,
 * the x-only integration and the (0x20000, 0x10000, 1) draw args all read from
 * ROM asm; all computed branches confirmed by braf_resolve.py.
 */
extern void  actor_emit_spark_children(void *task, void *entity);     /* actor_emit_sparks */
extern s32   grid_find_overlap_type_zero(void *vec, void *arg);          /* grid_overlap_find_type0 */
extern s32   collision_queue_actor(void *entity);                 /* drawlist_prepend */
extern void  sprite_draw_billboard(void *ent, s32 sx, s32 sy, s32 flag); /* sprite draw */
extern u8    g_06088064[];                                /* collision query arg */

void spark_projectile_slide_task(void *task) {
    void *entity = TASK_PTR(task, work);
    s16 count;

    actor_emit_spark_children(task, entity);

    if (grid_find_overlap_type_zero((u8 *)entity + 8, g_06088064) != 0) {
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

    collision_queue_actor(entity);
    sprite_draw_billboard(entity, 0x20000, 0x10000, 1);
}
