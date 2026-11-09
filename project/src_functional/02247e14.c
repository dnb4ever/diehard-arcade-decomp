/* enemy_choose_engagement [0x02247e14]: Resolves partner distance and facing before committing an engagement move or response. */
#include "dh_work.h"
#include "dh.h"
/* 0x02247e14 - func_02247e14(entity): combat engagement decider for a paired
 * actor. Resolves the partner, measures separation, picks a facing, then either
 * commits a queued move, enters a scripted reaction, or defers to the per-tick
 * dispatch. Returns 1 when it committed a move/config, else 0.
 *   t = g_06099590[entity->0x120];
 *   if (t == 0) { func_022469f2(entity); return 0; }   // no partner
 *   pe   = *(g_060995b0[t] + 8);                        // partner entity
 *   dx   = |entity->8 - pe->8| - 0x6666; if (dx < 0) dx = 0;
 *   band = (dx & 0x1C000) >> 14;                        // 0..7
 *   facing = (entity->8 >= pe->8) ? 1 : 0;
 *   if (func_02245fc4(entity, pe)) {                    // gate open
 *     if (band >= 3) { func_022469f2(entity); entity->0xC4 = 8; return 0; }
 *     if (entity->68 != facing) {
 *       actor_set_state_7(entity);
 *       entity->92 = *(u16*)(entity->0x160); return 0;
 *     }
 *     p = (u8*)0x060972e0 + entity->52 * 32;
 *     p[0] = (u16)entity->0x14C; p[2] = 0x8000;
 *     actor_init_from_cfg(entity, p); return 1;
 *   }
 *   r = rng_next();
 *   if ((entity->56 & 0xFA40) && (pe->0x84 & 0x400) && entity->68 != pe->68
 *        && (entity->0x8C & 0x3FFFF) == 0 && (r & 7) <= entity->0xE8) {
 *     actor_enter_state12(entity); return 0;
 *   }
 *   if (entity->68 != facing) {
 *     q = (void*)entity->0x160;
 *     switch (func_022460c8(entity)) {                   // 0..3
 *       case 0: actor_set_state_7(entity); entity->92 = *(u16*)q; break;
 *       case 1: actor_set_state_7(entity);
 *               entity->92 = *(u16*)(q + (rng_next() & 3) * 2);
 *               entity->0xE0 |= 0x100000; break;
 *       case 2: func_02247a16(entity); break;
 *       case 3: func_02247a3e(entity); break;
 *     }
 *     return 0;
 *   }
 *   row = *(*(entity->0x15C) + band*4);                 // config list for band
 *   cfg = row[(r & 3)];                                  // (word index)
 *   if (cfg > 0) { actor_init_from_cfg(entity, cfg); return 1; }
 *   g_06090250[~cfg](entity); return 0;                  // negative -> fn table
 * fidelity: HIGH - all 9 resolved branches plus the one table-indexed indirect
 * call (the reported "TABLE/COMPUTED" braf is the g_06090250 fn-ptr dispatch).
 * NOTE the inverted sense on entity->0x8C: the state-12 gate requires that field
 * masked by 0x3FFFF to be ZERO (bf skip). The separation band uses 14 arithmetic
 * right shifts of (dx & 0x1C000); dx is non-negative after the abs+clamp. r (the
 * early rng) is reused for both the case-1 entry and the band config index
 * (rng & 3). entity->0x15C is read once at the prologue and used only on the
 * entity->68 == facing path (the band<3 branch reloads entity->0x160 instead but
 * returns, so the prologue value survives). Partner pos/field +8 is axis-0. */
typedef void (*combat_fn)(void *entity);

