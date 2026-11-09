/* actor_spawn_secondary_effect [0x0223ab5c]: Spawns a selected secondary effect, clears launch speed and attaches it to the actor part pose. */
#include "dh.h"
/* 0x0223ab5c - sibling of func_0223ab04: arm effect sub-object with cleared fields.
 * If func_0223e4a0(entity) true and effect_pool_spawn(func_0223cbb0) with type
 * arg1+0xe succeeds, read q = spawn->unk8, set entity->unk74 = 0 and
 * entity->unk60 = 0x2000, then tail-call func_0223a924(q->unk34, 3,
 * &g_06097d58[(entity->unk34<<8)+0x50], entity).
 * fidelity: HIGH - bsrf 0x23e4a0,0x23cbb0 (type = arg1+14 via delay-slot add);
 * bra tail 0x23a924. null check on spawn result. */
extern u8 g_06097d58[];
extern s32   effect_pool_make_room(void *e);
extern void *effect_pool_spawn(s32 type, s32 flag, s32 pos);
extern void  effect_pool_update_pose(s32 kind, s32 a1, void *ptr, void *obj);

void actor_spawn_secondary_effect(void *entity, s32 arg1) {
    void *spawn;
    void *q;
    if (!effect_pool_make_room(entity)) return;
    spawn = effect_pool_spawn(arg1 + 14, 3, 0);
    if (spawn == 0) return;
    q = MP(spawn, 8);
    M32(entity, 0x74) = 0;
    M16(entity, 0x60) = 0x2000;
    effect_pool_update_pose(M8(q, 0x34), 3, &g_06097d58[(M8(entity, 0x34) << 8) + 0x50], entity);
}
