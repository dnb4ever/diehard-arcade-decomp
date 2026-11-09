/* debris_ballistic_update_source_alias [0x0221c8f4]: Duplicate draft bound to 0221c8f2 by v1.09; gravity/spin and owner completion gate. */
/* 0x0221c8f4 (debris_ballistic_update) - task handler for a ballistic debris/body
 * that arcs, spins and lands. ent = task->unk8. State at task->unk10.
 * (The real first instruction is at 0x21c8f2 "mov #16,r0" - it loads the state-field
 * offset into r0 for the entry dispatch; the body is keyed at 0x21c8f4.)
 *   0 (init): ent->unk30 = 0x46; pick a launch param by the low bits of the global
 *      *0x0609856C: idx = ((v & 4) >> 1) + (v & 1); ent->unk1C = g_060748C4[idx];
 *      bump phase; fall into physics.
 *   1 (physics): all deltas are scaled by the global frame shift g_06096C90 via
 *      ashr_var. ent->unk5C -= (0x240 >> s) (gravity on Z velocity);
 *      ent->unk28 += (0x200 >> s); ent->unk24 += (0x100 >> s) (tumble angles);
 *      ent->unkC += (ent->unk5C >> s); ent->unk8 -= (ent->unk1C >> s) (integrate).
 *      Once ent->unkC drops to <= 0x1000 (ground), bump phase; fall through.
 *   2: if the owning controller task->unk14->unk12 == 1, clear it, deactivate this
 *      task (task->unk0 = 0) and return; else fall through to draw.
 *   other: draw.
 * draw: push matrix (g_06098878 += 0x30); mtx_load_abc; mtx_apply_vec_at8(ent);
 *   mtx_rotate_zyx_at24(ent); submit_model_culled(ent, 0); pop matrix.
 * ABI: r4 = task; no return.
 * fidelity: HIGH - reconstructed from fn.py. 5 ashr_var calls (0x24a420, r0/r1 ABI)
 *   and the draw tail (mtx_load_abc 0x2149aa, mtx_apply_vec_at8 0x214a40,
 *   mtx_rotate_zyx_at24 0x214a06, submit_model_culled 0x215a2c) resolved via
 *   braf_resolve. shar (arith >>1) and the &4/&1 bit math preserved exactly. */
#include "dh_band21.h"

extern s32  shc_shift_right_signed(s32 value, s32 count);  /* ashr_var: (signed)value >> count */
extern void mtx_load_abc(void);                   /* mtx_load_abc */
extern void mtx_apply_vec_at8(u8 *obj);           /* mtx_apply_vec_at8 */
extern void mtx_rotate_zyx_at24(u8 *obj);         /* mtx_rotate_zyx_at24 */
extern void submit_model_culled(u32 obj, s32 flag); /* func_02215a2c */

void debris_ballistic_update_source_alias(u32 task)
{
    u32 ent = MEMU32(task + 8);
    s32 v, idx, s, ctrl;

    switch (MEM8(task + 16)) {
    case 0:
        v = MEM32(0x0609856c);
        idx = ((v & 4) >> 1) + (v & 1);
        MEM16(ent + 0x30) = 0x46;
        MEM32(ent + 0x1c) = MEM32(0x060748c4 + idx * 4);
        MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        /* fallthrough */
    case 1:
        s = MEM32(0x06096c90);
        MEM32(ent + 0x5c) -= shc_shift_right_signed(0x240, s);
        MEM16(ent + 0x28) += shc_shift_right_signed(0x200, s);
        MEM16(ent + 0x24) += shc_shift_right_signed(0x100, s);
        MEM32(ent + 0x0c) += shc_shift_right_signed(MEM32(ent + 0x5c), s);
        MEM32(ent + 0x08) -= shc_shift_right_signed(MEM32(ent + 0x1c), s);
        if (MEM32(ent + 0x0c) <= 0x1000)
            MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        /* fallthrough */
    case 2:
        ctrl = (s32)MEMU32(task + 0x14);
        if (MEM8((u32)ctrl + 0x12) == 1) {
            MEM8((u32)ctrl + 0x12) = 0;
            MEM32(task + 0) = 0;
            return;
        }
        break;
    default:
        break;
    }

    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at8((u8 *)(uintptr_t)ent);
    mtx_rotate_zyx_at24((u8 *)(uintptr_t)ent);
    submit_model_culled(ent, 0);
    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;
}
