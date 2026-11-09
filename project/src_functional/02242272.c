/* actor_pose_hold_state [0x02242272]: Existing map corroborated by source: task-handler: timed pose/hit-reaction hold (state unkD4 0..3); name MED */
#include "dh_work.h"
#include "dh.h"
/* 0x02242272  "stagger/getup" task stepper for an actor. arg0 = task wrapper,
 * inner = MP(task,8) is the actor. State machine on M32(inner,0xD4) (0..3):
 *   0 arm: stamp playerIdx into g_06097414, snapshot home coords (unkEC/F0/F4),
 *     set the 0xB4-frame timer, advance to 1.
 *   1 down: tick timer; while >0, if the hit flag unk48 is set play getup
 *     (func_02242226) and go to 2; on expiry rise (unkE0|=0x40000, heading 0x471)
 *     and go to 3.
 *   2 getup-hold: tick timer; re-trigger getup on unk48, else when may-act
 *     (unk54&2) drop the 0x400 bit, heading 0x46F, back to 1.
 *   3 recover: when may-act, load the recovery script (actor_script_begin with
 *     g_0608daf0 if unkE0&0x400000 else g_0608dafc), reset accel/flags and install
 *     the next task handler func_0224680c into task->unk0.
 * Always: animation_update(task, inner); func_0222ca98(inner) (shared tail).
 *
 * bounds: [0x242272, 0x2423da); the 0x35e fn.extent over-swallows func_02242410
 *   and func_022424c4 (split at the braf-tail to func_0222ca98 at 0x2423d6).
 * fidelity: HIGH - verified vs m2c/--asm. */
extern s8   g_06097414;        /* current-actor playerIdx latch */
extern s16  g_0608daf0[];      /* recovery script A */
extern s16  g_0608dafc[];      /* recovery script B */

void enemy_restore_saved_pose(void *actor);                 /* 0x02242226 */
void enemy_approach_entry_task(void *task);                  /* 0x0224680c (next handler) */
void actor_script_begin(void *actor, void *cfg);      /* 0x02244aa8 actor_script_begin */
s32  actor_anim_frame_update(void *task, void *actor);     /* 0x0222dc2c animation_update */
void draw_actor_hier_shadowed_entry(void *actor);                 /* 0x0222ca98 shared tail */

void actor_pose_hold_state(void *task) {
    void *inner = TASK_PTR(task, work);

    switch (M32(inner, 0xD4)) {
    case 0:
        g_06097414 = M8(inner, 0x34);
        M32(inner, 0xEC) = M32(inner, 0x08);
        M32(inner, 0xF0) = M32(inner, 0x0C);
        M32(inner, 0xF4) = M32(inner, 0x10);
        M32(inner, 0xC4) = 0xB4;
        M32(inner, 0xD4) += 1;
        /* fallthrough */
    case 1: {
        s32 t = M32(inner, 0xC4) - 1;
        M32(inner, 0xC4) = t;
        if ((t + 1) > 0) {
            if (M8(inner, 0x48) != 0) {
                enemy_restore_saved_pose(inner);
                M32(inner, 0xD4) = 2;
            }
        } else {
            g_06097414 = 0;
            M32(inner, 0xE0) |= 0x40000;
            M8(inner, 0x41) = 1;
            M16(inner, 0x5C) = 0x471;
            M32(inner, 0xD4) = DH_ENEMY_BEHAVIOR_DASH;
        }
        break;
    }
    case 2:
        M32(inner, 0xC4) -= 1;
        if (M8(inner, 0x48) != 0) {
            enemy_restore_saved_pose(inner);
        } else if (((u16)MU16(inner, 0x54)) & 2) {
            M32(inner, 0xE0) &= ~0x400;
            M8(inner, 0x41) = 1;
            M16(inner, 0x5C) = 0x46F;
            M32(inner, 0xD4) = 1;
        }
        break;
    case 3:
        if (((u16)MU16(inner, 0x54)) & 2) {
            void *cfg = (M32(inner, 0xE0) & 0x400000) ? (void *)g_0608daf0
                                                      : (void *)g_0608dafc;
            actor_script_begin(inner, cfg);
            M32(inner, 0x84) = 0x201;
            M32(inner, 0x88) = 0;
            M32(inner, 0xE0) = 1;
            M32(inner, 0xBC) = M32(inner, 0x140);
            M32(inner, 0xEC) = 0;
            M32(inner, 0xF0) = 0;
            M32(inner, 0xF4) = 0;
            M32(inner, 0xD4) = 0;
            TASK_PTR(task, handler) = (void *)enemy_approach_entry_task;
        }
        break;
    }

    actor_anim_frame_update(task, inner);
    draw_actor_hier_shadowed_entry(inner);
}
