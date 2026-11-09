/* enemy_update_state2 [0x02247106]: Steps the four-phase descriptor-driven state and resumes idle at completion. */
#include "dh.h"
#include "dh_work.h"

/* 0x02247106 - per-object state handler (state at obj->0xD8, values 0..3).
 * Args: arg0 (r10) = context/other object, arg1 (r5->r14) = the object.
 * tbl = *(void **)(obj + 0x170) is a small per-object parameter record read
 * at +4 / +6 / +0x0A / +0x0C.
 *
 * m2c FAILS on this entry ("Cannot find branch target func_022469f2"): that is
 * the tail `bra func_022469f2` to a function located before this one. Decompiled
 * by full manual disassembly + braf_resolve. All computed branches resolved:
 *   bsrf -> func_022489d0, actor_init_from_cfg, actor_script_step, actor_anim_frame_update (x3),
 *           func_02245382, func_02210020, func_022453de ; tail braf/bra ->
 *           actor_anim_frame_update, func_022469f2.
 * m2c/braf_resolve CORRECTION: braf_resolve flagged "247318/247324 bsrf r0"
 * as TABLE dispatches; both are literal-pool longs (0x00030000, 0x0003ffff)
 * loaded with mov.l, NOT branches (pool-phantom trap). The real loop calls are
 * func_02245382 (0xffffe118) and func_02210020 (0xfffc8daa).
 *
 * Callee signatures confirmed from their prologues:
 *   func_02210020() takes no args (overwrites r4/r6 on entry) -> s32.
 *   actor_anim_frame_update(a, obj) reads r4,r5 -> s32 (return used as a predicate here).
 *   func_02245382 reads r4,r5,r6; caller also loads r7=0x10000 and pushes a
 *     stack long 0x4000, so the call is modelled with all 5 ABI slots.
 *   func_022453de(obj) -> object ptr or NULL.
 *
 * Tables (HWRAM): 0x06099590 s32[] "slot active" flag, 0x060995B0 void*[]
 * slot pointers (deref +8 -> inner object); both indexed by obj->0x104 in the
 * pre-loop path and by k=1,2 in the scan loop. These are the same two tables
 * used by func_02243628.
 *
 * Return type: void. Every exit is either a tail call to actor_anim_frame_update(other,obj)
 * / func_022469f2(obj) or a plain rts that sets no return value (store-index left
 * in r0), i.e. a void state-handler that hands off to the next handler.
 * fidelity: HIGH on control flow / memory effects / calls; the void return and
 * the 5-slot func_02245382 prototype are faithful-ABI modelling choices.
 */

void  actor_enter_hit_reaction(void *obj);                                      /* extern */
void  actor_init_from_cfg(void *obj, s32 a);                               /* extern */
void  actor_script_step(void *obj);                                      /* extern */
s32   actor_anim_frame_update(void *other, void *obj);                         /* extern */
s32   actors_test_directional_range(void *obj, void *inner, s32 a, s32 b, s32 c);    /* extern */
s32   game_random_next(void);                                           /* extern */
void *enemy_find_player_in_strike_range(void *obj);                                      /* extern */
void  enemy_reset_idle_or_walk(void *obj);                                      /* extern */

