/* actor_state_gen_hitreact [0x0223a740]: Existing map corroborated by source: actor action-state generator: unk6D=1, swaps saved anim g_060E11E8/g_06096248, loads hit-anim g_0608CE74; calls func_0223b254/func_0223e104; tail func_022260c8 */
#include "dh.h"
/* 0x0223a740 (actor action-state generator) - begin action slot 1 (unk6D=1).
 * entity->unk8C |= 2; unk6D=1; unk6F=unk6B; seed saved anim word
 * g_060E11E8[slot]+2 from g_06096248[slot]+14 (slot=unk34, 32B stride); load facing
 * anim g_060E10B0[(u16)unk68] into g_06096248[slot]+14; unk45=1; unk8C &= 0x00FFFFFF;
 * clear grab queue unk6A/unk6B; run func_0223b254(entity,1,anim_rec). For player
 * slots (unk34 < 3): zone rec g_060E0E58[(unk34-1)] +10=0 / +8=12, call
 * func_0223e104(entity), load hit-anim g_0608CE74[unk6D] -> g_06096C74[unk34] &
 * entity->unk5C, then tail-call func_022260c8(entity).
 * fidelity: HIGH - unk34 byte, unk68 u16, unk6D byte; anim words s16; masks long.
 * bsrf cart 0x223e104 -> func_0223e104; braf cart 0x2260c8 -> func_022260c8. */
extern u8  g_06096248[];
extern u8  g_060e11e8[];
extern s16 g_060e10b0[];
extern u8  g_060e0e58[];
extern s16 g_0608ce74[];
extern s32 g_06096c74[];
extern void actor_load_part_offset(void *entity, s32, void *);
extern void hud_queue_action_label(void *entity);
extern void actor_fill_pose_history(void *entity);

void actor_state_gen_hitreact(void *entity) {
    s32 slot = M8(entity, 0x34);
    void *anim = &g_06096248[slot * 32];
    M32(entity, 0x8C) |= 2;
    M8(entity, 0x6D) = 1;
    M8(entity, 0x6F) = M8(entity, 0x6B);
    M16(&g_060e11e8[slot * 4], 2) = M16(anim, 14);
    M16(anim, 14) = g_060e10b0[MU16(entity, 0x68)];
    M8(entity, 0x45) = 1;
    M32(entity, 0x8C) &= 0x00FFFFFF;
    M8(entity, 0x6A) = 0;
    M8(entity, 0x6B) = 0;
    actor_load_part_offset(entity, 1, anim);
    if (M8(entity, 0x34) < 3) {
        u8 *zone = &g_060e0e58[(M8(entity, 0x34) - 1) * 16];
        s16 hit;
        M16(zone, 10) = 0;
        M16(zone, 8) = 12;
        hud_queue_action_label(entity);
        hit = g_0608ce74[M8(entity, 0x6D)];
        g_06096c74[M8(entity, 0x34)] = hit;
        M16(entity, 0x5C) = hit;
        actor_fill_pose_history(entity);
    }
}
