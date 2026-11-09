/* aim_effect_spawn [0x0223e3d0]: Existing map corroborated by source: spawn tracking/aim effect (no-op at count>=7). saves g_06096248[u34*0x20]+0xE to g_060e11ea[u34], loads new from g_060e10a8[arg1][entity->unk68]; first free slot -> func_0220ff0a(g_06099580, func_0223db2a); slot arrays set; new entity unk34=slot unk38=0x20000; caller entity unk8C|=2, unk6D=1, unk6F=slot. bsrf+mova/add resolved (func_0220ff0a, installed handler func_0223db2a). ; v1.09 corrected draft binding, work/decomp5/entries.asm */
#include "dh.h"
#include "dh_work.h"
/* func_0223e3d0(entity, slot_src): late-join hook - register a partner actor.
 * Bails if the active count at g_060E0E88 has reached 7. Swaps the current action param
 * word (g_06096248[unk34] + 14) into g_060E11EA[unk34], loading the replacement from
 * table g_060E10A8[arg1*8 + unk68*2]. Finds a free slot, spawns an actor via
 * func_0220ff0a(*g_06099580, handler func_0223db2a), records arg1/0/obj into the
 * g_060E0E88 slot arrays (+0x84/+0xA4/+0xC4), stamps the spawned entity
 * (unk34 = slot, unk38 = 0x20000) and sets entity->unk8C |= 2, unk6D = 1, unk6F = slot.
 * fidelity: HIGH - all offsets/strides (32-byte g_06096248 records, 4-byte slot arrays,
 *   8/2 table strides) verified; func_0220ff0a (braf) and template func_0223db2a
 *   (mova+add) resolved. */
extern void *task_spawn_actor_work(void *parent, void *handler);
extern u8 g_060E0E88[];
extern u8 g_06096248[];
extern u8 g_060E11EA[];
extern u8 g_060E10A8[];
extern u8 g_06099580[];

void aim_effect_spawn(void *ent, s32 arg1) {
    s32 slot;
    void *obj, *oe;

    if (M32(g_060E0E88, 0) >= 7) {
        return;
    }
    M16(g_060E11EA, ACTOR_WORK(ent)->slot_id * 4) = M16(g_06096248, ACTOR_WORK(ent)->slot_id * 32 + 14);
    M16(g_06096248, ACTOR_WORK(ent)->slot_id * 32 + 14) =
        M16(g_060E10A8, arg1 * 8 + (u16)M16(ent, 0x68) * 2);

    slot = -1;
    do {
        slot++;
    } while (M32(g_060E0E88, 0x84 + slot * 4) != 0);

    obj = task_spawn_actor_work(MP(g_06099580, 0), (void *)0x06041B2A /* handler func_0223db2a */);
    M32(g_060E0E88, 0) += 1;
    M32(g_060E0E88, 0xA4 + slot * 4) = 0;
    M32(g_060E0E88, 0x84 + slot * 4) = arg1;
    MP(g_060E0E88, 0xC4 + slot * 4) = obj;
    oe = MP(obj, 8);
    ACTOR_WORK(oe)->slot_id = (u8)slot;
    ACTOR_M32(oe, render_flags) = 0x20000;
    ACTOR_M32(ent, interaction_flags) |= 2;
    ACTOR_M8(ent, secondary_effect_type) = 1;
    ACTOR_M8(ent, secondary_effect_slot) = ACTOR_WORK(oe)->slot_id;
}
