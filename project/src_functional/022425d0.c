/* projectile_tick_armed_bomb [0x022425d0]: Slot bit and task states control spawn, arming and thrown explosive sequence. */
#include "dh_work.h"
#include "dh.h"
/* 0x022425d0  grenade/thrown-bomb task stepper. arg0 = task wrapper,
 * inner = MP(task,8) is the projectile object; mask = M32(inner,0x68) is this
 * projectile's slot bit. State machine on M8(task,0x10):
 *   0 spawn: heading 0x479, grab a collision slot (func_02228106 -> unk34),
 *     unk38=0x80000, snapshot x/y into unk60/unk64, compute slot bit
 *     unk68 = 1 << task->unkF (playerIdx) via the shift helper; advance.
 *   1 armed-wait: when this slot's bit is live in g_06098580, set the 0xC fuse,
 *     register a debris record in g_06098838 (keyed by unk34), advance.
 *   2/4 bounce countdown: tick fuse; on 0, seed the roll (unk6C=-0x1000,
 *     unk70=0x10000), (if active) wake the shared record g_0609882c, play sound
 *     0xA01205FF, advance, then fall into the roll step.
 *   3/5 roll step: alternate frames add/settle unk6C/unk70, decaying unk70 by >>2.
 *   6 detonate: clear this bit in g_06098580, zero velocity, wake the record,
 *     sound 0xA01305FF, set unk3C bit0, run the blast func_022434ae(.., g_0608db14),
 *     advance.
 *   7 debris-arc: while rising (unk64<0) integrate gravity and spin; on apex
 *     install the settle handler func_022462d0 into task->unk0.
 * Always tail-calls func_02246282(inner).
 *
 * bounds: [0x2425d0, 0x242888+delay); the 0x440 fn.extent over-swallows the next
 *   function (split at the braf-tail to func_02246282 at 0x242888).
 * notes: func_02228106 takes no args; sound_queue_push takes only the command;
 *   func_0224a360(v,n) is the r0<<r1 shift helper (here 1 << playerIdx).
 * fidelity: HIGH - field widths and masks verified against fn.py --asm. */
extern s32  g_scene_event_flags;        /* live-slot bitmask */
extern s32  g_play_session_mode;        /* feature-enable flag */
extern s32  g_view_request_priority;        /* shared-record use counter */
extern void *g_view_task;       /* shared record (+8 obj, +0xD flag) */
extern u8   g_06098838[];      /* debris records, 8-byte stride */
extern s32  g_sound_effects_enabled;        /* sound-enable gate */
extern s16  g_0608db14[];      /* blast parameter table */

s32  actor_allocate_effect_slot(void);                         /* 0x02228106 collision-slot grab */
s32  shc_shift_left(s32 value, s32 count);         /* 0x0224a360 value << count */
void sound_queue_push(s32 cmd);                      /* 0x02215f6c sound_queue_push */
void projectile_spawn_velocity_burst(void *task, void *actor, void *tbl); /* 0x022434ae */
void effect_tick_blink_countdown(void *task);                   /* 0x022462d0 settle handler */
void actor_draw_transformed(void *actor);                  /* 0x02246282 shared tail */

void projectile_tick_armed_bomb(void *task) {
    void *inner = TASK_PTR(task, work);
    s32 mask = M32(inner, 0x68);

    switch (TASK_S8(task, state)) {
    case 3:
    case 5:
    block_roll: {
        s32 t = M32(inner, 0x5C) + 1;
        M32(inner, 0x5C) = t;
        if ((t - 1) & 1) {
            M32(inner, 0x08) = M32(inner, 0x60) + M32(inner, 0x6C);
            M32(inner, 0x10) = M32(inner, 0x64) + M32(inner, 0x70);
        } else {
            s32 v;
            M32(inner, 0x08) = M32(inner, 0x60);
            M32(inner, 0x10) = M32(inner, 0x64);
            M32(inner, 0x6C) = -M32(inner, 0x6C);
            v = -(M32(inner, 0x70) >> 2);
            M32(inner, 0x70) = v;
            if (v == 0) {
                TASK_S8(task, state) += 1;
            }
        }
        break;
    }
    case 6: {
        g_scene_event_flags &= ~mask;
        M32(inner, 0x60) = 0;
        M32(inner, 0x64) = -0x10000;
        if (g_play_session_mode == 1 && g_view_request_priority >= 1) {
            void *obj = TASK_PTR(g_view_task, work);
            g_view_request_priority = 1;
            M8(g_view_task, 0xD) = 1;
            M16(obj, 0xAE) = 0;
            M16(obj, 0xB0) = 0;
            M16(obj, 0xB2) = 0;
        }
        if (g_sound_effects_enabled != 0) {
            sound_queue_push(0xA01305FF);
        }
        M32(inner, 0x3C) |= 1;
        projectile_spawn_velocity_burst(task, inner, g_0608db14);
        TASK_S8(task, state) += 1;
        break;
    }
    case 7:
        if (M32(inner, 0x64) < 0) {
            M32(inner, 0x0C) += M32(inner, 0x60);
            M32(inner, 0x60) -= 0x400;
            M32(inner, 0x10) += M32(inner, 0x64);
            M32(inner, 0x64) += 0x1000;
            M16(inner, 0x24) += 0x400;
            M16(inner, 0x26) -= 0x800;
            M16(inner, 0x26) += 0x400;
        } else {
            M16(inner, 0x24) = 0x4000;
            M32(inner, 0x5C) = 0x20;
            M32(inner, 0x3C) &= ~1;
            TASK_PTR(task, handler) = (void *)effect_tick_blink_countdown;
        }
        break;
    case 0:
        M16(inner, 0x30) = 0x479;
        M8(inner, 0x34) = (s8)actor_allocate_effect_slot();
        M32(inner, 0x38) = 0x80000;
        M32(inner, 0x60) = M32(inner, 0x08);
        M32(inner, 0x64) = M32(inner, 0x10);
        M32(inner, 0x68) = shc_shift_left(1, M8(task, 0x0F));
        TASK_S8(task, state) += 1;
        break;
    case 1:
        if (mask & g_scene_event_flags) {
            M32(inner, 0x58) = 0xC;
            if (M8(inner, 0x34) < 8 && (M8(task, 0x0F) + 1) < 8) {
                u8 *rec = g_06098838 + M8(inner, 0x34) * 8;
                M32(rec, 0) = M8(task, 0x0F) + 1;
                M32(rec, 4) = 0x12C;
            }
            TASK_S8(task, state) += 1;
        }
        break;
    case 2:
    case 4: {
        s32 t = M32(inner, 0x58) - 1;
        M32(inner, 0x58) = t;
        if ((t + 1) > 0) {
            /* still counting down */
        } else {
            M32(inner, 0x6C) = -0x1000;
            M32(inner, 0x70) = 0x10000;
            M32(inner, 0x5C) = 0;
            if (g_play_session_mode == 1 && g_view_request_priority >= 1) {
                void *obj = TASK_PTR(g_view_task, work);
                g_view_request_priority = 1;
                M8(g_view_task, 0xD) = 1;
                M16(obj, 0xAE) = 0;
                M16(obj, 0xB0) = 0;
                M16(obj, 0xB2) = 0;
            }
            if (g_sound_effects_enabled != 0) {
                sound_queue_push(0xA01205FF);
            }
            TASK_S8(task, state) += 1;
            goto block_roll;
        }
        break;
    }
    }

    actor_draw_transformed(inner);
}
