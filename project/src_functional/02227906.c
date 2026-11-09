/* effect_handoff_sprite_task [0x02227906]: Moves a timed sprite toward its partner record, updates the counter and draws each active frame. */
#include "dh_work.h"
#include "dh.h"
/* 0x02227906  task update step for a "handoff/throw" body (task at arg0, body at arg0+8). Aborts
 * (clears task->unk0) if the partner entity g_060995B0[body->unk60 + 1]'s sub-struct (+8) has its
 * +0x56 flag clear. Otherwise runs a 4-phase state machine on task->unk10:
 *   0: borrow the shared transform g_06098878 (+0x30), seed body position (0x8/0xC/0x10) from it,
 *      set kind 0x24 and a 10-frame timer (+0x58), advance phase.
 *   1: drift down (+0xC += 0x800) for 10 frames, then advance.
 *   2: ease position a quarter of the way toward target record g_06087ECC[body->unk60] for 8
 *      frames, then bump g_06099790[unk60] by body->unk64 and advance.
 *   3: drift (+0xC += 0x100) until the timer expires, then end the task.
 * Every active frame tail-calls func_022154fc(body, 0x20000, 0x20000, 4).
 * fidelity: HIGH - states, fields and the final tail call verified against the ROM. The 8x8 ease
 * table index keeps the ROM's (s8) truncation of unk60*0xC. */
extern void *g_actor_slot_tasks[];   /* entity pointer table */
extern void *g_matrix_stack_cursor;     /* shared transform cursor */
extern u8   g_06087ECC[];    /* 12-byte target-position records */
extern u16  g_06099790[];    /* per-slot accumulator */
extern void mtx_load_abc(s32 a, s32 b);
extern void mtx_apply_vec_at0(void *p);
extern void sprite_draw_billboard(void *body, s32 a, s32 b, s32 c);
void effect_handoff_sprite_task(void *task) {
    void *body = TASK_PTR(task, work);
    s32 slot = M32(body, 0x60);
    void *ent = g_actor_slot_tasks[slot + 1];

    if (MU16(MP(ent, 8), 0x56) == 0) {
        TASK_S32(task, handler) = 0;
        return;
    }
    switch (TASK_S8(task, state)) {
    case 0:
        g_matrix_stack_cursor = (char *)g_matrix_stack_cursor + 0x30;
        mtx_load_abc(slot, 0);
        mtx_apply_vec_at0((char *)body + 8);
        M32(body, 0x8)  = M32(g_matrix_stack_cursor, 0xC);
        M32(body, 0xC)  = M32(g_matrix_stack_cursor, 0x1C);
        M32(body, 0x10) = M32(g_matrix_stack_cursor, 0x2C);
        g_matrix_stack_cursor = (char *)g_matrix_stack_cursor - 0x30;
        M16(body, 0x30) = 0x24;
        M32(body, 0x58) = 0xA;
        TASK_S8(task, state) += 1;
        break;
    case 1:
        M32(body, 0xC) += 0x800;
        if (--M32(body, 0x58) == 0) {
            M32(body, 0x58) = 0xA;
            TASK_S8(task, state) += 1;
        }
        break;
    case 2: {
        u8 *rec = g_06087ECC + (s8)(slot * 0xC);
        s32 cx = M32(body, 0x8);
        s32 cy = M32(body, 0xC);
        s32 cz = M32(body, 0x10);
        M32(body, 0x8)  = cx + ((M32(rec, 0) - cx) >> 2);
        M32(body, 0xC)  = cy + ((M32(rec, 4) - cy) >> 2);
        M32(body, 0x10) = cz + ((M32(rec, 8) - cz) >> 2);
        if (--M32(body, 0x58) == 0) {
            M32(body, 0x58) = 8;
            g_06099790[slot] = (u16)(g_06099790[slot] + M32(body, 0x64));
            TASK_S8(task, state) += 1;
        }
        break;
    }
    case 3:
        M32(body, 0xC) += 0x100;
        if (--M32(body, 0x58) == 0) {
            TASK_S32(task, handler) = 0;
        }
        break;
    }
    sprite_draw_billboard(body, 0x20000, 0x20000, 4);
}
