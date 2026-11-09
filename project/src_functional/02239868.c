/* spread_projectile_handler [0x02239868]: Existing map corroborated by source: 2-state spreading projectile. s0: base=g_06097da8[src->unk34*0x100]; pos; kick unk4C=+-0x3800 and unk8+-0x8000 by (u16)src->unk26==0xC000; unk40=0x38000 unk44=0x600 unk48=0 unk58=0x2000 unk5C=unkC. s1: unk48++ kill when prev>8; unk40+=0x10000; unk8+=unk4C; unk4C-=unk4C>>2; unk58+=0x400; unk30=(unk48>>1)+0x17C; emit 3 child sprites at unkC=unk5C, unk5C+-unk58 via func_022153f6(ent,unk40,1). mova/add call ptr resolved. */
#include "dh_work.h"
#include "dh.h"
/* 0x02239868 (callC) - 2-state spreading projectile task handler.
 * ent = task->unk8; state = task->unk10.
 *   state 0 (launch): src = task->unk14->unk8; base = &g_06097da8[src->unk34*0x100].
 *     ent->unk8 = base[0]; ent->unkC = base[4]; ent->unk10 = base[8].
 *     By facing ((u16)src->unk26 == 0xC000): kick ent->unk4C = -0x3800 and
 *     ent->unk8 -= 0x8000; else ent->unk4C = 0x3800 and ent->unk8 += 0x8000.
 *     ent->unkC += 0x2800; unk40 = 0x38000; unk44 = 0x600; unk48 = 0;
 *     unk58 = 0x2000; unk5C = unkC. advance (fallthrough).
 *   state 1 (fly): unk48++; when the prior unk48 > 8 kill (task->unk0 = 0).
 *     unk40 += 0x10000; unk8 += unk4C; unkC = unk5C; unk4C -= unk4C >> 2;
 *     unk58 += 0x400; unk30 = (s16)((unk48 >> 1) + 0x17C). Then emit three
 *     child sprites at unkC = unk5C, unk5C + unk58, unk5C - unk58 via
 *     func_022153f6(ent, unk40, 1) (draw_child_sprite).
 *   default: return.
 * fidelity: HIGH. mova+add call pointer resolved to func_022153f6. */
extern u8   g_06097da8[];
extern void sprite_draw_child(void *obj, s32 scale, s32 flags);

void spread_projectile_handler(void *task) {
    void *ent = TASK_PTR(task, work);

    switch (TASK_S8(task, state)) {
    case 0: {
        void *src = MP(TASK_PTR(task, parent), 0x8);
        void *base = &g_06097da8[M8(src, 0x34) << 8];
        M32(ent, 0x8) = M32(base, 0x0);
        M32(ent, 0xC) = M32(base, 0x4);
        M32(ent, 0x10) = M32(base, 0x8);
        if ((u16)M16(src, 0x26) == 0xC000) {
            M32(ent, 0x4C) = -0x3800;
            M32(ent, 0x8) -= 0x8000;
        } else {
            M32(ent, 0x8) += 0x8000;
            M32(ent, 0x4C) = 0x3800;
        }
        M32(ent, 0xC) += 0x2800;
        M32(ent, 0x40) = 0x38000;
        M32(ent, 0x44) = 0x600;
        M32(ent, 0x48) = 0;
        M32(ent, 0x58) = 0x2000;
        M32(ent, 0x5C) = M32(ent, 0xC);
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
    }
    /* fallthrough */
    case 1: {
        s32 prev = M32(ent, 0x48);
        M32(ent, 0x48) = prev + 1;
        if (prev > 8) {
            TASK_S32(task, handler) = 0;
            return;
        }
        M32(ent, 0x40) += 0x10000;
        M32(ent, 0x8) += M32(ent, 0x4C);
        M32(ent, 0xC) = M32(ent, 0x5C);
        M32(ent, 0x4C) -= M32(ent, 0x4C) >> 2;
        M32(ent, 0x58) += 0x400;
        M16(ent, 0x30) = (s16)((M32(ent, 0x48) >> 1) + 0x17C);
        sprite_draw_child(ent, M32(ent, 0x40), 1);
        M32(ent, 0xC) = M32(ent, 0x5C) + M32(ent, 0x58);
        sprite_draw_child(ent, M32(ent, 0x40), 1);
        M32(ent, 0xC) = M32(ent, 0x5C) - M32(ent, 0x58);
        sprite_draw_child(ent, M32(ent, 0x40), 1);
        return;
    }
    default:
        break;
    }
}
