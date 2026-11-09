/* prop_tick_lateral_sequence [0x02218eac]: Task states translate X between table bounds, randomize phases and spawn companions. */
/* 0x02218eac - multi-state actor task handler (r4 = task, ent = task->unk8).
 * Switch on task->unk10:
 *   0 (init): ent->unk8 = *0x060547D8, ent->unkC = 0, ent->unk10 = *0x060547E0,
 *      ent->unk60 = ent->unk64 = 0. If task->unkF != 0: three calls to
 *      func_0220ff28(task, 0x232294); after the 2nd, byte +0xF of its return value = 1,
 *      after the 3rd, byte +0xF = 2; then state++. Else ent->unk58 = 0x7FFFFFFF, state=3.
 *   1: ent->unk8 -= 0x3000; if <= *0x060547D4: ent->unk58 = 20 (stored before the call),
 *      ret = func_02210020(); if (ret & 1) ent->unk64 = 4;
 *      else { ent->unk60 = 1; ent->unk64 = 2; } state = 3.
 *   2: ent->unk8 += 0x3000; if >= *0x060547D8: ent->unk58 = 20 (before the call),
 *      ret = func_02210020(); if (ret & 1) ent->unk64 = 4;
 *      else { ent->unk60 = 0; ent->unk64 = 1; } state = 3.
 *   3: ent->unk58 -= 1; if previous value <= 0, state = *(u8*)(ent+0x67).
 *   4: shared flag-clear block (see below); ent->unk10 -= 0x3000;
 *      if <= *0x060547DC { ent->unk58 = 50; ent->unk64 = 5; state = 6; }.
 *   5: ent->unk10 += 0x3000; if >= *0x060547E0 { ent->unk58 = 20; state = 3;
 *      if (ent->unk60) { ent->unk60 = 0; ent->unk64 = 1; }
 *      else { ent->unk60 = 1; ent->unk64 = 2; } }.
 *   6: shared flag-clear block, then the state-3 tail.
 *   flag-clear block (states 4 & 6): if (*0x06098670 == 1 && *0x06098830 >= 9):
 *      *0x06098830 = 9; (*(0x0609882C))->unkD = 1; and for r10 = *(*(0x0609882C)+8):
 *      halfwords at r10+0xAE/0xB0/0xB2 = 0.
 * Tail: ent->unk78++; if > 18 { ent->unk78 = 0; if (state >= 4 && *0x060E0DDC != 0)
 *   sound_queue_push(0xA0140BFF); }. ent->unk26 (s16) += 0x300; *0x06098878 += 0x30;
 *   mtx_load_abc(); mtx_apply_vec_at0(ent+8); ent->unk30 = local[0];
 *   submit_model_culled(ent, 0); mtx_rotate_zyx(ent+0x24); then four more times
 *   (local[1..4]): ent->unk30 = local[i]; submit_model_culled(ent, 0);
 *   *0x06098878 -= 0x30. local[] is a copy of the 12 bytes at 0x060547C8 (3 words).
 * fidelity: HIGH - hand-traced from disassembly. func_0220ff28 return value's byte +0xF
 *   is written (not task+0xF). ent->unk58 = 20 is stored in the bsrf delay slot, before
 *   func_02210020 runs. State-3 decrement test uses the pre-decrement value (cmp/pl after
 *   add #1). */
#include "dh_band21.h"

extern u32  task_spawn_list_work7c(u32 task, u32 arg);   /* returns pointer-like value */
extern u32  game_random_next(void);
extern void mtx_load_abc(void);                 /* func_022149aa */
extern void mtx_apply_vec_at0(u8 *obj);         /* func_02214a50 */
extern void mtx_rotate_zyx(u16 *angles);        /* func_022149e4 */
extern void submit_model_culled(u32 obj, s32 flag); /* func_02215a2c */
extern void sound_queue_push(s32 req);          /* func_02215f6c */

