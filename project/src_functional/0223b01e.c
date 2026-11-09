/* actor_begin_throw_effect [0x0223b01e]: Consumes the selected inventory counter, spawns a thrown object and updates animation channels. */
#include "dh.h"
/* 0x0223b01e (actor_state_gen_throw) - throw / projectile-spawn state generator. Returns 6;
 * the caller (0x022297d8) does state = ret + 0x21 = 0x27. Behaviour:
 *  - loads the move record g_0608CDCC[entity->unk6D + entity->unk98] (8-byte stride): the action
 *    id rec->unk2 (-> local), and rec->unk6 -> entity->unk90 (frame count); clears unk98;
 *  - decrements a per-arm projectile counter: unk72 (and var 3) when unk6D==1, else unk73 (var 4);
 *  - SPAWNS a projectile actor via spawn_obj_a(g_06099580, template@0x0223D1B6) and seeds the
 *    spawned entity's world pos (unk8/C/10 from the owner's slot record g_06097D88[unk34],
 *    0x100 stride), unk30 = 0x30B, unk14 = -0x1000/0x1000 by facing flag unk44;
 *  - for player slots (unk34 < 3) finds the free sub-slot (first of 6 in g_060E0E78[unk34-1]
 *    whose byte != var) and marks the hit-record g_060E0E58[unk34-1] (unkA = slot, unk8 = 0x10);
 *  - validates the action via func_0223cbb0(action_id,0,0): if NULL, bail returning 6; otherwise
 *    latch the throw into the actor (unk6C = action id, unk6E = spawned entity's unk34,
 *    unk8C |= type mask g_0604EBE4[action], and swap anim-slot words g_060E11E8[unk34] <->
 *    g_06096248[unk34].unk8 using the per-move anim table g_060E1148).
 * NOTE (spawn system): the spawn goes through spawn_obj_a with g_06099580 as the pool/list
 * context; if that allocator returns an empty/NULL object the whole chain stalls (relevant to the
 * port's empty-spawn-list investigation).
 * fidelity: HIGH - widths from the loads (unk6D/72/73/44/34 byte, unk98/90/8C/14 + slot pos long,
 * action id + anim words 16-bit); spawn template pointer resolved via ptr_resolve.py
 * (mova 0x23b090 + 0x2126 -> 0x0223d1b6); computed calls via braf_rewrite.py. */
extern void *task_spawn_child_work88(s32 ctx, void *template);  /* spawn_obj_a */
extern void *effect_pool_spawn(u16 action, s32 b, s32 c);
extern s32  g_06099580;      /* spawn pool/list context */
extern u8   g_0608CDCC[];    /* move/action record table, 8-byte stride */
extern u8   g_06097D88[];    /* per-slot world-pos record, 0x100 stride */
extern u8   g_06096248[];    /* per-slot actor record, 0x20 stride */
extern s16  g_060E11E8[];    /* per-slot saved anim word (4-byte stride) */
extern s32  g_actor_slot_masks_legacy[];    /* per-type/action flag masks */
extern u8   g_060E0E58[];    /* per-player hit record, 0x10 stride */
extern u8   g_060E0E78[];    /* per-player sub-slot table, 8-byte stride */
extern s16  g_060E1148[];    /* per-move anim word table (8*action stride) */

s32 actor_begin_throw_effect(void *entity) {
    void *rec = g_0608CDCC + (M8(entity, 0x6D) + M32(entity, 0x98)) * 8;
    u16 action = MU16(rec, 2);
    s32 var;
    void *sp_ent;
    void *slotrec;
    void *result;

    M32(entity, 0x98) = 0;
    M32(entity, 0x90) = MU16(rec, 6);
    if (M8(entity, 0x6D) == 1) {
        M8(entity, 0x72) = (s8)(M8(entity, 0x72) - 1);
        var = 3;
    } else {
        var = 4;
        M8(entity, 0x73) = (s8)(M8(entity, 0x73) - 1);
    }
    sp_ent = MP(task_spawn_child_work88(g_06099580, (void *)0x060411B6 /* PC-relative template in HWRAM */), 8);
    slotrec = g_06097D88 + (M8(entity, 0x34) << 8);
    M32(sp_ent, 8)  = M32(slotrec, 0);
    M32(sp_ent, 0xC) = M32(slotrec, 4);
    M32(sp_ent, 0x10) = M32(slotrec, 8);
    M16(sp_ent, 0x30) = 0x30B;
    M32(sp_ent, 0x14) = M8(entity, 0x44) ? -0x1000 : 0x1000;
    if (M8(entity, 0x34) < 3) {
        s16 i = 0;
        void *hit = g_060E0E58 + (M8(entity, 0x34) - 1) * 0x10;
        u8 *tbl = g_060E0E78 + (M8(entity, 0x34) - 1) * 8;
        while (tbl[i] != var) {
            i++;
            if (i >= 6)
                break;
        }
        M16(hit, 0xA) = i;
        M16(hit, 8) = 0x10;
    }
    result = effect_pool_spawn(action, 0, 0);
    if (result == 0)
        return 6;
    M8(entity, 0x6C) = (s8)action;
    M8(entity, 0x6E) = M8(MP(result, 8), 0x34);
    M32(entity, 0x8C) |= g_actor_slot_masks_legacy[action];
    M16(g_060E11E8, M8(entity, 0x34) * 4) = M16(g_06096248 + (M8(entity, 0x34) << 5), 8);
    M16(g_06096248 + (M8(entity, 0x34) << 5), 8) =
        M16(g_060E1148, action * 8 + MU16(entity, 0x68) * 2);
    return 6;
}
