/* prop_tick_banked_path [0x022191ec]: Integrates view deltas and samples weighted path table 060547e4; banks orientation. */
/* 0x022191ec - path-following / banking actor task (handler referenced via scene_handler
 * 0x022191ec). entity = task->unk8. A global control record rec = *(*(0x0609882C)+8) supplies
 * per-frame base deltas.
 *   state 0 (init): entity->unk8/unkC/unk10 = {0xFFE70000,0xFFF90000,0x000F0000};
 *     entity->unk37 = 1; entity->unk80 = 0; advance.
 *   state 1 (run): if scene flag 0x06098580 bit 0x00100000 is set, consume it and jump to
 *     state 2. Then every frame:
 *       - integrate base: unk8 += rec->unk14, unkC += rec->unk18, unk10 += rec->unk1C;
 *       - advance path param unk80 += 0x200 and evaluate weighted_accum3(table 0x060547E4,
 *         unk80) into entity+0x9C (writes unk9C/unkA0/unkA4);
 *       - target columns: unk90 = rec->unk8 - unkA4, unk94 = rec->unkC + unkA0,
 *         unk98 = rec->unk10 + unk9C;
 *       - ease bank angle unk28 toward (unk90 - unk8) and pitch angle unk24 toward
 *         (unk10 - unk98): if |delta| >= 0x10000 step by delta>>12 (clamped +/-0x600), else
 *         snap +/-16 toward 0;
 *       - unk18 = (unk94 - unkC) >> 5; apply the oriented local vector via
 *         mtx_push_rotate_apply(const vec @0x060548A4, angles @entity+0x24) -> g_06098B1C;
 *       - read back unk14 = *0x06098B1C, unk1C = *0x06098B24; integrate unk8/unkC/unk10 by
 *         unk14/unk18/unk1C; unk68 += 0x300; unk26 = sin_lookup(unk14) >> 5;
 *       - draw via func_0221b330(entity).
 *   state 2 / default: *task = 0 (terminate) on state 2; otherwise nothing.
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. braf/ptr: weighted_accum3 0x214194,
 *   mtx_push_rotate_apply 0x21080a, sin_lookup 0x213190, func_0221b330 0x21b330. All shifts
 *   are fixed-count (>>12, >>5, arithmetic shar). Angle fields are s16; clamp min stored as
 *   0xFA00 (= -0x600). The easing of the two angle fields is factored into a static helper. */
#include "dh_band21.h"

extern void weighted_accum3(void *rows, s32 sel, void *out3);   /* func_02214194 */
extern void mtx_push_rotate_apply(s32 *vec, u8 *angles);        /* func_0221080a */
extern s32  sin_lookup(s32 angle);                              /* func_02213190 */
extern void model_draw_composite_parts(u32 entity);                          /* object_post_update draw */

/* ease a signed-16 angle field at addr toward 0 by delta (0x022191ec blocks @0x28 and @0x24) */
static void ease_angle_field(u32 addr, s32 delta)
{
    s32 a = (delta < 0) ? -delta : delta;

    if (a >= 0x10000) {
        if (delta > 0) {
            s16 v = (s16)(MEM16(addr) + (delta >> 12));
            MEM16(addr) = v;
            if (v > 0x600)
                MEM16(addr) = 0x600;
        } else {
            s16 v = (s16)(MEM16(addr) + (delta >> 12));
            MEM16(addr) = v;
            if (v < -0x600)
                MEM16(addr) = (s16)0xfa00;
        }
    } else {
        s16 cur = MEM16(addr);
        if (cur > 0) {
            s16 v = (s16)(cur - 16);
            MEM16(addr) = v;
            if (v < 0)
                MEM16(addr) = 0;
        } else {
            s16 v = (s16)(cur + 16);
            MEM16(addr) = v;
            if (v > 0)
                MEM16(addr) = 0;
        }
    }
}

void prop_tick_banked_path(u32 task)
{
    s32 L[3];
    u32 entity = MEMU32(task + 8);
    u32 rec = MEMU32(MEMU32(DH_VIEW_TASK_ADDR) + 8);

    L[0] = MEM32(0x060548a4);
    L[1] = MEM32(0x060548a8);
    L[2] = MEM32(0x060548ac);

    switch (MEM8(task + 0x10)) {
    case 0:
        MEM32(entity + 0x08) = (s32)0xffe70000;
        MEM32(entity + 0x0c) = (s32)0xfff90000;
        MEM32(entity + 0x10) = 0x000f0000;
        MEM8(entity + 0x37) = 1;
        MEM32(entity + 0x80) = 0;
        MEM8(task + 0x10) = (s8)(MEM8(task + 0x10) + 1);
        break;
    case 1:
        if (MEM32(DH_SCENE_EVENT_FLAGS_ADDR) & 0x00100000) {
            MEM32(DH_SCENE_EVENT_FLAGS_ADDR) &= ~0x00100000;
            MEM8(task + 0x10) = 2;
        }

        MEM32(entity + 0x08) += MEM32(rec + 0x14);
        MEM32(entity + 0x0c) += MEM32(rec + 0x18);
        MEM32(entity + 0x10) += MEM32(rec + 0x1c);

        MEM32(entity + 0x80) += 0x200;
        weighted_accum3((void *)(uintptr_t)0x060547e4, MEM32(entity + 0x80),
                        (void *)(uintptr_t)(entity + 0x9c));

        MEM32(entity + 0x90) = MEM32(rec + 0x08) - MEM32(entity + 0xa4);
        MEM32(entity + 0x94) = MEM32(rec + 0x0c) + MEM32(entity + 0xa0);
        MEM32(entity + 0x98) = MEM32(rec + 0x10) + MEM32(entity + 0x9c);

        ease_angle_field(entity + 0x28, MEM32(entity + 0x90) - MEM32(entity + 0x08));
        ease_angle_field(entity + 0x24, MEM32(entity + 0x10) - MEM32(entity + 0x98));

        MEM32(entity + 0x18) = (MEM32(entity + 0x94) - MEM32(entity + 0x0c)) >> 5;

        mtx_push_rotate_apply(L, (u8 *)(uintptr_t)(entity + 0x24));

        MEM32(entity + 0x14) = MEM32(DH_TRANSFORMED_VECTOR_ADDR);
        MEM32(entity + 0x1c) = MEM32(0x06098b24);

        MEM32(entity + 0x08) += MEM32(entity + 0x14);
        MEM32(entity + 0x0c) += MEM32(entity + 0x18);
        MEM32(entity + 0x10) += MEM32(entity + 0x1c);

        MEM32(entity + 0x68) += 0x300;
        MEM16(entity + 0x26) = (s16)(sin_lookup(MEM32(entity + 0x14)) >> 5);

        model_draw_composite_parts(entity);
        break;
    case 2:
        MEM32(task) = 0;
        break;
    default:
        break;
    }
}