void enemy_update_state2(void *other, void *obj)
{
    void *tbl = MP(obj, 0x170);

    if (ACTOR_M8(obj, hit_pending) != 0) {
        actor_enter_hit_reaction(obj);
        actor_anim_frame_update(other, obj);
        return;
    }

    switch (ACTOR_M32(obj, behavior_phase)) {
    case 0:
        if ((u16)ACTOR_WORK(obj)->animation_events & 2) {
            ACTOR_M32(obj, behavior_phase) = 1;
            actor_init_from_cfg(obj, M32(tbl, 0));
        }
        actor_anim_frame_update(other, obj);
        return;

    case 1: {
        s32   fcmp;
        s32   r4v;
        s32  *tblA = (s32 *)(uintptr_t)DH_ACTOR_SLOT_IDS_ADDR;
        void **tblB = (void **)(uintptr_t)DH_ACTOR_SLOT_TASKS_ADDR;
        void *p;
        s32   idx;
        s32   k;

        actor_script_step(obj);
        fcmp = actor_anim_frame_update(other, obj);

        p = (u8 *)MP(obj, 0x118) + (u32)(u16)M16(obj, 0x11C) * 4;
        if ((M16(p, 2) & 0x4000) == 0) {
            return;
        }

        idx = ACTOR_M32(obj, queued_attack_slot);
        if (tblA[idx] == 0) {
            r4v = 0;
        } else {
            void *inner = MP(tblB[idx], 8);
            s32   val   = ACTOR_WORK(inner)->position_x;
            s32   cmp   = ACTOR_WORK(obj)->position_x;
            if (ACTOR_WORK(obj)->facing_right == 0) {
                r4v = (val >= cmp) ? 0 : 1;
            } else {
                r4v = (val > cmp) ? 1 : 0;
            }
        }

        if (!(fcmp == 0 && r4v == 0)) {
            ACTOR_M32(obj, behavior_phase) = 3;
            ACTOR_WORK(obj)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
            ACTOR_WORK(obj)->animation_requested = M16(tbl, 4);
            return;
        }

        for (k = 1; k <= 2; k++) {
            void *inner;
            s32   fl;
            s32   t;

            if (tblA[k] == 0) {
                continue;
            }
            inner = MP(tblB[k], 8);
            if (actors_test_directional_range(obj, inner, 0x00030000, 0x00010000, 0x4000) == 0) {
                continue;
            }
            t = game_random_next() & 1;
            if (!(ACTOR_M32(obj, interaction_flags) & 0x0003FFFF) &&
                t != 0 &&
                (u16)M16(tbl, 0x0A) != 0xFFFF) {
                fl = ACTOR_WORK(inner)->action_category;
                if ((fl & 0x040D) == 0) {
                    continue;
                }
                if ((fl & 0x237FBB00) != 0) {
                    continue;
                }
                if (ACTOR_M8(inner, action_pending) != 0) {
                    continue;
                }
                /* found */
                ACTOR_WORK(obj)->animation_control   = DH_ANIMATION_CONTROL_RESTART;
                ACTOR_WORK(obj)->animation_requested  = M16(tbl, 0x0A);
                ACTOR_WORK(obj)->action_category  = 0x100;
                ACTOR_WORK(obj)->state_flags |= 0x00040000;
                ACTOR_M32(obj, action_phase) = 1;
                ACTOR_M32(obj, behavior_phase)  = 2;
                return;
            }
            /* L2472ce */
            ACTOR_WORK(obj)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
            ACTOR_WORK(obj)->animation_requested = M16(tbl, 6 + (t << 1));
            ACTOR_M32(obj, behavior_phase) = 3;
            ACTOR_WORK(obj)->action_category = 0x100;
            return;
        }
        return;
    }

    case 2: {
        void *obj2 = enemy_find_player_in_strike_range(obj);
        if (obj2 != 0) {
            ACTOR_M32(obj, behavior_state)  = DH_ENEMY_BEHAVIOR_GRAPPLE;
            ACTOR_M32(obj, behavior_phase)  = 0;
            ACTOR_WORK(obj)->action_category  = 0x00020000;
            ACTOR_WORK(obj)->state_flags &= 0x0300;
            ACTOR_WORK(obj)->state_flags |= 0x2000;
            ACTOR_M32(obj, action_phase) = 2;
            ACTOR_M8(obj, paired_slot)   = ACTOR_WORK(obj2)->slot_id;
            ACTOR_WORK(obj)->animation_control   = DH_ANIMATION_CONTROL_RESTART;
            ACTOR_WORK(obj)->animation_requested  = M16(tbl, 0x0C);
            ACTOR_M8(obj2, hit_pending)  = ACTOR_WORK(obj)->slot_id;
            ACTOR_WORK(obj2)->action_category = 0x00040000;
            ACTOR_WORK(obj2)->action_state = 44;
            actor_anim_frame_update(other, obj);
            return;
        }
        if ((u16)ACTOR_WORK(obj)->animation_events & 2) {
            actor_anim_frame_update(other, obj);
            return;
        }
        enemy_reset_idle_or_walk(obj);
        actor_anim_frame_update(other, obj);
        return;
    }

    case 3:
        actor_anim_frame_update(other, obj);
        if ((u16)ACTOR_WORK(obj)->animation_events & 2) {
            enemy_reset_idle_or_walk(obj);
        }
        return;

    default:
        return;
    }
}
