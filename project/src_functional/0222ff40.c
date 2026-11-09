/* projectile_seed_table_task [0x0222ff40]: Existing map corroborated by source: init/seed (table-vel sibling of func_02230006): copy owner anim/flags; unk68|=g_0604EBE4[anim]; reserve ammo kill-on-fail; x2 unk64 if anim<3; unk62|=0x40; vel from  (src symbols_gen7.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x0222ff40 (projectile task handler, init/seed state) - table-driven sibling
 * of func_02230006. Reserve ammo and set up a projectile's motion/animation from
 * its owner, then advance to the next state func_022300e8.
 * ent = task->unk8; src = task->unk14->unk8 (owner actor).
 * Copy src->unk34 (slot id, byte) and src->unk38 (long) into ent; OR ent->unk68
 * with the per-slot flag word g_0604ebe4[ent->unk34].
 * Reserve ammo via func_0223c74a(src); if it returns != 0, kill (task->unk0 = 0).
 * If ent->unk34 < 3 double ent->unk64. OR ent->unk62 with 0x40.
 * Launch velocity is read from the table g_06087f84 at index
 * ((ent->unk62 & 63) - 11) * 4 + src->unk44 * 2: ent->unk14 = table[idx],
 * ent->unk22 = table[idx+1] (so the table encodes facing directly - no negate).
 * Position: func_022301c4(task, ent, rec, (ent->unk74+1)&1) where
 * rec = (u8*)g_06097d58 + src->unk34*0x100 + (u8)ent->unk5F*0x10; increment
 * ent->unk74. Set ent->unk5A = 0xB and task->unk0 = func_022300e8.
 *
 * find_entry.py: real entry is 0x2222ff40 (census row 0x22ff28 is the prior
 * function's constant pool).
 * fidelity: HIGH. bsrf resolved 0x22ff72 -> func_0223c74a; next-state ptr =
 * mova(.Lpool_22fff8)+0xF0 = 0x022300e8. */
extern s32 g_actor_slot_masks[];
extern u8 g_06097d58[];
extern s32 g_06087f84[];
extern s32 actor_consume_linked_effect_count(void *oent);
extern void effect_spawn_actor_clone(void *task, void *ent, void *rec, s32 flag);
extern void projectile_sustained_fire_task(void *task);

void projectile_seed_table_task(void *task) {
    void *ent = TASK_PTR(task, work);
    void *src = MP(TASK_PTR(task, parent), 8);
    void *rec;
    s32 idx, cnt;

    M8(ent, 0x34)  = M8(src, 0x34);
    M32(ent, 0x38) = M32(src, 0x38);
    M32(ent, 0x68) |= g_actor_slot_masks[M8(ent, 0x34)];

    if (actor_consume_linked_effect_count(src) != 0) {
        TASK_S32(task, handler) = 0;
        return;
    }

    if (M8(ent, 0x34) < 3)
        M32(ent, 0x64) *= 2;
    M16(ent, 0x62) = (s16)(M16(ent, 0x62) | 0x40);

    idx = ((M16(ent, 0x62) & 63) - 11) * 4 + (M8(src, 0x44) << 1);
    M32(ent, 0x14) = g_06087f84[idx];
    M16(ent, 0x22) = (s16)g_06087f84[idx + 1];

    rec = &g_06097d58[(s32)M8(src, 0x34) * 0x100 + (u8)M8(ent, 0x5F) * 0x10];
    cnt = M32(ent, 0x74) + 1;
    M32(ent, 0x74) = cnt;
    effect_spawn_actor_clone(task, ent, rec, cnt & 1);

    M16(ent, 0x5A) = 0xB;
    TASK_PTR(task, handler) = (void *)projectile_sustained_fire_task;
}
