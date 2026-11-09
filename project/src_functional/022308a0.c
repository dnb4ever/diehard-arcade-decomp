/* projectile_tick_attack_variant_a [0x022308a0]: One-shot target gate spawns effect cohort; otherwise motion and lifetime update. */
#include "dh_work.h"
#include "dh.h"
/* 0x022308a0 - action_setup_a "next handler": per-frame attack/shot setup + motion.
 * entity = task->unk8. On a one-shot trigger (entity->unk5D != 0, < 7, and the locked
 * target g_060995b0[unk5D]->unk8->unk38 lacks bit 0x400):
 *   if ((u8)g_060987ec[g_06098808 + 1] >> 1) & 1: spawn five effect sub-objects via
 *     spawn_obj_a(task, handler); each copies entity pos (unk8/C/10) into sub->unk8
 *     and (for the first four) seeds sub->unk20 = (rng()&0x1fff) - (entity->unk14>>N)
 *     (N=5 for handlers 0x238ba4 x2, N=6 for 0x238c4e x2; 0x238cd2 copies pos only);
 *   else advance the RNG four times.
 *   Then func_02238dde(); if (g_06098670==1 && g_06098830>=1) raise the late-join
 *   request on *g_0609882c (unkD=1, sub unkAE/B0/B2=0, g_06098830=1).
 *   Then actor_pair_propagate(entity); entity->unk5D = 0.
 * Motion: if grid_overlap_find_type0(entity+8, g_06088064)==0 and --entity->unk58 keeps
 *   the pre-decrement value > 0: snapshot pos to unk4C/50/54, advance unk8 += unk14,
 *   drawlist_prepend(entity), tail sprite_renderer(entity, 0x20000, 0x10000, 1).
 *   Otherwise kill the task (task->unk0 = 0).
 * fidelity: HIGH - all 5 bsrf/braf resolved (0x238dde,0x22f3c0,0x217ae4,0x22eea2,
 * tail 0x2154fc); spawn handlers via mova+add (0x238ba4/0x238c4e/0x238cd2); shar>>5/6
 * arithmetic; cmp/ge signed; unk58 pre-decrement test. func_02238dde is nilary. */
#define G32(a) (*(s32 *)(uintptr_t)(a))
#define GP(a)  (*(void **)(uintptr_t)(a))

extern void *task_spawn_child_work88(void *parent, void *handler); /* spawn_obj_a */
extern u32   game_random_next(void);                        /* rng_next */
extern void  display_mark_channel_bits(void);                         /* post-clone flag init */
extern void  actor_publish_linked_slot_value(void *entity);                 /* actor_pair_propagate */
extern s32   grid_find_overlap_type_zero(void *pos, void *grid);        /* grid_overlap_find_type0 */
extern void  collision_queue_actor(void *entity);                 /* drawlist_prepend */
extern void  sprite_draw_billboard(void *entity, s32 sx, s32 sy, s32 flag); /* sprite_renderer */
extern void  effect_spark_fall_back_task(void *task);
extern void  effect_spark_fall_forward_task(void *task);
extern void  effect_flash_expand_task(void *task);
extern void *g_actor_slot_tasks_legacy[];
extern u8    g_bios_status_buffer_legacy[];
extern u8    g_06088064[];

static void seed_sub(void *o, void *entity, s32 shift) {
    void *sub = MP(o, 8);
    s32 d;
    M32(sub, 8)  = M32(entity, 8);
    M32(sub, 12) = M32(entity, 12);
    M32(sub, 16) = M32(entity, 16);
    d = -(M32(entity, 20) >> shift);
    M32(sub, 20) = (s32)(game_random_next() & 0x1fff) + d;
}

void projectile_tick_attack_variant_a(void *task) {
    void *entity = TASK_PTR(task, work);
    s8 st = M8(entity, 0x5d);

    if (st != 0) {
        if (st >= 7) goto after_spawn;
        if (M32(TASK_PTR(g_actor_slot_tasks_legacy[st], work), 0x38) & 0x400) goto after_spawn;

        if ((g_bios_status_buffer_legacy[G32(0x06098808) + 1] >> 1) & 1) {
            void *o;
            seed_sub(task_spawn_child_work88(task, (void *)effect_spark_fall_back_task), entity, 5);
            seed_sub(task_spawn_child_work88(task, (void *)effect_spark_fall_back_task), entity, 5);
            seed_sub(task_spawn_child_work88(task, (void *)effect_spark_fall_forward_task), entity, 6);
            seed_sub(task_spawn_child_work88(task, (void *)effect_spark_fall_forward_task), entity, 6);
            o = task_spawn_child_work88(task, (void *)effect_flash_expand_task);
            {
                void *sub = MP(o, 8);
                M32(sub, 8)  = M32(entity, 8);
                M32(sub, 12) = M32(entity, 12);
                M32(sub, 16) = M32(entity, 16);
            }
        } else {
            game_random_next();
            game_random_next();
            game_random_next();
            game_random_next();
        }

        display_mark_channel_bits();
        if (G32(DH_PLAY_SESSION_MODE_ADDR) == 1 && G32(DH_VIEW_REQUEST_PRIORITY_ADDR) >= 1) {
            void *tgt = MP(GP(DH_VIEW_TASK_ADDR), 8);
            G32(DH_VIEW_REQUEST_PRIORITY_ADDR) = 1;
            M8(GP(DH_VIEW_TASK_ADDR), 0xd) = 1;
            M16(tgt, 0xae) = 0;
            M16(tgt, 0xb0) = 0;
            M16(tgt, 0xb2) = 0;
        }

    after_spawn:
        actor_publish_linked_slot_value(entity);
        M8(entity, 0x5d) = 0;
    }

    if (grid_find_overlap_type_zero((u8 *)entity + 8, g_06088064) == 0) {
        s16 old = M16(entity, 0x58);
        M16(entity, 0x58) = (s16)(old - 1);
        if (old > 0) {
            M32(entity, 0x4c) = M32(entity, 8);
            M32(entity, 0x50) = M32(entity, 12);
            M32(entity, 0x54) = M32(entity, 16);
            M32(entity, 8) = M32(entity, 8) + M32(entity, 20);
            collision_queue_actor(entity);
            sprite_draw_billboard(entity, 0x20000, 0x10000, 1);
            return;
        }
    }
    TASK_S32(task, handler) = 0;
}
