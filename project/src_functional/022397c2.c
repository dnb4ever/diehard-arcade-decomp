/* projectile_arc_handler [0x022397c2]: Existing map corroborated by source: 2-state arc toss. ent=task[8]. s0: base=g_06097da8[src->unk34*0x100], pos=base[0/4/8], horiz kick by (u16)src->unk26==0xC000, unk40=0x48000 unk44=0x600 unk48=0. s1: unk40+=0x8000; unkC+=unk44; unk30=(unk48>>1)+0x17C; unk48++; kill when >10; tail func_022153f6(ent,unk40,1). braf resolved. */
#include "dh_work.h"
#include "dh.h"
/* 0x022397c2 (callC) - 2-state projectile/arc task handler.
 * ent = task->unk8; state = task->unk10.
 *   state 0 (launch): src = task->unk14->unk8; base = &g_06097da8[src->unk34*0x100].
 *     ent->unk8 = base[0]; ent->unkC = base[4]; ent->unk10 = base[8];
 *     horizontal kick ent->unk8 += ((u16)src->unk26 == 0xC000 ? 0x16000 :
 *     0xFFFEA000); ent->unk40 = 0x48000; ent->unk44 = 0x600; ent->unk48 = 0;
 *     advance to state 1 (fallthrough).
 *   state 1 (fly): ent->unk40 += 0x8000; ent->unkC += ent->unk44;
 *     ent->unk30 = (s16)((ent->unk48 >> 1) + 0x17C); ent->unk48++;
 *     when ent->unk48 > 10 kill (task->unk0 = 0); then tail-draw.
 *   default: return (no draw).
 * tail: func_022153f6(ent, ent->unk40, 1) (draw_child_sprite).
 * fidelity: HIGH. braf tail resolved to func_022153f6. */
extern u8   g_06097da8[];
extern void sprite_draw_child(void *obj, s32 scale, s32 flags);

void projectile_arc_handler(void *task) {
    void *ent = TASK_PTR(task, work);

    switch (TASK_S8(task, state)) {
    case 0: {
        void *src = MP(TASK_PTR(task, parent), 0x8);
        void *base = &g_06097da8[M8(src, 0x34) * 0x100];
        M32(ent, 0x8) = M32(base, 0x0);
        M32(ent, 0xC) = M32(base, 0x4);
        M32(ent, 0x10) = M32(base, 0x8);
        M32(ent, 0x8) += ((u16)M16(src, 0x26) == 0xC000) ? 0x16000 : (s32)0xFFFEA000;
        M32(ent, 0x40) = 0x48000;
        M32(ent, 0x44) = 0x600;
        M32(ent, 0x48) = 0;
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
    }
    /* fallthrough */
    case 1:
        M32(ent, 0x40) += 0x8000;
        M32(ent, 0xC) += M32(ent, 0x44);
        M16(ent, 0x30) = (s16)((M32(ent, 0x48) >> 1) + 0x17C);
        M32(ent, 0x48) += 1;
        if (M32(ent, 0x48) > 0xA)
            TASK_S32(task, handler) = 0;
        sprite_draw_child(ent, M32(ent, 0x40), 1);
        break;
    default:
        break;
    }
}
