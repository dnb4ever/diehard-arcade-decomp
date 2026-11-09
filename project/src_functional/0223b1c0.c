#include "dh_work.h"
/* actor_begin_hold_effect [0x0223b1c0]: Releases a pending primary handoff, updates the held effect count and requests the slot animation. */
#include "dh.h"
/* 0x0223b1c0 (actor state generator, grab/hold variant) - shared callee; a caller does
 * state = func_0223b1c0(entity) + 0x21 (this always returns 1, i.e. state 0x22). Side effects:
 * fetches the linked partner object (g_060E0F4C[unk6F] -> +8). If a grab is active (unk6C!=0),
 * restores the per-slot anim word (g_06096250[unk34<<5] = g_060E11E8[unk34]), marks the queue
 * slot g_060E0F2C[unk6E]=5, clears the type-flag mask in unk8C (&= ~g_0604EBE4[unk6C]), and
 * clears unk6E/unk6C. Then nudges the partner's facing angle (obj->unk56 += g_0608CEF4[unk6D])
 * and copies this slot's anim id g_06096C74[unk34] into unk5C. Returns 1.
 * fidelity: HIGH - unk6F/6C/6E/6D/34 signed byte, unk8C long, unk5C word, obj->unk56 word,
 * mask table g_0604EBE4 long, g_060E11E8/g_0608CEF4 word, g_060E0F2C/g_06096C74 long.
 * No computed branches. */
extern void *g_060E0F4C[];   /* per-slot partner object pointers, index unk6F */
extern s16   g_060E11E8[];
extern u8    g_06096250[];   /* 0x20-byte stride per-slot records */
extern s32   g_060E0F2C[];
extern s32   g_actor_slot_masks_legacy[];   /* per-type state-flag masks */
extern s16   g_0608CEF4[];
extern s32   g_06096C74[];

s32 actor_begin_hold_effect(void *entity) {
    void *obj = MP(g_060E0F4C[ACTOR_M8(entity, secondary_effect_slot)], 8);
    if (ACTOR_M8(entity, primary_effect_type) != 0) {
        s8 idx = ACTOR_M8(entity, slot_id);
        M16(&g_06096250[idx << 5], 0) = g_060E11E8[idx * 2];
        g_060E0F2C[ACTOR_M8(entity, primary_effect_slot)] = 5;
        ACTOR_M32(entity, interaction_flags) &= ~g_actor_slot_masks_legacy[ACTOR_M8(entity, primary_effect_type)];
        ACTOR_M8(entity, primary_effect_slot) = 0;
        ACTOR_M8(entity, primary_effect_type) = 0;
    }
    M16(obj, 0x56) += g_0608CEF4[ACTOR_M8(entity, secondary_effect_type)];
    ACTOR_M16(entity, animation_requested) = g_06096C74[ACTOR_M8(entity, slot_id)];
    return 1;
}
