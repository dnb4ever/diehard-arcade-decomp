/* projectile_launch_task [0x022317c0]: Existing map corroborated by source: launch init: reserves ammo func_0223c74a(oent); seeds pos/vel; next -> func_02231864 */
#include "dh_work.h"
#include "dh.h"
/* 0x022317c0 (task handler, init state) - launch a tossed/arc object from a source
 * actor after reserving its ammo. entity = task->unk8; oent = task->unk14->unk8.
 * Reserve via func_0223c74a(oent); if it fails (!=0) clear task and return.
 * Else copy launch pos from g_06097D58[(oent->unk34<<8) + (u8)entity->unk5F*0x10]
 * into entity->unk8/C/10, inherit oent->unk34/unk38, OR per-slot flags
 * g_0604EBE4[unk34] into unk68, set unk14 = oent->unk44 ? -0x2000 : 0x2000,
 * unk18 = -0x1000, unk5A = unk58, unk30 = 0x1A2, unk6C = 0x40000, advance
 * task->unk0 to func_02231864.
 * fidelity: HIGH - unk34 byte, unk5F u8, unk5A/unk30 word, others long.
 * bsrf cart 0x23c74a -> func_0223c74a; next-handler ptr resolved 0x2317c0 -> 0x02231864. */
extern s32  actor_consume_linked_effect_count(void *oent);
extern u8   g_06097d58[];
extern s32  g_actor_slot_masks[];
extern void projectile_fly_task(void *task);

void projectile_launch_task(void *task) {
    void *ent = TASK_PTR(task, work);
    void *oent = MP(TASK_PTR(task, parent), 8);
    u8 *src = &g_06097d58[(M8(oent, 0x34) << 8) + MU8(ent, 0x5F) * 0x10];
    if (actor_consume_linked_effect_count(oent) != 0) {
        TASK_S32(task, handler) = 0;
        return;
    }
    M32(ent, 0x8)  = M32(src, 0);
    M32(ent, 0xC)  = M32(src, 4);
    M32(ent, 0x10) = M32(src, 8);
    M8(ent, 0x34)  = M8(oent, 0x34);
    M32(ent, 0x38) = M32(oent, 0x38);
    M32(ent, 0x68) |= g_actor_slot_masks[M8(ent, 0x34)];
    M32(ent, 0x14) = (M8(oent, 0x44) != 0) ? -0x2000 : 0x2000;
    M32(ent, 0x18) = -0x1000;
    M16(ent, 0x5A) = M16(ent, 0x58);
    M16(ent, 0x30) = 0x1A2;
    M32(ent, 0x6C) = 0x40000;
    TASK_PTR(task, handler) = (void *)projectile_fly_task;
}
