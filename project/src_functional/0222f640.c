/* projectile_track_init_task [0x0222f640]: Existing map corroborated by source: init/seed muzzle-track projectile: load pos from record g_06097D58[owner.unk34,unk5F]; copy owner unk34/unk38; unk68|=g_0604EBE4[anim]; unk62|=0x80; unk58--; install (src symbols_gen7.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x0222f640 (projectile task handler, init/seed state) - seed a muzzle/track
 * projectile from its owner and advance to func_0222f6a8.
 * ent = task->unk8; owner = task->unk14->unk8.
 *
 * Loads the start position from the animation record g_06097d58 (indexed by the
 * owner's anim id unk34 and the entity frame unk5F), copies the owner's anim id
 * (unk34) and flags (unk38), ORs g_0604ebe4[anim] into unk68, sets bit 7 of
 * unk62, decrements lifetime unk58, and installs the next state func_0222f6a8.
 *
 * find_entry.py: real entry is 0x0222f640 (census rows 0x22f628 and 0x22f8b6
 * are both pool-contaminated aliases of this one function).
 * fidelity: HIGH - struct offsets, the record index, the field copies and the
 * next-state pointer (= 0x22f6a8, mova+0x8) all read from ROM asm; no computed
 * branches (braf_resolve.py reports none).
 */
extern u8  g_06097d58[];
extern s32 g_actor_slot_masks[];
extern void projectile_phase_track_task(void *task);

void projectile_track_init_task(void *task) {
    void *ent   = TASK_PTR(task, work);
    void *owner = MP(TASK_PTR(task, parent), 8);
    void *rec   = &g_06097d58[(s32)M8(owner, 0x34) * 0x100
                              + (u8)M8(ent, 0x5F) * 0x10];

    M32(ent, 0x08) = M32(rec, 0x00);
    M32(ent, 0x0C) = M32(rec, 0x04);
    M32(ent, 0x10) = M32(rec, 0x08);

    M8(ent, 0x34)  = M8(owner, 0x34);
    M32(ent, 0x38) = M32(owner, 0x38);
    M32(ent, 0x68) |= g_actor_slot_masks[M8(ent, 0x34)];
    M16(ent, 0x62) = (s16)(M16(ent, 0x62) | 0x80);
    M16(ent, 0x58) = M16(ent, 0x58) - 1;

    TASK_PTR(task, handler) = (void *)projectile_phase_track_task;
}
