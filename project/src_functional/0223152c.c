/* projectile_launch_dual_task [0x0223152c]: Existing map corroborated by source: launch-init variant (sibling of 0x2317c0). (src symbols_band231.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x0223152c (task handler) - projectile launch-init variant that also spawns two
 * trail/muzzle child objects (sibling of 0x022317c0 projectile_launch_task).
 * ent = task->unk8; oent = task->unk14->unk8 (owner/target entity).
 * src = &g_06097D58[(oent->unk34 << 8) + (u8)ent->unk5F * 0x10] (per-slot anim/pos rec).
 * Reserve ammo: if func_0223c74a(oent) != 0 -> kill (task->unk0 = 0) and return.
 * Else seed entity: pos ent->unk8/C/10 = src[0/4/8]; ent->unk34(b)=oent->unk34;
 *   ent->unk38 = oent->unk38; ent->unk68 |= g_0604EBE4[(s8)ent->unk34];
 *   ent->unk5A(w) = ent->unk58(w); optional sound_queue_push(0xA01312FF) when g_060E0DDC.
 * Spawn two 0x6c-byte children via spawn_obj_0x6c(task, handler):
 *   c1 handler 0x02239194: c1->unkF(b)=oent->unk44, c1->unk13(b)=ent->unk59, ent->unk78=c1;
 *   c2 handler 0x0223938C: c2->unkF(b)=oent->unk44, c2->unk13(b)=ent->unk59.
 * ent->unk70 = 0xC000; next step task->unk0 = 0x02231630.
 * fidelity: HIGH - oent->unk34 byte<<8, ent->unk5F u8*0x10, table g_0604EBE4 stride 4,
 *   counter unk58/5A word, child byte fields.
 * bsrf 0x23c74a(ammo)/0x215f6c(sound)/0x20fece(spawn x2); handlers via mova+add. */
extern s32   actor_consume_linked_effect_count(void *oent);                     /* reserve ammo (!=0 = fail) */
extern void  sound_queue_push(s32 cmd);                        /* sound_queue_push */
extern void *task_spawn_current_work6c(void *parent, void *handler);    /* spawn_obj_0x6c */
extern u8    g_06097d58[];
extern s32   g_actor_slot_masks;
extern s32   g_sound_effects_enabled;

void projectile_launch_dual_task(void *task) {
    void *ent = TASK_PTR(task, work);
    void *oent = MP(TASK_PTR(task, parent), 8);
    u8 *src = &g_06097d58[(M8(oent, 0x34) << 8) + MU8(ent, 0x5F) * 0x10];
    void *c1;
    void *c2;

    if (actor_consume_linked_effect_count(oent) != 0) {
        TASK_S32(task, handler) = 0;
        return;
    }

    M32(ent, 0x8)  = M32(src, 0);
    M32(ent, 0xC)  = M32(src, 4);
    M32(ent, 0x10) = M32(src, 8);
    M8(ent, 0x34)  = M8(oent, 0x34);
    M32(ent, 0x38) = M32(oent, 0x38);
    M32(ent, 0x68) |= (&g_actor_slot_masks)[M8(ent, 0x34)];
    M16(ent, 0x5A) = M16(ent, 0x58);
    if (g_sound_effects_enabled != 0)
        sound_queue_push(0xA01312FF);

    c1 = task_spawn_current_work6c(task, (void *)0x02239194);
    M8(c1, 0xF)  = M8(oent, 0x44);
    M8(c1, 0x13) = M8(ent, 0x59);
    MP(ent, 0x78) = c1;

    c2 = task_spawn_current_work6c(task, (void *)0x0223938C);
    M8(c2, 0xF)  = M8(oent, 0x44);
    M8(c2, 0x13) = M8(ent, 0x59);

    M32(ent, 0x70) = 0xC000;
    TASK_S32(task, handler) = (s32)0x02231630;
}
