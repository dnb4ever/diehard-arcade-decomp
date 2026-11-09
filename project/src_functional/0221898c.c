/* prop_tick_swing_pair [0x0221898c]: Sine phase drives open/closed angular limits and optional overlay model. */
/* 0x0221898c - swinging two-part prop (door/gate) task handler (r4 = task, ent = task->unk8).
 * ent->unk6C is the current swing angle, driven by sin_lookup of a phase unk68; unk60/unk64
 * are the open/closed limits derived from the rest angle unk26. task->unk12 selects a model
 * (and whether a second overlay model is drawn); task->unk13 picks the start side.
 *   0 (init): ent->unk30 = g_06054798[task->unk12]; unk60 = (u16)unk26;
 *      unk64 = (u16)unk26 - 0x5000; unk7C = 0; if (task->unk13) { unk6C = unk64; state=4; }
 *      else { unk6C = unk60; state=1; }.
 *   1: if (*0x06098580 & 1) { *0x06098580 &= ~1; unk68 = 0; state++. }
 *   2 (close swing): unk68 += 0x100; unk6C -= sin_lookup(unk68) >> 5;
 *      if (unk6C < unk64) { unk6C = unk64; state++. }
 *   3: unk68 = 0; unk7C = 0; state = 4.
 *   4 (hold open): if (task->unk12 & 1) { if (++unk7C > 0x3C) { unk68 = 0; state++. } }
 *      if (*0x06098580 & 2) { *0x06098580 &= ~2; unk68 = 0; state++. }
 *   5 (open swing): unk68 += 0x100; unk6C += sin_lookup(unk68) >> 5;
 *      if (unk6C > unk60) { unk6C = unk60; state++. }
 *      if (*0x06098580 & 1) { unk68 = 0; *0x06098580 &= ~1; state = 2. }
 *   6: unk68 = 0; state = 1.  other: no-op.
 * Always (draw tail): unk26 = unk6E; push matrix (*0x06098878 += 0x30); mtx_load_abc;
 *   mtx_apply_vec_at0(ent+8); mtx_rotate_zyx(ent+0x24); submit_model_culled(ent,0).
 *   If (state > 2 && task->unk12 == 1): mtx_load_abc; mtx_apply_vec_at0(0x06054788);
 *   save unk30 in unk32, unk30 = (s16)*0x06054794, submit_model_culled(ent,0), restore
 *   unk30. Pop matrix (-= 0x30).
 * ABI: r4 = task; no return.
 * fidelity: HIGH - reconstructed from fn.py/m2c. bsrf resolved (braf_resolve): sin_lookup
 *   0x213190 (states 2 & 5), mtx_load_abc 0x2149aa, mtx_apply_vec_at0 0x214a50,
 *   mtx_rotate_zyx 0x2149e4, submit_model_culled 0x215a2c. The swing step is sin_lookup
 *   arithmetic-shifted >>5 (m2c rendered it as a constant 0x68>>5 because it could not
 *   model the call). Second-model block uses the post-update state. */
#include "dh_band21.h"

extern s32  sin_lookup(s32 angle);                   /* sin_lookup */
extern void mtx_load_abc(void);                      /* mtx_load_abc */
extern void mtx_apply_vec_at0(u8 *obj);              /* mtx_apply_vec_at0 */
extern void mtx_rotate_zyx(u16 *angles);             /* mtx_rotate_zyx */
extern void submit_model_culled(u32 obj, s32 flag);  /* func_02215a2c */

void prop_tick_swing_pair(u32 task)
{
    u32 ent = MEMU32(task + 8);

    switch (MEM8(task + 16)) {
    case 0: {
        u16 n26 = (u16)MEM16(ent + 0x26);
        MEM16(ent + 0x30) = MEM16(0x06054798 + MEM8(task + 0x12) * 2);
        MEM32(ent + 0x60) = n26;
        MEM32(ent + 0x64) = n26 - 0x5000;
        MEM32(ent + 0x7c) = 0;
        if (MEM8(task + 0x13) != 0) {
            MEM32(ent + 0x6c) = MEM32(ent + 0x64);
            MEM8(task + 16) = 4;
        } else {
            MEM32(ent + 0x6c) = MEM32(ent + 0x60);
            MEM8(task + 16) = 1;
        }
        break;
    }
    case 1:
        if (MEM32(DH_SCENE_EVENT_FLAGS_ADDR) & 1) {
            MEM32(DH_SCENE_EVENT_FLAGS_ADDR) &= ~1;
            MEM32(ent + 0x68) = 0;
            MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        }
        break;
    case 2: {
        s32 d;
        MEM32(ent + 0x68) += 0x100;
        d = sin_lookup(MEM32(ent + 0x68)) >> 5;
        MEM32(ent + 0x6c) -= d;
        if (MEM32(ent + 0x6c) < MEM32(ent + 0x64)) {
            MEM32(ent + 0x6c) = MEM32(ent + 0x64);
            MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        }
        break;
    }
    case 3:
        MEM32(ent + 0x68) = 0;
        MEM32(ent + 0x7c) = 0;
        MEM8(task + 16) = 4;
        break;
    case 4:
        if (MEM8(task + 0x12) & 1) {
            MEM32(ent + 0x7c) += 1;
            if (MEM32(ent + 0x7c) > 0x3c) {
                MEM32(ent + 0x68) = 0;
                MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
            }
        }
        if (MEM32(DH_SCENE_EVENT_FLAGS_ADDR) & 2) {
            MEM32(DH_SCENE_EVENT_FLAGS_ADDR) &= ~2;
            MEM32(ent + 0x68) = 0;
            MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        }
        break;
    case 5: {
        s32 d;
        MEM32(ent + 0x68) += 0x100;
        d = sin_lookup(MEM32(ent + 0x68)) >> 5;
        MEM32(ent + 0x6c) += d;
        if (MEM32(ent + 0x6c) > MEM32(ent + 0x60)) {
            MEM32(ent + 0x6c) = MEM32(ent + 0x60);
            MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        }
        if (MEM32(DH_SCENE_EVENT_FLAGS_ADDR) & 1) {
            MEM32(ent + 0x68) = 0;
            MEM32(DH_SCENE_EVENT_FLAGS_ADDR) &= ~1;
            MEM8(task + 16) = 2;
        }
        break;
    }
    case 6:
        MEM32(ent + 0x68) = 0;
        MEM8(task + 16) = 1;
        break;
    default:
        break;
    }

    MEM16(ent + 0x26) = MEM16(ent + 0x6e);

    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at0((u8 *)(uintptr_t)(ent + 8));
    mtx_rotate_zyx(PTR(u16, ent + 0x24));
    submit_model_culled(ent, 0);

    if (MEM8(task + 16) > 2 && MEM8(task + 0x12) == 1) {
        mtx_load_abc();
        mtx_apply_vec_at0((u8 *)(uintptr_t)0x06054788);
        MEM16(ent + 0x32) = MEM16(ent + 0x30);
        MEM16(ent + 0x30) = (s16)MEM32(0x06054794);
        submit_model_culled(ent, 0);
        MEM16(ent + 0x30) = MEM16(ent + 0x32);
    }

    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;
}
