/* projectile_emit_task [0x022304b4]: Existing map corroborated by source: spray child spark entities (spawn_from_template func_0220ff28, child handler 0x022305e2) until expiry/owner collision */
#include "dh_work.h"
#include "dh.h"
/* 0x022304b4 (projectile task handler, emit/spray state) - a fired projectile
 * that sprays child spark entities each frame until it expires or its owner
 * collides.
 * ent = task->unk8 (this projectile); src = task->unk14->unk8 (owner actor).
 * Terminate when src->unk84 & 0x43800 (collision/death flags) OR the
 * pre-increment spray counter ent->unk74 exceeds 30 (ent->unk74 is incremented
 * every frame): if g_060e0ddc != 0 fire sound sound_queue_push(0xA0140DFF), then
 * task->unk0 = 0 (kill task).
 * Otherwise spawn a child holder via func_0220ff28(task, template 0x022305e2):
 *   holder->unkF = ent->unk77 & 3; cent = holder->unk8 (child entity);
 *   copy ent->unk68(long), unk5F(byte), unk60/unk62(word), unk64(long),
 *   unk34(byte), unk38(long) into cent; cent->unk5A = ent->unk58;
 *   cent->unk14 = ent->unk14; cent->unk30 = 0x13F.
 *   position record rec = (u8*)g_06097d58 + src->unk34*0x100 + (u8)ent->unk5F*0x10;
 *   cent->unk8/unkC/unk10 = rec[0/4/8] (long); cent->unk22 = ent->unk22.
 *   directional nudge by src->unk44: cent->unk8  += src->unk44 ? 0x3000 : -0x3000;
 *   cent->unkC -= 0x1C00; cent->unk10 += src->unk44 ? 0x1800 : -0x1800.
 * fidelity: HIGH. bsrf resolved: 0x2304ea->sound_queue_push, 0x2304fc->func_0220ff28;
 * child template r5 = mova(.Lpool_2304f8)+0xEA = 0x022305e2. */
extern u8 g_06097d58[];
extern s32 g_sound_effects_enabled;
extern void sound_queue_push(s32 cmd);          /* sound_queue_push */
extern void *task_spawn_list_work7c(void *parent, void *template); /* spawn_from_template */

void projectile_emit_task(void *task) {
    void *src = MP(TASK_PTR(task, parent), 8);
    void *ent = TASK_PTR(task, work);
    s32 cnt;
    void *holder;
    void *cent;
    void *rec;

    if ((M32(src, 0x84) & 0x43800) ||
        (cnt = M32(ent, 0x74), M32(ent, 0x74) = cnt + 1, cnt > 30)) {
        if (g_sound_effects_enabled != 0)
            sound_queue_push(0xA0140DFF);
        TASK_S32(task, handler) = 0;
        return;
    }

    holder = task_spawn_list_work7c(task, (void *)0x060345e2 /* PC-relative pointer in HWRAM */);
    M8(holder, 0xF) = (s8)(M8(ent, 0x77) & 3);
    cent = MP(holder, 8);
    M32(cent, 0x68) = M32(ent, 0x68);
    M8 (cent, 0x5F) = M8(ent, 0x5F);
    M16(cent, 0x60) = M16(ent, 0x60);
    M16(cent, 0x62) = M16(ent, 0x62);
    M32(cent, 0x64) = M32(ent, 0x64);
    M8 (cent, 0x34) = M8(ent, 0x34);
    M32(cent, 0x38) = M32(ent, 0x38);
    M16(cent, 0x5A) = M16(ent, 0x58);
    M32(cent, 0x14) = M32(ent, 0x14);
    M16(cent, 0x30) = 0x13F;

    rec = &g_06097d58[(s32)M8(src, 0x34) * 0x100 + (u8)M8(ent, 0x5F) * 0x10];
    M32(cent, 0x8)  = M32(rec, 0);
    M32(cent, 0xC)  = M32(rec, 4);
    M32(cent, 0x10) = M32(rec, 8);
    M16(cent, 0x22) = M16(ent, 0x22);

    M32(cent, 0x8)  += M8(src, 0x44) ? 0x3000 : -0x3000;
    M32(cent, 0xC)  -= 0x1C00;
    M32(cent, 0x10) += M8(src, 0x44) ? 0x1800 : -0x1800;
}
