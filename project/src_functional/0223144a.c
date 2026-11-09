/* projectile_track_task [0x0223144a]: Existing map corroborated by source: tracked projectile toward partner (src rec g_06097D58); kill on expiry/target-blocked(unk84&0x4B800); drawlist tail */
#include "dh_work.h"
#include "dh.h"
/* 0x0223144a (task handler) - tracked projectile step toward partner.
 * entity = task->unk8; oent = task->unk14->unk8 (target entity).
 * src = &g_06097D58[(oent->unk34 << 8) + (u8)entity->unk5F * 0x10] (per-slot anim/pos rec).
 * If entity->unk5D != 0 run func_0222f3c0(entity). Decrement entity->unk5A; kill when
 * expired (<=0) or target is in a blocking state (oent->unk84 & 0x4B800): flag
 * entity->unk78->unk12 = 1, optional sound sound_queue_push(0xA0130DFF) when g_060E0DDC,
 * clear task. Otherwise snapshot pos, load pos from src, tail drawlist_prepend.
 * fidelity: HIGH - unk34 byte, unk5F u8, unk5A word, unk84 long mask 0x4B800.
 * bsrf 0x22f3c0/0x215f6c; braf 0x22eea2 -> func_0222eea2. */
extern void actor_publish_linked_slot_value(void *entity);
extern void sound_queue_push(s32 cmd);        /* sound_queue_push */
extern void collision_queue_actor(void *entity);   /* drawlist_prepend */
extern u8   g_06097d58[];
extern s32  g_sound_effects_enabled;

void projectile_track_task(void *task) {
    void *ent = TASK_PTR(task, work);
    void *oent = MP(TASK_PTR(task, parent), 8);
    u8 *src = &g_06097d58[(M8(oent, 0x34) << 8) + MU8(ent, 0x5F) * 0x10];
    s16 n;
    if (M8(ent, 0x5D) != 0)
        actor_publish_linked_slot_value(ent);
    n = M16(ent, 0x5A);
    M16(ent, 0x5A) = (s16)(n - 1);
    if (n <= 0 || (M32(oent, 0x84) & 0x4B800)) {
        M8(MP(ent, 0x78), 0x12) = 1;
        if (g_sound_effects_enabled != 0)
            sound_queue_push(0xA0130DFF);
        TASK_S32(task, handler) = 0;
        return;
    }
    M32(ent, 0x4C) = M32(ent, 0x8);
    M32(ent, 0x50) = M32(ent, 0xC);
    M32(ent, 0x54) = M32(ent, 0x10);
    M32(ent, 0x8)  = M32(src, 0);
    M32(ent, 0xC)  = M32(src, 4);
    M32(ent, 0x10) = M32(src, 8);
    collision_queue_actor(ent);
}
