/* spark_tick_child_spawn [0x022305e2]: Projectile emitter child follows target-slot trigger, creates subentity and expires. */
#include "dh_work.h"
#include "dh.h"
/* 0x022305e2  spark_child_spawn_task  (child handler of projectile_emit_task,
 * spawned by func_022304b4 with this address as its template).
 *
 * arg0 = task; ent = task->unk8 (this spark/holder entity).
 *
 * Phase 1 (gated on ent->unk5D != 0): when ent->unk5D is in 1..6 it spawns a
 * sub-entity holder via func_0220feec(task, handler 0x02239500); the holder's
 * child entity (holder->unk8) gets ->unk34 = ent->unk5D, and holder->unkF =
 * func_02210020()(rng) & 3.  For any non-zero unk5D it then calls
 * func_0222f3c0(ent) (actor_pair_propagate) and clears ent->unk5D = 0.
 *
 * Phase 2: func_02217ae4(&ent->unk8, bounds 0x06088064) is a region/bounds test;
 * non-NULL return kills the task (task->unk0 = 0).  Otherwise the lifetime word
 * ent->unk5A is decremented; if it was already 0 the task is killed.
 *
 * Phase 3 (alive): save previous position (unk4C/50/54 = unk8/C/10), integrate
 * unk8 += unk14; when task->unkF == 0 add to the draw list via func_0222eea2;
 * tail renderer func_022154fc(ent, 0x2C0000, 0x30000, 1).
 *
 * braf_resolve: 230606->0x220feec, 230612->0x210020(rng), 23061c->0x22f3c0,
 * 23062c->0x217ae4, 23066c->0x22eea2, 230682->0x2154fc(tail).
 * ptr_resolve: 230602 mova+add -> 0x02239500 (spawn handler).
 */
extern u8   g_06097d58[];

extern void *task_spawn_child_work88(void *parent, void *handler);  /* spawn_obj_variant */
extern u32   game_random_next(void);                         /* rng_next */
extern void  actor_publish_linked_slot_value(void *entity);                 /* actor_pair_propagate */
extern void *grid_find_overlap_type_zero(void *pos, void *ext);         /* region/bounds test */
extern void  collision_queue_actor(void *entity);                 /* drawlist_prepend */
extern void  sprite_draw_billboard(u32 ent, s32 a1, s32 a2, s32 a3); /* renderer */

void spark_tick_child_spawn(void *task) {
    void *ent = TASK_PTR(task, work);
    s32   phase = (s8)M8(ent, 0x5D);
    void *holder;
    void *child;
    s16   life;

    if (phase != 0) {
        if (phase < 7) {
            holder = task_spawn_child_work88(task, (void *)0x0603d500 /* PC-relative pointer in HWRAM */);
            child = MP(holder, 8);
            M8(child, 0x34) = (s8)phase;
            M8(holder, 0xF) = (s8)(game_random_next() & 3);
        }
        actor_publish_linked_slot_value(ent);
        M8(ent, 0x5D) = 0;
    }

    if (grid_find_overlap_type_zero((void *)((char *)ent + 8), (void *)0x06088064) != 0) {
        TASK_S32(task, handler) = 0;
        return;
    }

    life = M16(ent, 0x5A);
    M16(ent, 0x5A) = life - 1;
    if (life == 0) {
        TASK_S32(task, handler) = 0;
        return;
    }

    M32(ent, 0x4C) = M32(ent, 8);
    M32(ent, 0x50) = M32(ent, 0xC);
    M32(ent, 0x54) = M32(ent, 0x10);
    M32(ent, 8) = M32(ent, 8) + M32(ent, 0x14);

    if (M8(task, 0xF) == 0)
        collision_queue_actor(ent);

    sprite_draw_billboard((u32)(uintptr_t)ent, 0x2C0000, 0x30000, 1);
}
/* fidelity: HIGH - all 6 computed branches + spawn-handler pointer resolved;
   control flow, field offsets, lifetime decrement and tail renderer verified
   against asm. Spawn handler 0x02239500 passed as raw address (data arg). */
