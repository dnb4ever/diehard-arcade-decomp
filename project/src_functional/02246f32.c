#include "dh_work.h"
/* enemy_choose_simple_response [0x02246f32]: Chooses guard or descriptor response under the reduced combat flag gates. */
#include "dh.h"
/* 0x02246f32 - per-object state handler (func_02246f32), a slimmer sibling of
 * func_02246dbc (no 0x8C/0x40 pre-gate, no 0x20/0x128 block). obj = r4 (r14).
 * Real function is [0x246f32, 0x247036). d / flags cached as in func_02246dbc.
 *
 *   if (flags & 0x100) { if (func_02247d6e(obj) == 0) func_022469f2(obj); return; }
 *   switch (actor_state_dispatch(obj)) {
 *     case 1: (*(void(**)(void*))(0x06090230 + obj->0x100*4))(obj); return;
 *     case 3: func_022470a2(obj); return;                // tail
 *     case 2: func_02247064(obj); break;                 // -> continuation
 *     default: break;                                  // case 0 -> continuation
 *   }
 *   if (!(obj->0x54 & 2)) {
 *       if (d == 0) return;
 *       if (obj->0x40 != 0) return;
 *       if ((u16)*(s16*)(d + obj->0x100*2) != (u16)obj->0x58) return;
 *   }
 *   switch (func_02243fe0(obj)) {
 *     case 1: actor_set_state_7(obj); obj->0x5C = obj->0x14A; break;
 *     case 2: func_02249fb0(obj); return;                // tail
 *     default: break;
 *   }
 *
 * fidelity: HIGH - targets resolved; tst #2 only (no mask trap); u16 compares
 * use extu.w. */
s32  enemy_enter_guard(void *obj);   /* extern */
void enemy_reset_idle_or_walk(void *obj);   /* extern (tail) */
s32  actor_state_dispatch(void *obj);   /* extern (switch sel) */
void enemy_enter_descriptor_state18(void *obj);   /* extern (case-3 tail) */
void enemy_enter_idle_state18(void *obj);   /* extern */
s32  enemy_sweep_toward_target(void *obj);   /* extern (switch sel) */
void actor_set_state_7(void *obj);   /* extern */
void enemy_enter_state19(void *obj);   /* extern (tail) */

void enemy_choose_simple_response(void *obj)
{
    void *d = MP(ACTOR_MP(obj, action_config_tables), 4);
    s32 flags = ACTOR_M32(obj, state_flags);

    if (flags & 0x100) {
        if (enemy_enter_guard(obj) == 0)
            enemy_reset_idle_or_walk(obj);
        return;
    }

    switch (actor_state_dispatch(obj)) {
    case 1:
        (*(void (**)(void *))(uintptr_t)(0x06090230 + ACTOR_M32(obj, queued_attack_kind) * 4))(obj);
        return;
    case 3:
        enemy_enter_descriptor_state18(obj);
        return;
    case 2:
        enemy_enter_idle_state18(obj);
        break;
    default:
        break;
    }

    if (!(ACTOR_MU16(obj, animation_events) & 2)) {
        if (d == 0)
            return;
        if (ACTOR_M8(obj, interpolation_ticks) != 0)
            return;
        if ((u16)M16(d, ACTOR_M32(obj, queued_attack_kind) * 2) != (u16)ACTOR_M16(obj, animation_frame))
            return;
    }

    switch (enemy_sweep_toward_target(obj)) {
    case 1:
        actor_set_state_7(obj);
        ACTOR_M16(obj, animation_requested) = ACTOR_M16(obj, approach_animation);
        break;
    case 2:
        enemy_enter_state19(obj);
        return;
    default:
        break;
    }
}
