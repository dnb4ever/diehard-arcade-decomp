#include "dh_work.h"
/* enemy_choose_full_response [0x02246dbc]: Chooses guard, approach or descriptor response under the full combat flag gates. */
#include "dh.h"
/* 0x02246dbc - per-object state handler / FSM (func_02246dbc), a fuller sibling
 * of func_02246ab8's tail dispatch. obj = r4 (r14).
 *
 * SCOPE: fn.py over-swallows this into func_02246f32; the real function is
 * [0x246dbc, 0x246f32). braf_resolve reports a computed branch at 0x246e68
 * ("via mov.b @(r0,r8)") - that is a pool/phantom misread (no such bsrf exists
 * in the instruction stream) and is ignored.
 *
 * d = *(void**)(*(void**)(obj+0x158) + 4)  is cached once (r13, callee-saved);
 * flags = obj->0xE0 is cached once (r4).
 *
 *   if (flags & 0x100) { if (func_02247d6e(obj) == 0) func_022469f2(obj); return; }
 *   if ((obj->0x8C & 0x3FFFF) == 0 && (flags & 0x40)) {
 *       s32 r = func_02245d1c(obj);
 *       if (r != 0) { func_02249836(obj, r); return; }   // braf tail, 2 args
 *   }
 *   switch (actor_state_dispatch(obj)) {
 *     case 1: (*(void(**)(void*))(0x06090230 + obj->0x100*4))(obj); return;
 *     case 3: func_022470a2(obj); return;                // tail
 *     case 2: func_02247064(obj); break;                 // -> continuation
 *     default: break;                                  // case 0 -> continuation
 *   }
 *   // continuation:
 *   if (!(obj->0x54 & 2)) {
 *       if (d == 0) return;
 *       if (obj->0x40 != 0) return;
 *       if ((u16)*(s16*)(d + obj->0x100*2) != (u16)obj->0x58) return;
 *   }
 *   if (obj->0xE0 & 0x20) {
 *       s32 idx = obj->0x128;
 *       if (*(void**)(0x06099590 + idx*4) == 0) { func_022469f2(obj); return; }
 *       void *src = *(void**)(0x060995B0 + idx*4);
 *       if (func_0224492c(obj, MP(src,8), 0x18000, 0x20000) != 0) {
 *           func_022469f2(obj); return;
 *       }
 *   }
 *   switch (func_02243fe0(obj)) {
 *     case 1: actor_set_state_7(obj); obj->0x5C = obj->0x14A; break;
 *     case 2: func_02249fb0(obj); return;                // tail
 *     default: break;
 *   }
 *
 * fidelity: HIGH - all bsrf/braf targets resolved; tst #imm are #2 and #32
 * (< 0x80, no zero/sign-extend mask trap); u16 field compares use extu.w. */
s32  enemy_enter_guard(void *obj);                              /* extern */
void enemy_reset_idle_or_walk(void *obj);                              /* extern (tail) */
s32  enemy_find_latched_effect_target(void *obj);                              /* extern */
void enemy_begin_combo(void *obj, s32 arg);                     /* extern (tail) */
s32  actor_state_dispatch(void *obj);                              /* extern (switch sel) */
void enemy_enter_descriptor_state18(void *obj);                              /* extern (case-3 tail) */
void enemy_enter_idle_state18(void *obj);                              /* extern */
s32  actor_test_target_rectangle(void *obj, void *target, s32 a, s32 b);  /* extern */
s32  enemy_sweep_toward_target(void *obj);                              /* extern (switch sel) */
void actor_set_state_7(void *obj);                              /* extern */
void enemy_enter_state19(void *obj);                              /* extern (tail) */

void enemy_choose_full_response(void *obj)
{
    void *d = MP(ACTOR_MP(obj, action_config_tables), 4);
    s32 flags = ACTOR_M32(obj, state_flags);

    if (flags & 0x100) {
        if (enemy_enter_guard(obj) == 0)
            enemy_reset_idle_or_walk(obj);
        return;
    }

    if ((ACTOR_M32(obj, interaction_flags) & 0x3FFFF) == 0 && (flags & 0x40)) {
        s32 r = enemy_find_latched_effect_target(obj);
        if (r != 0) {
            enemy_begin_combo(obj, r);
            return;
        }
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

    if (ACTOR_M32(obj, state_flags) & 0x20) {
        s32 idx = ACTOR_M32(obj, player_target_slot);
        if (*(void **)(uintptr_t)(DH_ACTOR_SLOT_IDS_ADDR + idx * 4) == 0) {
            enemy_reset_idle_or_walk(obj);
            return;
        }
        {
            void *src = *(void **)(uintptr_t)(DH_ACTOR_SLOT_TASKS_ADDR + idx * 4);
            if (actor_test_target_rectangle(obj, MP(src, 8), 0x18000, 0x20000) != 0) {
                enemy_reset_idle_or_walk(obj);
                return;
            }
        }
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
