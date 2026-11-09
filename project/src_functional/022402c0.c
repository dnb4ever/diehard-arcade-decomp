#include "dh_work.h"
/* actor_check_expiry_or_lock [0x022402c0]: Updates damage and target depth, then expires or locks the actor and selects directional animation. */
#include "dh.h"

/* 0x022402c0 - per-object "expire / lock-on" check (func_022402c0).
 *
 * Args: r4 = obj (r14), r5 = out (r12, an s32* result slot written on every
 * path). Runs the per-frame damage/update helpers, then decides whether the
 * object expires (sets the 0x40000 flag, *out = 1) or enters a seek/lock-on
 * state, and finally tail-calls func_0224124e(obj, out, 0) - except the lock-on
 * branch, which returns directly.
 *
 * SCOPE / CENSUS CORRECTION: fn.py reports "2379 insn / extent 0x1296" for this
 * address, and notes/functional_giants2.txt deferred it as a 2379-insn monster
 * with 58 computed branches. That extent is a CENSUS BLOB of ~20 sibling
 * functions [0x2402c0, 0x241556) that share a tail pool of helpers at 0x241556+
 * (func_02241556 / actor_motion_init / func_0224158c / func_022416c4 / func_0224174e /
 * func_022417a4 / func_022417ea / func_02241324 / func_022413e6 / func_022414ee / ...).
 * The REAL func_022402c0 is only [0x2402c0, 0x24043c) (~90 insn) and resolves
 * fully. m2c mis-structures this: it stops ~60 lines in and then splices code
 * from a later sibling (its func_022416c4 call + the giant obj->0x5C expression
 * belong to a different function, not to func_022402c0).
 *
 * The "58 computed branches / actor_apply_damage x30 / func_02227704 x15" counts in the
 * note are blob-wide; func_022402c0 itself has 5 computed calls, all resolved:
 *   0x2402d2 bsrf -> actor_apply_damage (obj, obj->0x64)       [per-frame update]
 *   0x2402e2 bsrf -> func_02227704 (obj)                  [gated on E0&0x10000]
 *   0x2402e8 bsrf -> func_0224174e (obj)
 *   0x240316 bsrf -> func_02241556 (obj, 6) -> s32
 *   0x2403b8 bsrf -> sound_queue_push (0xA0....FF command)   [PIC effect trigger]
 * Tail: 0x24042c bra -> func_0224124e(obj, out, 0).
 *
 * m2c COMPUTED-CALL FIX: m2c rendered "if (0x56 != 0)" for the func_02241556
 * result test (it lost the register). No tst #imm8 occurs (all tst are register
 * form), so no zero/sign-extend mask trap applies here.
 *
 * obj fields: +0x08 s32 pos; +0x38 s32 flags(0x7800); +0x44 s8 facing; +0x4A s8
 * slot index; +0x56 s16; +0x5C s16; +0x64 s16 hp/timer; +0x84 s32; +0xBA s16;
 * +0xE0 s32 flags; +0x108 s32; +0x10C s32; +0x146 s16 max; +0x180 ptr.
 * Globals: 0x060995B0 source-object array; 0x06098564 mode; 0x060E0DDC effect
 * enable; 0x0608DA40 s16 effect param.
 */

void actor_apply_damage(void *obj, s32 arg);                  /* extern */
void actor_resolve_facing(void *obj);                           /* extern */
void actor_follow_target_depth(void *obj);                           /* extern */
s32  actor_debounce_frame_event(void *obj, s32 arg);                  /* extern */
void sound_queue_push(s32 code);                            /* extern (PIC) */
void actor_select_directional_animation(void *obj, s32 *out, s32 arg);        /* extern (tail) */

void actor_check_expiry_or_lock(void *obj, s32 *out)
{
    s16 hp;

    actor_apply_damage(obj, M16(obj, 0x64));
    if (ACTOR_M32(obj, state_flags) & 0x10000)
        actor_resolve_facing(obj);
    actor_follow_target_depth(obj);

    hp = M16(obj, 0x64);
    if (hp >= M16(obj, 0x146))
        goto expire;
    if (ACTOR_M16(obj, health) <= 0)
        goto expire;
    if (hp == 0)
        goto expire;

    if (actor_debounce_frame_event(obj, 6) != 0) {
        ACTOR_M32(obj, action_category)  = 0x800;
        ACTOR_M32(obj, action_phase) = 0;
        *out = 0;
        goto tail;
    }

    ACTOR_M32(obj, event_count) = 1;
    {
        void *src    = *(void **)(uintptr_t)(DH_ACTOR_SLOT_TASKS_ADDR + ACTOR_M8(obj, hit_source_slot) * 4);
        s32   toward = ((M32(MP(src, 8), 8) - ACTOR_M32(obj, position_x)) < 0) ? 1 : 0;

        if ((ACTOR_M32(obj, render_flags) & 0x7800) && ACTOR_M8(obj, facing_right) != toward &&
            M32((void *)(uintptr_t)DH_READY_PLAYER_MASK_ADDR, 0) == 3) {
            /* seek / lock-on */
            ACTOR_M16(obj, animation_requested)   = M16(ACTOR_MP(obj, reaction_descriptor), 0xA);
            ACTOR_M32(obj, action_category)   = 0x800;
            ACTOR_M32(obj, state_flags)  |= 0x02000000;
            ACTOR_M32(obj, action_phase)  = 6;
            ACTOR_M16(obj, type_flags)  |= 0x100;
            if (M32((void *)(uintptr_t)DH_SOUND_EFFECTS_ENABLED_ADDR, 0) != 0)
                sound_queue_push(((s32)M16((void *)(uintptr_t)0x0608DA40, 0) << 8) | DH_SOUND_CUE_BASE);
            *out = 0;
            return;
        }
    }
    /* else fall through to expire */

expire:
    ACTOR_M32(obj, state_flags)  |= 0x40000;
    ACTOR_M32(obj, action_phase)  = 1;
    ACTOR_M32(obj, action_category)   = 0x1000;
    *out = 1;
tail:
    actor_select_directional_animation(obj, out, 0);
}
