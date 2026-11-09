/* projectile_tick_arc_to_burst [0x02230cdc]: Gravity arc expires on collision, target latch or timer then selects burst handler. */
#include "dh_work.h"
#include "dh.h"
/* 0x02230cdc - action_setup_b "next handler": lifetime + gravity-arc projectile update.
 * entity = task->unk8. Retire when entity->unk5D != 0, or grid_overlap_find_type0
 * (entity+8, g_06088064) hits, or --entity->unk58 reaches 0:
 *   task->unk0 = func_02230e34 (next state); entity->unk30 = 0x187; entity->unk32 = 0;
 *   sprite_submit func_022153f6(entity, 0x80000, 1);
 *   if (g_06098670==1 && g_06098830>=1): raise late-join on *g_0609882c (unkD=1,
 *     sub unkAE/B0/B2=0, g_06098830=1);
 *   if (g_06098670==1): g_06098834 |= 1; (*g_0609882c)->unk8->unkAA = 10;
 *   spawn child func_0220fece(task, func_02238e94).
 * Otherwise (alive) if (g_060987bc & 1): spawn child func_0220feec(task, func_02231336),
 *   copy entity pos into child->unk8, child->unk32 = 0, child->unk64 = 0x40000.
 * Then physics (shared): snapshot pos to unk4C/50/54; unkC += unk18; unk8 += unk14;
 *   unk18 -= 0x100; unk14 += unk1C; drawlist_prepend(entity);
 *   tail sprite_renderer(entity, 0x20000, 0x10000, 1).
 * fidelity: HIGH - m2c over-swallowed the next-state handler; bounded to 0x230e34
 * (next real entry); tail braf 0x2154fc modeled as a call. All bsrf resolved
 * (0x217ae4,0x2153f6,0x20fece,0x20feec,0x22eea2); handlers via mova+add. unk18 -0x100
 * is the sign-extended word 0xff00. */
#define G32(a) (*(s32 *)(uintptr_t)(a))
#define GP(a)  (*(void **)(uintptr_t)(a))

extern s32   grid_find_overlap_type_zero(void *pos, void *grid);        /* grid_overlap_find_type0 */
extern void  sprite_draw_child(void *entity, s32 a, s32 b);   /* sprite_submit1 */
extern void *task_spawn_current_work6c(void *task, void *handler);   /* spawn_child_task */
extern void *task_spawn_child_work88(void *task, void *handler);   /* spawn_obj_a */
extern void  collision_queue_actor(void *entity);                 /* drawlist_prepend */
extern void  sprite_draw_billboard(void *entity, s32 sx, s32 sy, s32 flag); /* sprite_renderer */
extern void  effect_tick_explosion_debris(void *task);                  /* next-state handler */
extern void  effect_tick_expanding_frames(void *task);                  /* alive-spawn handler */
extern void  obj_anim_step(void *task);                  /* obj_anim_step handler */
extern u8    g_06088064[];

void projectile_tick_arc_to_burst(void *task) {
    void *entity = TASK_PTR(task, work);
    int retire;

    if (M8(entity, 0x5d) != 0) {
        retire = 1;
    } else if (grid_find_overlap_type_zero((u8 *)entity + 8, g_06088064) != 0) {
        retire = 1;
    } else {
        s16 n = (s16)(M16(entity, 0x58) - 1);
        M16(entity, 0x58) = n;
        retire = (n == 0);
    }

    if (retire) {
        TASK_PTR(task, handler) = (void *)effect_tick_explosion_debris;
        M16(entity, 0x30) = 0x187;
        M16(entity, 0x32) = 0;
        sprite_draw_child(entity, 0x80000, 1);
        if (G32(DH_PLAY_SESSION_MODE_ADDR) == 1 && G32(DH_VIEW_REQUEST_PRIORITY_ADDR) >= 1) {
            void *tgt = MP(GP(DH_VIEW_TASK_ADDR), 8);
            G32(DH_VIEW_REQUEST_PRIORITY_ADDR) = 1;
            M8(GP(DH_VIEW_TASK_ADDR), 0xd) = 1;
            M16(tgt, 0xae) = 0;
            M16(tgt, 0xb0) = 0;
            M16(tgt, 0xb2) = 0;
        }
        if (G32(DH_PLAY_SESSION_MODE_ADDR) == 1) {
            G32(0x06098834) |= 1;
            M16(MP(GP(DH_VIEW_TASK_ADDR), 8), 0xaa) = 10;
        }
        task_spawn_current_work6c(task, (void *)obj_anim_step);
    } else {
        if (G32(DH_FRAME_TICK_ADDR) & 1) {
            void *sub = MP(task_spawn_child_work88(task, (void *)effect_tick_expanding_frames), 8);
            M32(sub, 8)  = M32(entity, 8);
            M32(sub, 12) = M32(entity, 12);
            M32(sub, 16) = M32(entity, 16);
            M16(sub, 0x32) = 0;
            M32(sub, 0x64) = 0x40000;
        }
    }

    M32(entity, 0x4c) = M32(entity, 8);
    M32(entity, 0x50) = M32(entity, 12);
    M32(entity, 0x54) = M32(entity, 16);
    M32(entity, 12) = M32(entity, 12) + M32(entity, 24);
    M32(entity, 8)  = M32(entity, 8)  + M32(entity, 20);
    M32(entity, 24) = M32(entity, 24) - 0x100;
    M32(entity, 20) = M32(entity, 20) + M32(entity, 28);
    collision_queue_actor(entity);
    sprite_draw_billboard(entity, 0x20000, 0x10000, 1);
}
