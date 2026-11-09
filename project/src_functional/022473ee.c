/* enemy_begin_scripted_dash [0x022473ee]: Selects dash or recovery and initializes velocity, duration and descriptor fields. */
#include "dh_work.h"
#include "dh.h"
/* 0x022473ee  begin a scripted dash/charge move for an actor. arg0 = actor,
 * arg1 = move parameter record. Dispatch on func_022460c8(actor):
 *   0/1 start: set state unkD4=3, clear unkD8, speed cap unk84=0x8000, flags
 *     unkE0=0xC0000 (plus 0x08000000 when the record's unkC bit 0x4000 is set),
 *     unk108=1, face flag unk41=1, heading unk5C = record.unkC & 0x1FFF; zero the
 *     motion accumulators unk14/18/1C. Pick the X velocity unkEC: if the record
 *     gives an explicit speed (unk0) use it (negated when facing left, unk44), else
 *     home toward the target slot g_060995b0[unk128]->unk8 (or self) with
 *     (dx>>5, dy>>5). Store duration/params unkF4/unkF0/unk104 and the record ptr
 *     unk100.
 *   2 -> func_02247a16(actor).  3 -> func_02247a3e(actor).
 *
 * bounds: [0x022473ee, 0x2475a8); the 0x678 fn.extent over-swallows func_022475a8,
 *   func_02247924, func_02247a16 and func_02247a3e (cmp/eq dispatch, not a table).
 * fidelity: HIGH - verified vs fn.py --asm; >>5 are arithmetic shifts. */
extern s32  g_actor_slot_ids[];      /* per-slot active flag */
extern void *g_actor_slot_tasks_legacy[];     /* per-slot anchor pointers; deref +8 -> actor */

s32  enemy_classify_linked_effect(void *actor);                 /* 0x022460c8 move-mode select */
void enemy_enter_dash_recovery_a(void *actor);                 /* 0x02247a16 */
void enemy_enter_dash_recovery_b(void *actor);                 /* 0x02247a3e */

void enemy_begin_scripted_dash(void *actor, void *rec) {
    switch (enemy_classify_linked_effect(actor)) {
    case 0:
    case 1: {
        s16 word = M16(rec, 0xC);
        s32 speed;
        s32 vy = 0;

        ACTOR_M32(actor, behavior_state) = DH_ENEMY_BEHAVIOR_DASH;
        ACTOR_M32(actor, behavior_phase) = 0;
        ACTOR_M32(actor, action_category) = 0x8000;
        ACTOR_M32(actor, state_flags) = 0xC0000;
        ACTOR_M32(actor, action_phase) = 1;
        ACTOR_M8(actor, animation_control) = DH_ANIMATION_CONTROL_RESTART;
        ACTOR_M16(actor, animation_requested) = (s16)(0x1FFF & word);
        if (((u16)word) & 0x4000) {
            ACTOR_M32(actor, state_flags) |= 0x08000000;
        }
        ACTOR_M32(actor, movement_x) = 0;
        ACTOR_M32(actor, movement_y) = 0;
        ACTOR_M32(actor, movement_z) = 0;

        speed = M32(rec, 0x00);
        if (speed != 0) {
            M32(actor, 0xEC) = (ACTOR_M8(actor, facing_right) != 0) ? -speed : speed;
        } else {
            s32 slot = ACTOR_M32(actor, player_target_slot);
            void *target = (g_actor_slot_ids[slot] != 0) ? TASK_PTR(g_actor_slot_tasks_legacy[slot], work) : actor;
            M32(actor, 0xEC) = (M32(target, 0x08) - ACTOR_M32(actor, position_x)) >> 5;
            vy = (M32(target, 0x10) - ACTOR_M32(actor, position_z)) >> 5;
        }
        M32(actor, 0xF4) = vy;
        M32(actor, 0xF0) = M32(rec, 0x04);
        ACTOR_M32(actor, queued_attack_slot) = M32(rec, 0x08);
        ACTOR_MP(actor, queued_attack_kind) = rec;
        return;
    }
    case 2:
        enemy_enter_dash_recovery_a(actor);
        return;
    case 3:
        enemy_enter_dash_recovery_b(actor);
        return;
    }
}
