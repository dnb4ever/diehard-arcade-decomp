/* actor_emit_spark_children [0x0222fe14]: Spawns configured spark children from the partner actor or consumes the same random draws. */
#include "dh_work.h"
#include "dh.h"
/* 0x0022fe14  actor_emit_sparks
 *
 * Spawn up to three "spark/trail" sub-objects off an actor, gated by a config
 * bit.  arg0 = task, arg1 = entity.
 *
 * When the entity's partner slot (unk5D) is set, in range (<7), and the partner
 * is alive (not flag 0x400 at unk38, not flag 0x100 at unkBA):
 *   - if config bit ((g_060987EC[g_06098808+1]>>1)&1) is set, spawn three
 *     sub-objects via spawn_obj_variant(task, handler), seed each one's sub-struct
 *     (obj->unk8) with the entity position, and give the first two a randomised
 *     horizontal velocity  unk14 = -(entity->unk14 >> {5,6}) + (rng() & 0x1FFF);
 *   - otherwise burn two rng() draws so the stream stays in step.
 * The partner slot is then cleared.
 *
 * Previously deferred: m2c could not resolve the pooled jsr targets. Computed
 * via mova+offset from the pool (braf_resolve.py reports 0 bsrf/braf - these are
 * mova-relative absolute calls):
 *   r11 = 0x220feec  spawn_obj_variant
 *   r14 = 0x210020   rng()
 *   spawn handlers 0x238ba4 / 0x238c4e / 0x238cd2
 *
 * fidelity: HIGH - the gate chain, the three spawns, the >>5 / >>6 shifts, the
 * rng mask 0x1FFF and the balanced 2-draw else-path all read from ROM asm.
 */
extern s32   g_06098808;
extern u8    g_bios_status_buffer[];
extern void *g_actor_slot_tasks[];

extern void *task_spawn_child_work88(void *parent, void *handler);  /* spawn_obj_variant */
extern s32   game_random_next(void);                          /* rng */
extern void  effect_spark_fall_back_task(void);                          /* spark handler 1 */
extern void  effect_spark_fall_forward_task(void);                          /* spark handler 2 */
extern void  effect_flash_expand_task(void);                          /* spark handler 3 */

void actor_emit_spark_children(void *task, void *entity) {
    s32 slot = M8(entity, 0x5D);

    if (slot != 0) {
        if (slot < 7) {
            void *partner = TASK_PTR(g_actor_slot_tasks[slot], work);
            if (!(M32(partner, 0x38) & 0x400) && !(M16(partner, 0xBA) & 0x100)) {
                if ((g_bios_status_buffer[g_06098808 + 1] >> 1) & 1) {
                    void *obj, *sub;

                    obj = task_spawn_child_work88(task, (void *)effect_spark_fall_back_task);
                    sub = MP(obj, 8);
                    M32(sub, 0x08) = M32(entity, 0x08);
                    M32(sub, 0x0C) = M32(entity, 0x0C);
                    M32(sub, 0x10) = M32(entity, 0x10);
                    M32(sub, 0x14) = -(M32(entity, 0x14) >> 5)
                                     + (game_random_next() & 0x1FFF);

                    obj = task_spawn_child_work88(task, (void *)effect_spark_fall_forward_task);
                    sub = MP(obj, 8);
                    M32(sub, 0x08) = M32(entity, 0x08);
                    M32(sub, 0x0C) = M32(entity, 0x0C);
                    M32(sub, 0x10) = M32(entity, 0x10);
                    M32(sub, 0x14) = -(M32(entity, 0x14) >> 6)
                                     + (game_random_next() & 0x1FFF);

                    obj = task_spawn_child_work88(task, (void *)effect_flash_expand_task);
                    sub = MP(obj, 8);
                    M32(sub, 0x08) = M32(entity, 0x08);
                    M32(sub, 0x0C) = M32(entity, 0x0C);
                    M32(sub, 0x10) = M32(entity, 0x10);
                } else {
                    game_random_next();
                    game_random_next();
                }
            }
        }
        M8(entity, 0x5D) = 0;
    }
}
