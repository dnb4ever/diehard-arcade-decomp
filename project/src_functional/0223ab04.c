/* actor_arm_effect_with_motion [0x0223ab04]: Free-slot query and effect spawn; seeds speed/pitch then effect handoff. */
#include "dh.h"
/* 0x0223ab04 - arm a "muzzle/effect" sub-object on entity (variant with set fields).
 * If func_0223e4a0(entity) (free-slot query) is true and effect_pool_spawn
 * (func_0223cbb0) succeeds, read q = spawn->unk8, set entity->unk74 = 0x1000 and
 * entity->unk60 = 0x1800, then tail-call func_0223a924(q->unk34, 3,
 * &g_06097d58[(entity->unk34<<8)+0x50], entity).
 * fidelity: HIGH - two bsrf calls resolved (0x23e4a0,0x23cbb0); bra tail to 0x23a924.
 * null check is on the spawn result, not on q. arg1 feeds effect type unchanged. */
extern u8 g_06097d58[];
extern s32   effect_pool_make_room(void *e);
extern void *effect_pool_spawn(s32 type, s32 flag, s32 pos);
extern void  effect_pool_update_pose(s32 kind, s32 a1, void *ptr, void *obj);

void actor_arm_effect_with_motion(void *entity, s32 arg1) {
    void *spawn;
    void *q;
    if (!effect_pool_make_room(entity)) return;
    spawn = effect_pool_spawn(arg1, 3, 0);
    if (spawn == 0) return;
    q = MP(spawn, 8);
    M32(entity, 0x74) = 0x1000;
    M16(entity, 0x60) = 0x1800;
    effect_pool_update_pose(M8(q, 0x34), 3, &g_06097d58[(M8(entity, 0x34) << 8) + 0x50], entity);
}
