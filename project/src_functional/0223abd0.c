/* actor_spawn_counted_effect [0x0223abd0]: Spawns the gated counted effect and seeds its count and launch parameters. */
#include "dh.h"
/* 0x0223abd0 - conditional spawn of a counted effect sub-object on entity.
 * n = entity->unk34 - 1. Bail if func_0223e4a0(entity) is false, if
 * (g_0604ebe4[1^n] & *(s32*)0x06098564) != 0, or if (g_0609978c[n] - 1) > 0.
 * Otherwise effect_pool_spawn(func_0223cbb0(1,3,0)); on success q = spawn->unk8:
 *   q->unk56 = 8; q->unk90 += 1; entity->unk74 = 0x1000; entity->unk60 = 0x3000;
 *   tail func_0223a924(q->unk34, 3, &g_06097d58[entity->unk34<<8], entity).
 * fidelity: HIGH - bsrf 0x23e4a0,0x23cbb0 resolved; bra tail 0x23a924. g_0604ebe4
 * index is 1^(unk34-1); g_0609978c is s16 by (unk34-1); masks read from asm. */
#define G32(a) (*(s32 *)(uintptr_t)(a))
extern s32   g_actor_slot_masks[];
extern s16   g_0609978c[];
extern u8    g_06097d58[];
extern s32   effect_pool_make_room(void *e);
extern void *effect_pool_spawn(s32 type, s32 flag, s32 pos);
extern void  effect_pool_update_pose(s32 kind, s32 a1, void *ptr, void *obj);

void actor_spawn_counted_effect(void *entity) {
    s32 n;
    void *spawn;
    void *q;
    if (!effect_pool_make_room(entity)) return;
    n = M8(entity, 0x34) - 1;
    if (g_actor_slot_masks[1 ^ n] & G32(DH_READY_PLAYER_MASK_ADDR)) return;
    if (g_0609978c[n] - 1 > 0) return;
    spawn = effect_pool_spawn(1, 3, 0);
    if (spawn == 0) return;
    q = MP(spawn, 8);
    M16(q, 0x56) = 8;
    M32(q, 0x90) += 1;
    M32(entity, 0x74) = 0x1000;
    M16(entity, 0x60) = 0x3000;
    effect_pool_update_pose(M8(q, 0x34), 3, &g_06097d58[M8(entity, 0x34) << 8], entity);
}
