/* enemy_update_scripted_response [0x02248318]: Chooses and advances response configurations under hit, script and pair-lock gates. */
#include "dh.h"
/* 0x02248318  actor state machine; ABI (r4=task, r5=entity).
 *   if entity[0x4c] != 0 -> func_022489d0(entity)
 *   else switch (entity[0xd8]):
 *     case 0: if (actor_script_step(entity) == 0) {
 *               rec = MP(entity,0x184); entity[0x41]=1;
 *               if (func_02245f48(entity) != 0) { actor_init_from_cfg(entity, M32(rec,4));  entity[0xd8]=1; }
 *               else                            { actor_init_from_cfg(entity, M32(rec,8));  entity[0xd8]=2; } }
 *     case 1: if (actor_script_step(entity) == 0) {
 *               if (func_02245eb8(entity) != 0) func_022499a6(entity);
 *               else func_022469f2(entity); }
 *     case 2: if (actor_script_step(entity) == 0) func_022469f2(entity);
 *   tail: actor_anim_frame_update(task, entity).
 * fidelity: HIGH - hand-traced. */
extern void actor_enter_hit_reaction(void *entity);
extern s32  actor_script_step(void *actor);
extern s32  actor_scan_pair_slots_simple(void *a);
extern s32  enemy_test_player_pair_lock(void *a);
extern void enemy_enter_optional_pose(void *entity);
extern void enemy_reset_idle_or_walk(void *obj);
extern void actor_init_from_cfg(void *actor, s16 *src);
extern s32  actor_anim_frame_update(s32 task, void *entity);

s32 enemy_update_scripted_response(s32 task, void *entity) {
    if (M8(entity, 0x4C) != 0) {
        actor_enter_hit_reaction(entity);
    } else {
        switch (M32(entity, 0xD8)) {
        case 0:
            if (actor_script_step(entity) == 0) {
                void *rec = MP(entity, 0x184);
                M8(entity, 0x41) = 1;
                if (actor_scan_pair_slots_simple(entity) != 0) {
                    actor_init_from_cfg(entity, (s16 *)M32(rec, 4));
                    M32(entity, 0xD8) = 1;
                } else {
                    actor_init_from_cfg(entity, (s16 *)M32(rec, 8));
                    M32(entity, 0xD8) = 2;
                }
            }
            break;
        case 1:
            if (actor_script_step(entity) == 0) {
                if (enemy_test_player_pair_lock(entity) != 0) {
                    enemy_enter_optional_pose(entity);
                } else {
                    enemy_reset_idle_or_walk(entity);
                }
            }
            break;
        case 2:
            if (actor_script_step(entity) == 0) {
                enemy_reset_idle_or_walk(entity);
            }
            break;
        default:
            break;
        }
    }
    return actor_anim_frame_update(task, entity);
}
