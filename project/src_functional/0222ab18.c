#include "dh_work.h"
#include "dh.h"
/* 0x022ab18 (actor_apply_damage) - subtract `amount` from an actor's HP word (unk56); on drain,
 * raise a late-join request. Kin to func_0222ac60 (actor_hit_countdown). entity=r4, amount=r5.
 * Two symmetric paths on the actor's slot index unk34:
 *   unk34 > 2 (extra/boss slot): bail if amount==0; if (unk38 & 0xFE00) and g_06098564 != 3 then
 *     amount *= 2. unk56 -= amount; return if still > 0. Clamp unk56 = 0, then if the live count
 *     g_06099584[+8]==1, two-player flag g_06098670==1 and progress g_06098830>=6: set
 *     g_06098830 = 6, g_0609882C->unkD = 1, and on (g_0609882C->unk8): unkAE = unk34,
 *     unkB0 = func_02210020(&g_0609882C), unkB2 = (unk38 & 0xFE00) ? 1 : 0.
 *   unk34 <= 2 (player slot): return unless unk4A > 2. unk56 -= amount; return if > 0. Clamp 0,
 *     then with slot idx = (unk34==1)?2:1, if g_06099590[idx]==0, g_06098670==1 and g_06098830>=5:
 *     set g_06098830 = 5, g_0609882C->unkD = 1, and on (g_0609882C->unk8): unkAE = unk34,
 *     unkB0 = func_02210020(&g_0609882C), unkB2 = 0.
 * Always clears g_06099798 = 0 on the way out.
 * fidelity: HIGH - bsrf tails resolved to func_02210020 (0x210020). unk34/4A byte, unk56 word,
 * unk38 long; unkAE/B0/B2 word on the request object; table/flags longs, g_06099798 word. */
extern s32  game_random_next(void *p);
extern u8   g_actor_slot_registry[];    /* entity table: +8 live count, +0xC id[] */
extern s32  g_actor_slot_ids[];    /* active-slot flags (table + 0xC) */
extern s32  g_ready_player_mask;
extern s32  g_play_session_mode;      /* two-player flag */
extern s32  g_view_request_priority;      /* progress */
extern void *g_view_task_legacy;     /* late-join request object pointer */
extern s16  g_06099798;

void actor_apply_damage(void *entity, s32 amount) {
    if ((s8)M8(entity, ACTOR_SLOT_ID) > 2) {
        if (amount == 0)
            goto done;
        if ((ACTOR_M32(entity, render_flags) & 0xFE00) != 0 && g_ready_player_mask != 3)
            amount <<= 1;
        ACTOR_M16(entity, health) = (s16)(ACTOR_M16(entity, health) - amount);
        if ((s16)ACTOR_M16(entity, health) > 0)
            goto done;
        ACTOR_M16(entity, health) = 0;
        if (M32(g_actor_slot_registry, 8) == 1 && g_play_session_mode == 1 && g_view_request_priority >= 6) {
            void *obj = TASK_PTR(g_view_task_legacy, work);
            g_view_request_priority = 6;
            M8(g_view_task_legacy, 0xD) = 1;
            M16(obj, 0xAE) = (s8)M8(entity, ACTOR_SLOT_ID);
            M16(obj, 0xB0) = (s16)game_random_next(&g_view_task_legacy);
            M16(obj, 0xB2) = (ACTOR_M32(entity, render_flags) & 0xFE00) ? 1 : 0;
        }
    } else {
        s32 idx;
        if ((s8)ACTOR_M8(entity, hit_source_slot) <= 2)
            goto done;
        ACTOR_M16(entity, health) = (s16)(ACTOR_M16(entity, health) - amount);
        if ((s16)ACTOR_M16(entity, health) > 0)
            goto done;
        ACTOR_M16(entity, health) = 0;
        idx = ((s8)M8(entity, ACTOR_SLOT_ID) == 1) ? 2 : 1;
        if (g_actor_slot_ids[idx] == 0 && g_play_session_mode == 1 && g_view_request_priority >= 5) {
            void *obj = TASK_PTR(g_view_task_legacy, work);
            g_view_request_priority = 5;
            M8(g_view_task_legacy, 0xD) = 1;
            M16(obj, 0xAE) = (s8)M8(entity, ACTOR_SLOT_ID);
            M16(obj, 0xB0) = (s16)game_random_next(&g_view_task_legacy);
            M16(obj, 0xB2) = 0;
        }
    }
done:
    g_06099798 = 0;
}
