#include "dh_work.h"
/* enemy_update_action_commands [0x02248bb8]: Runs the double-dispatch action command machine and advances animation. */
#include "dh.h"
/* 0x02248bb8  actor_command_dispatch - double jump-table command dispatcher.
 * ABI (r4=task, r5=entity). animation_update (actor_anim_frame_update 0x0222dc2c) is called
 * through r11 (jsr @r11) and actor_init_from_cfg through r9 (jsr @r9); both resolved via
 * ptr_resolve (mova PIC anchors). Prologue latches r5 = (s8)entity[0x34] for reuse.
 *
 * Top switch on entity[0xd8]:
 *   state 0: animation_update; if (entity[0x54]&2) { entity[0x64]=0; entity[0xd8]=1; }
 *   state 1: the command dispatch (below)
 *   default: return
 *
 * State 1 returns immediately if entity[0x4c]!=0 (hit wait). Otherwise it splits on
 * entity[0x56]:
 *   entity[0x56] <= 0 : cmp-chain on cmd=entity[0x88]
 *       0x3b/0x3c -> combo-table block (falls through to the 0x4a block)
 *       0x4a      -> enter-state-3 block
 *       0x44      -> set hit byte + enter state 1
 *       0x49      -> func_0223ffcc -> entity[0xd8]
 *       0x42/0x43 -> despawn (func_02249cda) + actor_init_from_cfg(table)
 *       0x3d/dflt -> tail func_02249cda
 *   entity[0x56] >  0 : runtime braf table @0x248db8 (35 entries, braf PC 0x248db2,
 *       target = 0x248db6 + (s16)tbl[cmd-0x28]). Table decoded DIRECTLY from ROM
 *       (the deferral note's cmd->target map had the command indices shifted; the
 *       raw-ROM decode below is authoritative and was cross-checked with the block
 *       addresses). cmd=idx+0x28:
 *         0x28                 -> 0x248e9e
 *         0x30                 -> 0x248f34
 *         0x3b,0x3c,0x3f       -> 0x248dfe
 *         0x3d,0x40,0x41       -> 0x248e30
 *         0x42 -> 0x248ed2  0x43 -> 0x248eea  0x44 -> 0x248e4e
 *         0x49 -> 0x248e66  0x4a -> 0x248e7e
 *         every other in-range cmd -> 0x248f72 (bare return)
 *
 * Combo sub-tables (block 0x248c84): 0x060540e8 (s8[] indexed by entity[0x5e]),
 * 0x06054080 (8-byte records), 0x06053f04 (s16[]), 0x060e0ddc (enable flag).
 * bsrf at 0x248cb2 has a non-nop delay slot (or r3,r4) so it is resolved by hand to
 * sound_queue_push. All computed branches resolved with braf_resolve; the 0x248db8
 * table decoded + verified from ROM bytes. fidelity: HIGH - hand-traced vs fn.py.
 */
extern s32  actor_anim_frame_update(void *task, void *entity);   /* animation_update */
extern void actor_init_from_cfg(void *entity, s32 arg);
extern void sound_queue_push(s32 arg);
extern s32  actor_dispatch_action_and_finish(void *task, void *entity);   /* -> new state */
extern void enemy_enter_despawn(void *task, void *entity);   /* actor_enter_despawn */
extern void enemy_enter_timed_reaction(void *entity);               /* actor_enter_state4 */
extern s32  enemy_enter_guard(void *entity);
extern void enemy_reset_idle_or_walk(void *entity);
extern s32  game_random_next(void);
extern void actor_resolve_facing(void *entity);

