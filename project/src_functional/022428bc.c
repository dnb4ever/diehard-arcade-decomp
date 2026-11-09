/* effect_tick_armed_puff [0x022428bc]: Initializes slot mask/scale, waits for arm gate and advances puff draw states. */
#include "dh_work.h"
#include "dh.h"
/* 0x022428bc  smoke/shrapnel puff task stepper + per-frame draw. arg0 = task
 * wrapper, inner = MP(task,8) is the puff object. State machine on M8(task,0x10):
 *   0 spawn: heading 0x478, grab a collision slot (func_02228106 -> unk34),
 *     unk38=0x80000, start scale unk60=0x10000, slot bit unk68 = 1 << playerIdx;
 *     advance.
 *   1 wait-armed: when this bit is live in g_06098580, set rise velocity
 *     (unk64=-0x800, unk6C=-0x200), fuse unk58=2, register a debris record in
 *     g_06098838 (keyed by unk34); advance.
 *   2 rise/settle: integrate scale by velocity (unk60 += unk64, unk64 += unk6C);
 *     once falling (unk64<0) and small (unk60<0x2000), damp (unk64 = -(unk64>>1))
 *     and tick the fuse; on expiry go to state 3 (idle).
 *   3 idle: nothing.
 * Every frame: push the matrix stack, build model matrix from pos (inner+8),
 * angles (inner+0x24) and uniform scale unk60, then draw via func_02215a2c.
 *
 * bounds: [0x2428bc, 0x242a10); split out of func_022425d0's over-swallowed
 *   extent (a literal pool at 0x24288c precedes this prologue; 0x242a10 begins
 *   func_02242a10).
 * notes: func_02228106 takes no args; func_0224a360(v,n) = v<<n (1 << playerIdx).
 * fidelity: HIGH - verified against fn.py --asm. */
extern s32  g_scene_event_flags;        /* live-slot bitmask */
extern u8   g_06098838[];      /* debris records, 8-byte stride */
extern s32  g_matrix_stack_cursor;        /* matrix-stack pointer */

s32  actor_allocate_effect_slot(void);                         /* 0x02228106 collision-slot grab */
s32  shc_shift_left(s32 value, s32 count);         /* 0x0224a360 value << count */
void mtx_load_abc(void);                         /* 0x022149aa mtx push/identity */
void mtx_apply_vec_at0(void *vec);                    /* 0x02214a50 apply matrix to vec */
void mtx_rotate_zyx(void *angles);                 /* 0x022149e4 mtx_rotate_zyx */
void matrix_scale_columns(s32 x, s32 y, s32 z);          /* 0x02213ce4 mtx_scale_cols */
void model_submit_culled(void *entity, s32 flag);       /* 0x02215a2c draw model */

void effect_tick_armed_puff(void *task) {
    void *inner = TASK_PTR(task, work);

    switch (TASK_S8(task, state)) {
    case 0:
        M16(inner, 0x30) = 0x478;
        M8(inner, 0x34) = (s8)actor_allocate_effect_slot();
        M32(inner, 0x38) = 0x80000;
        M32(inner, 0x60) = 0x10000;
        M32(inner, 0x68) = shc_shift_left(1, M8(task, 0x0F));
        TASK_S8(task, state) += 1;
        break;
    case 1:
        if (g_scene_event_flags & M32(inner, 0x68)) {
            M32(inner, 0x64) = -0x800;
            M32(inner, 0x6C) = -0x200;
            M32(inner, 0x58) = 2;
            if (M8(inner, 0x34) < 8 && (M8(task, 0x0F) + 1) < 8) {
                u8 *rec = g_06098838 + M8(inner, 0x34) * 8;
                M32(rec, 0) = M8(task, 0x0F) + 1;
                M32(rec, 4) = 0x12C;
            }
            TASK_S8(task, state) += 1;
        }
        break;
    case 2: {
        s32 v;
        M32(inner, 0x60) += M32(inner, 0x64);
        v = M32(inner, 0x64) + M32(inner, 0x6C);
        M32(inner, 0x64) = v;
        if (v < 0 && M32(inner, 0x60) < 0x2000) {
            s32 t;
            M32(inner, 0x64) = -(M32(inner, 0x64) >> 1);
            t = M32(inner, 0x58) - 1;
            M32(inner, 0x58) = t;
            if ((t + 1) < 0) {
                TASK_S8(task, state) = 3;
            }
        }
        break;
    }
    case 3:
        break;
    }

    g_matrix_stack_cursor += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at0((u8 *)inner + 8);
    mtx_rotate_zyx((u8 *)inner + 0x24);
    matrix_scale_columns(M32(inner, 0x60), 0x10000, 0x10000);
    model_submit_culled(inner, 0);
    g_matrix_stack_cursor -= 0x30;
}
