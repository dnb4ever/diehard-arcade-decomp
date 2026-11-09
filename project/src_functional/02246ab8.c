#include "dh_work.h"
/* enemy_update_interrupts [0x02246ab8]: Resolves hit/counter interrupts and dispatches the active enemy script before animation. */
#include "dh.h"

/* 0x02246ab8 - per-object counter/hit-interrupt FSM (func_02246ab8).
 *
 * Args: r4 = ctx (saved in r13, only forwarded to the shared tail), r5 = obj
 * (r14, the object operated on). Cached once at entry: flags = M32(obj,0xE0)
 * (r11). Every path except switch-case 3 converges on the shared tail
 *     actor_anim_frame_update(ctx, obj)
 * and returns. (Case 3 tail-calls func_022470a2(obj) and bypasses the tail.)
 *
 * SCOPE / CENSUS CORRECTION: fn.py reports this region as "785 insn" but that
 * extent covers a whole blob of sibling functions. The real func_02246ab8 is
 * only [0x246ab8, 0x246cac) (~250 insn) and is fully resolvable. The blob also
 * holds: func_02246cac [0x246cac,0x246d0c) (a small state-init helper called 3x
 * here via bsr), and sibling state handlers func_02246d0c / func_02246dbc /
 * func_02246f32, plus leaf helpers func_02247036 / func_02247064 / func_022470a2 - all
 * declared extern below.
 *
 * TAIL-TARGET CORRECTION: notes/symbols_giants2.txt called the r12 tail
 * "0x00225c2c". That is a miscalculation. r12 = mova(0x246ad0) + 0xFFFE715C =
 * 0x0022DC2C. Confirmed independently by sibling func_02246d0c whose own tail
 * (0x246d1c + 0xFFFE6F10) resolves to the SAME 0x0022DC2C. The universal tail
 * is actor_anim_frame_update, not 0x00225c2c.
 *
 * SWITCH-VAR CORRECTION: m2c renders the dispatch as "switch(0xC8)" (lost the
 * register). The real selector is the return value of actor_state_dispatch(obj), not
 * obj->0xC8. (obj->0xC8 is separately decremented earlier.)
 *
 * m2c COMPUTED-CALL FIXES (m2c emitted M2C_ERROR at every bsrf and substituted
 * byte-offset literals for the lost return values):
 *   - "if (0x4C != 0)"      -> if (actor_flag_rng_1in4(obj) != 0)
 *   - "(u16)temp_r0 == 0"   -> func_02245bcc(obj) == 0
 *   - "(0xC8 != 0)"         -> func_02247d6e(obj) != 0
 *   - "(0xC8 & 7) + 8"      -> (func_02210020() & 7) + 8   [func_02210020 no args]
 *   - "if (0x128 == 0)"     -> func_0224492c(...) == 0
 *   - m2c mis-attached the args (elem->8, 0x18000, 0x20000) to .L246cac; they
 *     belong to func_0224492c. The real func_02246cac call takes only obj.
 * No tst #imm8 >= 0x80 occurs here (the only tst #imm is tst #2), so no
 * zero/sign-extend mask trap applies.
 *
 * Globals: 0x06097418 abort/interrupt flag; 0x06090230 fn-ptr table (indexed by
 * obj->0x100); 0x06099584 base -> +0x0C(0x06099590) active-flag array, +0x2C
 * (0x060995B0) source-object array (both indexed by obj->0x128).
 * obj fields: +0x4C s8 hit-interrupt; +0x54 u16 flagword (bit1); +0x5C s16;
 * +0x14A s16; +0xC4 s32 counter; +0xC8 s32 counter/state; +0xE0 s32 flags
 * (bits 0x8/0xC/0x20/0x50/0x100); +0x100 s32 table index; +0x128 s32 slot idx.
 */