extern s32  enemy_reset_idle_or_walk(void *entity);            /* 0x022469f2 per-tick dispatch */
extern s32  enemy_classify_attack_timing(void *entity, void *pe);  /* 0x02245fc4 engagement gate */
extern void actor_set_state_7_entry(void *entity);            /* 0x0224840a actor_set_state_7 */
extern void actor_init_from_config_raw_entry(void *entity, void *cfg); /* 0x02244988 actor_init_from_cfg */
extern void enemy_enter_random_idle18(void *entity);            /* 0x02249ea0 actor_enter_state12 */
extern s32  enemy_classify_linked_effect(void *entity);            /* 0x022460c8 phase classifier 0..3 */
extern void enemy_enter_dash_recovery_a(void *entity);            /* 0x02247a16 */
extern void enemy_enter_dash_recovery_b(void *entity);            /* 0x02247a3e */
extern s32  game_random_next(void);                    /* 0x02210020 rng_next */
extern s32      g_actor_slot_ids[];                       /* partner-index table */
extern void    *g_actor_slot_tasks_legacy[];                       /* partner-object pointer table */
extern combat_fn g_06090250[];                      /* negative-code fn-ptr table */

s32 enemy_choose_engagement(void *entity)
{
    s32 sp0 = ACTOR_M32(entity, reaction_config_tables);
    s32 t = g_actor_slot_ids[ACTOR_M32(entity, target_slot)];
    void *pe;
    s32 dx, band, facing, r;

    if (t == 0) {
        enemy_reset_idle_or_walk(entity);
        return 0;
    }

    pe = TASK_PTR(g_actor_slot_tasks_legacy[t], work);

    dx = ACTOR_M32(entity, position_x) - M32(pe, 8);
    if (dx < 0)
        dx = M32(pe, 8) - ACTOR_M32(entity, position_x);
    dx -= 0x6666;
    if (dx < 0)
        dx = 0;
    band = (dx & 0x1C000) >> 14;
    facing = (ACTOR_M32(entity, position_x) >= M32(pe, 8)) ? 1 : 0;

    if (enemy_classify_attack_timing(entity, pe) != 0) {
        if (band >= 3) {
            enemy_reset_idle_or_walk(entity);
            M32(entity, 0xC4) = 8;
            return 0;
        }
        if (ACTOR_M8(entity, facing_right) != facing) {
            actor_set_state_7_entry(entity);
            ACTOR_M16(entity, animation_requested) = M16((void *)(uintptr_t)M32(entity, 0x160), 0);
            return 0;
        } else {
            u8 *p = (u8 *)(uintptr_t)0x060972e0U + ACTOR_M8(entity, slot_id) * 32;
            M16(p, 0) = ACTOR_M16(entity, state2_animation);
            M16(p, 2) = (s16)0x8000;
            actor_init_from_config_raw_entry(entity, p);
            return 1;
        }
    }

    r = game_random_next();
    if ((ACTOR_M32(entity, render_flags) & 0xFA40) != 0 &&
        (M32(pe, 0x84) & 0x400) != 0 &&
        ACTOR_M8(entity, facing_right) != M8(pe, 68) &&
        (ACTOR_M32(entity, interaction_flags) & 0x3FFFF) == 0 &&
        !((r & 7) > M32(entity, 0xE8))) {
        enemy_enter_random_idle18(entity);
        return 0;
    }

    if (ACTOR_M8(entity, facing_right) != facing) {
        void *q = (void *)(uintptr_t)M32(entity, 0x160);
        switch (enemy_classify_linked_effect(entity)) {
        case 0:
            actor_set_state_7_entry(entity);
            ACTOR_M16(entity, animation_requested) = M16(q, 0);
            break;
        case 1:
            actor_set_state_7_entry(entity);
            ACTOR_M16(entity, animation_requested) = M16(q, (game_random_next() & 3) * 2);
            ACTOR_M32(entity, state_flags) |= 0x100000;
            break;
        case 2:
            enemy_enter_dash_recovery_a(entity);
            break;
        case 3:
            enemy_enter_dash_recovery_b(entity);
            break;
        default:
            break;
        }
        return 0;
    } else {
        s32 row = M32((void *)(uintptr_t)sp0, band * 4);
        s32 cfg = M32((void *)(uintptr_t)row, (r & 3) * 4);
        if (cfg > 0) {
            actor_init_from_config_raw_entry(entity, (void *)(uintptr_t)cfg);
            return 1;
        }
        g_06090250[~cfg](entity);
        return 0;
    }
}
