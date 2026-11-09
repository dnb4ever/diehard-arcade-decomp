/* prop_tick_spinning_pickup [0x022186ec]: Five phases cycle spin envelope, model table and optional companion object. */
/* 0x022186ec (spinning_pickup_update) - task handler for a rotating pickup/token that
 * pulses its spin speed through a 5-phase cycle, then draws. ent = task->unk8.
 * State at task->unk10:
 *   0 (init): pick the model id from the table at 0x06054750 - if ent->unk26 == 0 use
 *      *(s16*)0x06054752 and spawn a companion object
 *      (spawn_from_template(task, func_02232178)); else use *(s16*)0x06054750. Seed the
 *      spin speed envelope from task->unk15: fast lane -> ent->unk60 = ent->unk64 =
 *      0xA00, period 0x78; slow lane -> 0xC00, period 0x96. ent->unk68 = ent->unk6C =
 *      period. Advance phase.
 *   1: countdown ent->unk68; at 0 reload from ent->unk6C and advance (hold at speed).
 *   2: ease spin speed down: ent->unk60 -= 0x20 until < 0x20, then advance.
 *   3: countdown ent->unk68 like phase 1 (hold at min).
 *   4: ease spin speed up: ent->unk60 += 0x20 until >= ent->unk64, then reload the
 *      timer and loop back to phase 1.
 * All phases: integrate the spin angle ent->unk28 += ent->unk60, then draw: push a
 *   matrix frame (g_06098878 += 0x30); mtx_load_abc; mtx_apply_vec_at0(ent+8);
 *   mtx_rotate_y(ent->unk26); mtx_rotate_z(ent->unk28); submit_model_culled(ent,0);
 *   pop frame.
 * ABI: r4 = task; no return.
 * fidelity: HIGH - reconstructed from fn.py. bsrf resolved (braf_resolve):
 *   spawn_from_template 0x20ff28 (template 0x232178 via mova+add PIC), mtx_load_abc
 *   0x2149aa, mtx_apply_vec_at0 0x214a50, mtx_rotate_y 0x213bf4, mtx_rotate_z 0x213c4c,
 *   submit_model_culled 0x215a2c. The 0x218734..0x21875e gap is the literal pool. */
#include "dh_band21.h"

extern void *task_spawn_list_work7c(void *parent, void *template);  /* spawn_from_template */
extern void  muzzle_spark_task(void);                          /* companion template */
extern void  mtx_load_abc(void);                           /* mtx_load_abc */
extern void  mtx_apply_vec_at0(u8 *obj);                   /* mtx_apply_vec_at0 */
extern void  mtx_rotate_y(s32 angle);                      /* mtx_rotate_y */
extern void  mtx_rotate_z(s32 angle);                      /* mtx_rotate_z */
extern void  submit_model_culled(u32 obj, s32 flag);       /* func_02215a2c */

void prop_tick_spinning_pickup(u32 task)
{
    u32 ent = MEMU32(task + 8);
    s32 period;

    switch (MEM8(task + 16)) {
    case 0:
        if (MEM16(ent + 0x26) == 0) {
            MEM16(ent + 0x30) = MEM16(0x06054752);
            task_spawn_list_work7c((void *)(uintptr_t)task, (void *)muzzle_spark_task);
        } else {
            MEM16(ent + 0x30) = MEM16(0x06054750);
        }
        if (MEM8(task + 15) != 0) {
            MEM32(ent + 0x64) = 0xa00;
            MEM32(ent + 0x60) = 0xa00;
            period = 0x78;
        } else {
            MEM32(ent + 0x64) = 0xc00;
            MEM32(ent + 0x60) = 0xc00;
            period = 0x96;
        }
        MEM32(ent + 0x6c) = period;
        MEM32(ent + 0x68) = period;
        MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        break;
    case 1:
        MEM32(ent + 0x68) -= 1;
        if (MEM32(ent + 0x68) == 0) {
            MEM32(ent + 0x68) = MEM32(ent + 0x6c);
            MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        }
        break;
    case 2:
        MEM32(ent + 0x60) -= 0x20;
        if (MEM32(ent + 0x60) < 0x20)
            MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        break;
    case 3:
        MEM32(ent + 0x68) -= 1;
        if (MEM32(ent + 0x68) == 0) {
            MEM32(ent + 0x68) = MEM32(ent + 0x6c);
            MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        }
        break;
    case 4:
        MEM32(ent + 0x60) += 0x20;
        if (MEM32(ent + 0x60) >= MEM32(ent + 0x64)) {
            MEM32(ent + 0x68) = MEM32(ent + 0x6c);
            MEM8(task + 16) = 1;
        }
        break;
    default:
        break;
    }

    MEM16(ent + 0x28) = (s16)(MEM16(ent + 0x28) + MEM32(ent + 0x60));

    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at0((u8 *)(uintptr_t)(ent + 8));
    mtx_rotate_y((u16)MEM16(ent + 0x26));
    mtx_rotate_z((u16)MEM16(ent + 0x28));
    submit_model_culled(ent, 0);
    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;
}
