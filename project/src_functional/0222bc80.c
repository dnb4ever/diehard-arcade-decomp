/* actor_state4_enter [0x0222bc80]: Existing map corroborated by source: (member) state-4 (attack wind-up) setup. e->unk74=0x3000; */
#include "dh.h"
/* 0x0222bc80 actor_state4_enter(entity) - sets up an actor for state 4 (throw/attack
 * wind-up): seeds timers/flags, picks an attack sub-parameter from a doubly-indirect
 * table, and optionally fires func_0223a9d0.
 *   entity->unk74 = 0x3000;
 *   entity->unk60 = (s16)((func_02210020() & 0xFFF) + 0x2200);   (randomised spread)
 *   func_0222acf8(entity);
 *   entity->unkC8 += 0x1000;
 *   entity->unk88 = 7; entity->unk84 = 0x2000; entity->unk43 = 4;
 *   idx = (u16)entity->unk62 >> 13;  if (entity->unk44) idx = (idx + 4) & 7;
 *   entity->unk5C = (**g_06086700)[idx*4 + 1];     (s16 table, double indirection)
 *   entity->unk46 = 0;
 *   if (entity->unk8C & 0x00FFFFFF) { func_0223a9d0(entity, 3); entity->unk45 = 0; }
 *   g_06099780[entity->unk34 - 1] = g_060987BC;    (stamp current frame)
 * fidelity: HIGH - func_02210020 void/u32 and func_0222acf8 void/1-arg exist;
 * func_0223a9d0 undecompiled, called (entity,3) per asm; three bsrf resolved
 * (0x210020, 0x23a9d0). m2c dropped the two bsrf returns (unk60/unk45). */
extern u32  game_random_next(void);
extern void actor_compute_motion_components(void *entity);
extern void actor_commit_effect_handoffs(void *entity, s32 a1);
extern void *g_06086700;    /* -> (-> s16 attack-param table) */
extern s32   g_frame_tick;    /* free-running frame counter */
extern s32   g_06099780[];

void actor_state4_enter(void *entity) {
    s32 idx;
    void *tbl;

    M32(entity, 0x74) = 0x3000;
    M16(entity, 0x60) = (s16)((game_random_next() & 0xFFF) + 0x2200);
    actor_compute_motion_components(entity);
    M32(entity, 0xC8) = M32(entity, 0xC8) + 0x1000;
    M32(entity, 0x88) = 7;
    M32(entity, 0x84) = 0x2000;
    M8(entity, 0x43) = 4;
    idx = MU16(entity, 0x62) >> 13;
    if (M8(entity, 0x44) != 0) {
        idx = (idx + 4) & 7;
    }
    tbl = MP(g_06086700, 0x0);
    M16(entity, ACTOR_ANIM_REQUEST) = M16(tbl, (idx * 4 + 1) * 2);
    M8(entity, 0x46) = 0;
    if (M32(entity, 0x8C) & 0x00FFFFFF) {
        actor_commit_effect_handoffs(entity, 3);
        M8(entity, 0x45) = 0;
    }
    g_06099780[M8(entity, ACTOR_SLOT_ID) - 1] = g_frame_tick;
}