void prop_tick_lateral_sequence(u32 task)
{
    s16 local[6];
    u32 ent = MEMU32(task + 8);
    u32 ctrl = MEMU32(DH_VIEW_TASK_ADDR);
    u32 r10 = MEMU32(ctrl + 8);
    u32 mode = MEMU32(DH_PLAY_SESSION_MODE_ADDR);
    u32 ret;
    s32 v;
    int i;

    local[0] = (s16)MEM16(0x060547c8);
    local[1] = (s16)MEM16(0x060547ca);
    local[2] = (s16)MEM16(0x060547cc);
    local[3] = (s16)MEM16(0x060547ce);
    local[4] = (s16)MEM16(0x060547d0);
    local[5] = (s16)MEM16(0x060547d2);

    switch (MEM8(task + 0x10)) {
    case 0:
        MEM32(ent + 0x08) = MEM32(0x060547d8);
        MEM32(ent + 0x0c) = 0;
        MEM32(ent + 0x10) = MEM32(0x060547e0);
        MEM32(ent + 0x60) = 0;
        MEM32(ent + 0x64) = 0;
        if (MEM8(task + 0x0f) != 0) {
            task_spawn_list_work7c(task, 0x232294);
            ret = task_spawn_list_work7c(task, 0x232294);
            MEM8(ret + 0x0f) = 1;
            ret = task_spawn_list_work7c(task, 0x232294);
            MEM8(ret + 0x0f) = 2;
            MEM8(task + 0x10) = (s8)(MEM8(task + 0x10) + 1);
        } else {
            MEM32(ent + 0x58) = 0x7fffffff;
            MEM8(task + 0x10) = 3;
        }
        break;
    case 1:
        MEM32(ent + 0x08) -= 0x3000;
        if ((s32)MEM32(ent + 0x08) <= (s32)MEM32(0x060547d4)) {
            MEM32(ent + 0x58) = 20;
            ret = game_random_next();
            if (ret & 1) {
                MEM32(ent + 0x64) = 4;
            } else {
                MEM32(ent + 0x60) = 1;
                MEM32(ent + 0x64) = 2;
            }
            MEM8(task + 0x10) = 3;
        }
        break;
    case 2:
        MEM32(ent + 0x08) += 0x3000;
        if ((s32)MEM32(ent + 0x08) >= (s32)MEM32(0x060547d8)) {
            MEM32(ent + 0x58) = 20;
            ret = game_random_next();
            if (ret & 1) {
                MEM32(ent + 0x64) = 4;
            } else {
                MEM32(ent + 0x60) = 0;
                MEM32(ent + 0x64) = 1;
            }
            MEM8(task + 0x10) = 3;
        }
        break;
    case 3:
        goto state3_tail;
    case 4:
        if (mode == 1 && (s32)MEM32(DH_VIEW_REQUEST_PRIORITY_ADDR) >= 9) {
            MEM32(DH_VIEW_REQUEST_PRIORITY_ADDR) = 9;
            MEM8(ctrl + 0x0d) = 1;
            MEM16(r10 + 0xae) = 0;
            MEM16(r10 + 0xb0) = 0;
            MEM16(r10 + 0xb2) = 0;
        }
        MEM32(ent + 0x10) -= 0x3000;
        if ((s32)MEM32(ent + 0x10) <= (s32)MEM32(0x060547dc)) {
            MEM32(ent + 0x58) = 50;
            MEM32(ent + 0x64) = 5;
            MEM8(task + 0x10) = 6;
        }
        break;
    case 5:
        MEM32(ent + 0x10) += 0x3000;
        if ((s32)MEM32(ent + 0x10) >= (s32)MEM32(0x060547e0)) {
            MEM32(ent + 0x58) = 20;
            MEM8(task + 0x10) = 3;
            if (MEM32(ent + 0x60) != 0) {
                MEM32(ent + 0x60) = 0;
                MEM32(ent + 0x64) = 1;
            } else {
                MEM32(ent + 0x60) = 1;
                MEM32(ent + 0x64) = 2;
            }
        }
        break;
    case 6:
        if (mode == 1 && (s32)MEM32(DH_VIEW_REQUEST_PRIORITY_ADDR) >= 9) {
            MEM32(DH_VIEW_REQUEST_PRIORITY_ADDR) = 9;
            MEM8(ctrl + 0x0d) = 1;
            MEM16(r10 + 0xae) = 0;
            MEM16(r10 + 0xb0) = 0;
            MEM16(r10 + 0xb2) = 0;
        }
state3_tail:
        v = (s32)MEM32(ent + 0x58) - 1;
        MEM32(ent + 0x58) = v;
        if (v + 1 <= 0)
            MEM8(task + 0x10) = (s8)MEM8(ent + 0x67);
        break;
    default:
        break;
    }

    v = (s32)MEM32(ent + 0x78) + 1;
    MEM32(ent + 0x78) = v;
    if (v > 18) {
        MEM32(ent + 0x78) = 0;
        if ((s8)MEM8(task + 0x10) >= 4 && MEM32(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0)
            sound_queue_push((s32)0xa0140bff);
    }

    MEM16(ent + 0x26) = (s16)((s16)MEM16(ent + 0x26) + 0x300);

    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at0((u8 *)(uintptr_t)(ent + 8));
    MEM16(ent + 0x30) = local[0];
    submit_model_culled(ent, 0);
    mtx_rotate_zyx((u16 *)(uintptr_t)(ent + 0x24));
    for (i = 1; i < 5; i++) {
        MEM16(ent + 0x30) = local[i];
        submit_model_culled(ent, 0);
    }
    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;
}
