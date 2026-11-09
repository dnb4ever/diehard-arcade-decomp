/* enemy_tick_grabbed_overlay [0x022433e8]: Publishes active grab handle, snapshots angles and draws victim overlay. */
#include "dh_work.h"
#include "dh.h"
/* 0x022433e8  per-frame step + draw for the grabbed-victim overlay. arg0 = task,
 * inner = MP(task,8) is the victim actor. State on M8(task,0x10):
 *   0 latch: heading 0x4E3, publish this task as the active grab via g_060973f8,
 *     snapshot the live angle triple (unk24/26/28) into the held copy
 *     (unk2A/2C/2E), advance.
 *   1 held: nothing.
 * Always step the shared task tail func_02246282. Then, while the victim's
 * unk80 bit0 is set (actively held), push the matrix stack, build the hold pose
 * from the fixed offset table g_0608db30 and the held angles (inner+0x2A), draw
 * the body with the temporary heading 0x4ED, and pop.
 *
 * bounds: [0x2433e8, 0x2434ae); split out of func_02243066's over-swallowed
 *   extent (0x2434ae begins func_022434ae).
 * fidelity: HIGH - verified vs fn.py --asm; angle copies are 16-bit. */
extern void *g_060973f8;       /* active grab task */
extern s32  g_matrix_stack_cursor;        /* matrix-stack pointer */
extern s16  g_0608db30[];      /* hold-pose offset table */

void mtx_load_abc(void);                        /* 0x022149aa mtx push/identity */
void mtx_rotate_zyx(void *angles);                /* 0x022149e4 mtx_rotate_zyx */
void mtx_apply_vec_at0(void *vec);                   /* 0x02214a50 apply matrix to vec */
void model_submit_culled(void *entity, s32 flag);      /* 0x02215a2c draw model */
void actor_draw_transformed(void *actor);                 /* 0x02246282 shared task tail */

void enemy_tick_grabbed_overlay(void *task) {
    void *inner = TASK_PTR(task, work);

    switch (TASK_S8(task, state)) {
    case 0:
        M16(inner, 0x30) = 0x4E3;
        g_060973f8 = task;
        M16(inner, 0x2A) = M16(inner, 0x24);
        M16(inner, 0x2C) = M16(inner, 0x26);
        M16(inner, 0x2E) = M16(inner, 0x28);
        TASK_S8(task, state) += 1;
        break;
    case 1:
        break;
    }

    actor_draw_transformed(inner);

    if (M32(inner, 0x80) & 1) {
        s16 saved;
        g_matrix_stack_cursor += 0x30;
        mtx_load_abc();
        mtx_apply_vec_at0(g_0608db30);
        mtx_rotate_zyx((u8 *)inner + 0x2A);
        saved = M16(inner, 0x30);
        M16(inner, 0x30) = 0x4ED;
        model_submit_culled(inner, 0);
        M16(inner, 0x30) = saved;
        g_matrix_stack_cursor -= 0x30;
    }
}