void  actor_enter_hit_reaction(void *obj);                          /* extern */
s32   actor_flag_rng_1in4(void *obj);                          /* extern */
void  enemy_enter_clash(void *obj);                          /* extern */
void  enemy_enter_stage_pose18(void *obj);                          /* extern */
s32   enemy_test_reference_facing(void *obj);                          /* extern */
void  actor_set_state_7(void *obj);                          /* extern */
void  enemy_choose_random_direction(void *obj);                          /* extern */
s32   enemy_enter_guard(void *obj);                          /* extern */
s32   game_random_next(void);                               /* extern (no args) */
s32   actor_state_dispatch(void *obj);                          /* extern (switch sel) */
s32   actor_test_target_rectangle(void *obj, void *target, s32 a, s32 b); /* extern */
void  enemy_begin_approach(void *obj);                          /* extern (local init) */
void  enemy_enter_descriptor_state18(void *obj);                          /* extern (case-3 tail) */
void  actor_anim_frame_update(void *ctx, void *obj);               /* extern (shared tail) */

void enemy_update_interrupts(void *ctx, void *obj)
{
    s32 flags = ACTOR_M32(obj, state_flags);         /* r11: cached object flags */
    s32 old;

    if (ACTOR_M8(obj, hit_pending) != 0) {           /* hit-interrupt pending */
        actor_enter_hit_reaction(obj);
        goto done;
    }
    if (actor_flag_rng_1in4(obj) != 0) {
        enemy_enter_clash(obj);
        goto done;
    }
    if (M32((void *)(uintptr_t)0x06097418, 0) != 0) {
        enemy_enter_stage_pose18(obj);
        goto done;
    }
    if (ACTOR_MU16(obj, animation_events) & 2) {
        if (enemy_test_reference_facing(obj) == 0) {
            actor_set_state_7(obj);
            ACTOR_M16(obj, animation_requested) = ACTOR_M16(obj, approach_animation);
            goto done;
        }
        enemy_choose_random_direction(obj);               /* result discarded; falls through */
    }

    old = M32(obj, 0xC8);
    M32(obj, 0xC8) = old - 1;
    if (old <= 0 && (flags & 8) == 0) {
        if ((flags & 0x100) && enemy_enter_guard(obj) != 0)
            goto done;
        if (flags & 0x20)
            M32(obj, 0xC8) = (game_random_next() & 7) + 8;
        else
            enemy_begin_approach(obj);
        goto done;
    }

    if (flags & 0xC) {
        old = M32(obj, 0xC4);
        M32(obj, 0xC4) = old - 1;
        if (old == 0)
            ACTOR_M32(obj, state_flags) &= ~4;
        goto done;
    }

    if ((flags & 0x100) && enemy_enter_guard(obj) != 0)
        goto done;

    if (flags & 0x50) {
        enemy_begin_approach(obj);
        goto done;
    }

    switch (actor_state_dispatch(obj)) {
    case 1:
        (*(void (**)(void *))(uintptr_t)(0x06090230 + ACTOR_M32(obj, queued_attack_kind) * 4))(obj);
        goto done;
    case 3:
        enemy_enter_descriptor_state18(obj);               /* tail-call, bypasses actor_anim_frame_update */
        return;
    case 0:
    case 2:
    default:
        if (flags & 0x20) {
            s32 idx = ACTOR_M32(obj, player_target_slot);
            if (*(void **)(uintptr_t)(DH_ACTOR_SLOT_IDS_ADDR + idx * 4) != 0) {
                void *elem = *(void **)(uintptr_t)(DH_ACTOR_SLOT_TASKS_ADDR + idx * 4);
                if (actor_test_target_rectangle(obj, MP(elem, 8), 0x18000, 0x20000) == 0) {
                    ACTOR_M32(obj, state_flags) &= ~0x20;
                    enemy_begin_approach(obj);
                }
            }
        }
        goto done;
    }

done:
    actor_anim_frame_update(ctx, obj);
}
