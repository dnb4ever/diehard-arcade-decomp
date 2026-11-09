/* actor_enter_retarget_aim [0x0223af0c]: Clears speed, selects opposing heading and aim pitch, resets effect and chooses pose. */
#include "dh.h"
/* 0x0223af0c - enter a retarget/aim state on entity.
 *   entity->unk74 = 0; entity->unk62 = (entity->unk44 != 0) ? 0xC000 : 0x4000;
 *   entity->unk60 = 0x1800; entity->unk45 = 0; func_0223a9d0(entity, 3);
 * If entity->unk6D == 0 AND (entity->unk8C & 0x02000000): func_0223a6dc(entity);
 *   entity->unk41 = 1; entity->unk43 = 13.
 * Else: entity->unk84 = 1; entity->unk88 = 0;
 *   entity->unk5C = (s16)g_06096C74[entity->unk34]; entity->unk41 = 1; entity->unk43 = 1.
 * fidelity: HIGH - direct bsr 0x23a9d0 (a1=3 in r5) and 0x23a6dc; unk62 word store
 * truncates 0xC000; unk5C word store truncates g_06096C74 long. */
extern s32  g_06096C74[];
extern void actor_commit_effect_handoffs(void *entity, s32 a1);
extern void actor_load_pose_animation(void *entity);

void actor_enter_retarget_aim(void *entity) {
    M32(entity, 0x74) = 0;
    M16(entity, 0x62) = (s16)((M8(entity, 0x44) != 0) ? 0xC000 : 0x4000);
    M16(entity, 0x60) = 0x1800;
    M8(entity, 0x45) = 0;
    actor_commit_effect_handoffs(entity, 3);
    if (M8(entity, 0x6D) == 0 && (M32(entity, 0x8C) & 0x02000000)) {
        actor_load_pose_animation(entity);
        M8(entity, 0x41) = 1;
        M8(entity, 0x43) = 13;
    } else {
        M32(entity, 0x84) = 1;
        M32(entity, 0x88) = 0;
        M16(entity, 0x5C) = (s16)g_06096C74[M8(entity, 0x34)];
        M8(entity, 0x41) = 1;
        M8(entity, 0x43) = 1;
    }
}
