/* actor_load_recovery_pose [0x0223ac50]: Selects an eight-byte recovery descriptor, requests its animation and enters handler 0x28. */
#include "dh.h"
/* 0x0223ac50 - load a recovery/aim entry into entity from one of two 8-byte tables.
 * If entity->unkE4 (s32) == 6: idx = (entity->unkE0 & 0x01000000) ? 1 : 0, entry =
 *   &g_0608ce64[idx]; else entry = &g_0608ce04[entity->unk6D (signed byte)].
 * Then entity->unk5C = entry->s16@0; entity->unk41 = 1;
 *   entity->unk90 (s32) = (u16)entry->s16@4; entity->unk43 = 0x28.
 * fidelity: HIGH - *8 stride (shll2;shll); @+4 zero-extended (extu.w); last store in
 * rts delay slot. Tables are 8-byte records. */

void actor_load_recovery_pose(void *entity) {
    u8 *entry;
    if (M32(entity, 0xe4) == 6) {
        s32 idx = (M32(entity, 0xe0) & 0x01000000) ? 1 : 0;
        entry = (u8 *)((uintptr_t)0x0608ce64 + idx * 8);
    } else {
        entry = (u8 *)((uintptr_t)0x0608ce04 + M8(entity, 0x6d) * 8);
    }
    M16(entity, 0x5c) = M16(entry, 0);
    M8(entity, 0x41) = 1;
    M32(entity, 0x90) = MU16(entry, 4);
    M8(entity, 0x43) = 0x28;
}
