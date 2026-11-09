/* actor_tick_animation_sequence [0x022119a4]: Frame sequence at 06053344 advances display record, then expires. */
/* 0x022119a4 (actor_play_anim_seq) - per-frame updater for an actor that plays a
 * frame sequence from the table at 0x06053344 on its display record base = arg0->unk8.
 * Two phases on arg0->unk10:
 *   phase 0 (init): base->unk58 (frame counter) = 0; base->unk30 = table[0]; base->unk20
 *     = 0x100; base->unk34 = (s8)func_02228106(arg0) (sequence length). If that is
 *     negative, deactivate (arg0->unk0 = 0) and stop. Else set phase 1, call
 *     func_02210858(base, 0x06052814), and fall through.
 *   phase 1 (run): advance frame counter, base->unk30 = table[counter-1], sample the
 *     animation via func_022109d4(base) (-> ret), set base->unk26 = obj->unk5e +
 *     base->unk62 (obj = arg0->unk14->unk8), build the local offset with
 *     mtx_push_rotate_apply(base+0x14, base+0x24), compose world position from the current
 *     transform origin *0x06098b1c/20/24 plus base->unk4c/0/54 into base->unk8/C/10,
 *     push a transform (*0x06098878 += 0x30), run mtx_load_abc(0x06098b1c) /
 *     mtx_apply_vec_at8(base) / mtx_rotate_zyx_at24(base), and if the composed depth
 *     (*0x06098878)->unk2c > 0x38000 submit the model via func_02215a2c(base, 0), then
 *     pop the transform. When the anim sampler signals end (ret != 0) or the sequence
 *     runs past 0x1b frames, clear timer slot 0x060996d4[base->unk34-8] (if unk34>8)
 *     and deactivate (arg0->unk0 = 0).
 * fidelity: faithful to operations - call targets resolved from bsrf and argument
 *   registers read at each call site (func_02228106 is out of band, modelled as
 *   taking arg0 / returning the s8 length it stores). Functional, not byte-exact. */
#include "dh_band21.h"

extern s32  actor_allocate_effect_slot(u32 actor);
extern void animation_bind_channel(u32 base, u32 arg1);
extern s16  animation_sample_absolute(u32 base);
extern void mtx_push_rotate_apply(u32 a, u32 b);
extern void mtx_load_abc(u32 a);
extern void mtx_apply_vec_at8(u32 base);
extern void mtx_rotate_zyx_at24(u32 base);
extern void model_submit_culled(u32 base, s32 arg1);

void actor_tick_animation_sequence(u32 arg0) {
    u32 base = MEM32(arg0 + 8);
    s32 phase = MEM8(arg0 + 0x10);
    s16 ret;
    s32 n;
    u32 obj;
    s32 slot;

    switch (phase) {
    case 0:
        MEM32(base + 0x58) = 0;
        MEM16(base + 0x30) = MEM16(0x06053344);
        MEM16(base + 0x20) = 0x100;
        MEM8(base + 0x34) = (s8)actor_allocate_effect_slot(arg0);
        if ((s8)MEM8(base + 0x34) < 0) {
            MEM32(arg0) = 0;
            return;
        }
        MEM8(arg0 + 0x10) = 1;
        animation_bind_channel(base, 0x06052814);
        /* fallthrough */
    case 1:
        n = MEM32(base + 0x58) + 1;
        MEM32(base + 0x58) = n;
        MEM16(base + 0x30) = MEM16(0x06053344 + (n - 1) * 2);

        ret = animation_sample_absolute(base);

        obj = MEM32(MEM32(arg0 + 0x14) + 8);
        MEM16(base + 0x26) = (s16)(MEM16(obj + 0x5e) + MEM16(base + 0x62));

        mtx_push_rotate_apply(base + 0x14, base + 0x24);

        MEM32(base + 0x8)  = MEM32(DH_TRANSFORMED_VECTOR_ADDR) + MEM32(base + 0x4c);
        MEM32(base + 0xc)  = MEM32(0x06098b20);
        MEM32(base + 0x10) = MEM32(0x06098b24) + MEM32(base + 0x54);

        MEM32(DH_MATRIX_STACK_CURSOR_ADDR) += 0x30;
        mtx_load_abc(DH_TRANSFORMED_VECTOR_ADDR);
        mtx_apply_vec_at8(base);
        mtx_rotate_zyx_at24(base);
        if (MEM32(MEM32(DH_MATRIX_STACK_CURSOR_ADDR) + 0x2c) > 0x38000) {
            model_submit_culled(base, 0);
        }
        MEM32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;

        if (ret != 0 || MEM32(base + 0x58) >= 0x1b) {
            slot = MEM8(base + 0x34);
            if (slot > 8) {
                MEM8(DH_EFFECT_SLOT_OCCUPIED_ADDR + (slot - 8)) = 0;
            }
            MEM32(arg0) = 0;
        }
        return;
    }
}