void enemy_update_action_commands(void *task, void *entity) {
    switch (ACTOR_M32(entity, behavior_phase)) {
    case 0:
        actor_anim_frame_update(task, entity);
        if (ACTOR_MU16(entity, animation_events) & 2) {
            MU16(entity, 0x64) = 0;
            ACTOR_M32(entity, behavior_phase) = 1;
        }
        return;

    case 1: {
        s32 cmd;
        if (ACTOR_M8(entity, hit_pending) != 0) {
            return;
        }
        cmd = ACTOR_M32(entity, action_state);

        if (ACTOR_M16(entity, health) > 0) {
            /* runtime braf table @0x248db8 */
            if ((u32)(cmd - 0x28) >= 35) {
                return;
            }
            switch (cmd) {
            case 0x28:                          /* 0x248e9e */
                ACTOR_M32(entity, behavior_state) = DH_ENEMY_BEHAVIOR_POSE;
                ACTOR_M32(entity, action_category) = 0x200;
                ACTOR_M32(entity, action_state) = 0;
                ACTOR_M32(entity, action_phase) = 6;
                ACTOR_M8(entity, animation_control) = DH_ANIMATION_CONTROL_RESTART;
                if (game_random_next() >= 0) {
                    ACTOR_MU16(entity, animation_requested) = 0x190;
                } else {
                    ACTOR_MU16(entity, animation_requested) = ACTOR_M16(entity, state8_animation);
                }
                actor_anim_frame_update(task, entity);
                return;

            case 0x30:                          /* 0x248f34 */
                actor_resolve_facing(entity);
                ACTOR_M32(entity, behavior_state) = DH_ENEMY_BEHAVIOR_POSE;
                ACTOR_M32(entity, action_category) = 0x200;
                ACTOR_M32(entity, action_state) = 0;
                ACTOR_M32(entity, action_phase) = 6;
                ACTOR_M8(entity, animation_control) = DH_ANIMATION_CONTROL_RESTART;
                if (game_random_next() >= 0) {
                    ACTOR_MU16(entity, animation_requested) = 0x85;
                } else {
                    ACTOR_MU16(entity, animation_requested) = ACTOR_M16(entity, state2_animation);
                }
                actor_anim_frame_update(task, entity);
                return;

            case 0x3B:                          /* 0x248dfe */
            case 0x3C:
            case 0x3F:
                ACTOR_MU16(entity, angle_y) += 0x8000;
                M32(entity, 0xBC) = M32(entity, 0x140);
                ACTOR_M8(entity, hit_shake_ticks) = 10;
                if (enemy_enter_guard(entity) != 0) {
                    M32(entity, 0xC4) = 0;
                } else {
                    enemy_reset_idle_or_walk(entity);
                }
                actor_anim_frame_update(task, entity);
                return;

            case 0x3D:                          /* 0x248e30 */
            case 0x40:
            case 0x41:
                M32(entity, 0xBC) = M32(entity, 0x140);
                enemy_enter_timed_reaction(entity);
                return;

            case 0x42:                          /* 0x248ed2 */
                ACTOR_MU16(entity, health) = 0;
                enemy_enter_despawn(task, entity);
                ACTOR_M32(entity, behavior_state) = 0;
                actor_init_from_cfg(entity, (s32)0x06090110);
                actor_anim_frame_update(task, entity);
                return;

            case 0x43:                          /* 0x248eea */
                ACTOR_MU16(entity, health) = 0;
                enemy_enter_despawn(task, entity);
                ACTOR_M32(entity, behavior_state) = 0;
                actor_init_from_cfg(entity, (s32)0x06090114);
                actor_anim_frame_update(task, entity);
                return;

            case 0x44:                          /* 0x248e4e */
                ACTOR_M8(entity, hit_pending) = M8(entity, 0x67);
                ACTOR_M32(entity, action_state) = 0;
                ACTOR_M32(entity, behavior_state) = 12;
                ACTOR_M32(entity, behavior_phase) = 1;
                actor_anim_frame_update(task, entity);
                return;

            case 0x49:                          /* 0x248e66 */
                ACTOR_M8(entity, hit_source_slot) = ACTOR_M8(entity, slot_id);
                ACTOR_M32(entity, behavior_state) = DH_ENEMY_BEHAVIOR_REACTION;
                ACTOR_M32(entity, behavior_phase) = actor_dispatch_action_and_finish(task, entity);
                return;

            case 0x4A:                          /* 0x248e7e */
                ACTOR_M32(entity, behavior_state) = DH_ENEMY_BEHAVIOR_REACTION;
                ACTOR_M32(entity, behavior_phase) = 3;
                ACTOR_M32(entity, state_flags) = 0x40400;
                ACTOR_M32(entity, action_phase) = 1;
                M32(entity, 0xBC) = M32(entity, 0x140);
                actor_anim_frame_update(task, entity);
                return;

            default:                            /* 0x248f72 */
                return;
            }
        } else {
            /* cmp-chain path (entity[0x56] <= 0) */
            switch (cmd) {
            case 0x3B:                          /* 0x248c84 */
            case 0x3C: {
                s16 sel = ACTOR_M16(entity, model_index);
                s8  ta  = ((s8 *)(uintptr_t)0x060540E8)[sel];
                s8 *rec = (s8 *)(uintptr_t)(0x06054080 + (s32)ta * 8);
                if (*(s32 *)(uintptr_t)DH_SOUND_EFFECTS_ENABLED_ADDR != 0) {
                    s16 tb = ((s16 *)(uintptr_t)0x06053F04)[(u8)rec[1]];
                    sound_queue_push((s32)(((u32)((s32)tb << 8)) | DH_SOUND_CUE_BASE_U));
                }
                ACTOR_MU16(entity, angle_y) += 0x8000;   /* 0x248cb6 */
                ACTOR_M8(entity, animation_control) = DH_ANIMATION_CONTROL_RESTART;
                ACTOR_MU16(entity, animation_requested) = 0x13D;
            }
            /* fall through */                   /* 0x248cca */
            case 0x4A:
                ACTOR_M32(entity, behavior_state) = DH_ENEMY_BEHAVIOR_REACTION;
                ACTOR_M32(entity, behavior_phase) = 3;
                ACTOR_M32(entity, state_flags) = 0x48C00;
                ACTOR_M32(entity, action_phase) = 1;
                M32(entity, 0xBC) = M32(entity, 0x140);
                actor_anim_frame_update(task, entity);    /* 0x248d88 */
                return;

            case 0x44:                          /* 0x248cea */
                ACTOR_M8(entity, hit_pending) = M8(entity, 0x67);
                actor_anim_frame_update(task, entity);
                ACTOR_M32(entity, behavior_state) = 12;
                ACTOR_M32(entity, behavior_phase) = 1;
                return;

            case 0x49:                          /* 0x248d04 */
                ACTOR_M8(entity, hit_source_slot) = ACTOR_M8(entity, slot_id);
                ACTOR_M32(entity, behavior_state) = DH_ENEMY_BEHAVIOR_REACTION;
                ACTOR_M32(entity, behavior_phase) = actor_dispatch_action_and_finish(task, entity);
                return;

            case 0x42:                          /* 0x248d64 */
                enemy_enter_despawn(task, entity);
                ACTOR_M32(entity, behavior_state) = 0;
                actor_init_from_cfg(entity, (s32)0x06090110);
                actor_anim_frame_update(task, entity);    /* 0x248d88 */
                return;

            case 0x43:                          /* 0x248d78 */
                enemy_enter_despawn(task, entity);
                ACTOR_M32(entity, behavior_state) = 0;
                actor_init_from_cfg(entity, (s32)0x06090114);
                actor_anim_frame_update(task, entity);    /* 0x248d88 */
                return;

            case 0x3D:                          /* 0x248d18 */
            default:
                enemy_enter_despawn(task, entity);
                return;
            }
        }
    }

    default:
        return;
    }
}
