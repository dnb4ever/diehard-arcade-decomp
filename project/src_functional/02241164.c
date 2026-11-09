#include "dh_work.h"
/* actor_select_walk_or_random_pose [0x02241164]: Debounces the reaction path into a directional walk or chooses a random facing pose. */
#include "dh.h"
/* 0x02241164 - func_02241164: per-state "enter statewalk else random-face"
 * handler (blob sibling of func_022402c0). real fn [0x241164, 0x24124e) -
 * prologue pushes r8,r11,r12,r13,r14; two exits (bra-tail to func_0224124e, and
 * a plain rts at 0x24124a). Shared tail-call target of several siblings.
 *   actor_apply_damage(obj, M16(obj,0x64));
 *   if (M32(obj,0xE0) & 0x10000) func_02227704(obj);
 *   if (func_02241556(obj, 6) == 0 && M16(obj,0x56) > 0) {
 *       M32(obj,0x10C)=1; M32(obj,0x108)=1; M32(obj,0x84)=0x1000;
 *       M32(obj,0xE0)|=0x40000; *out=1; func_0224124e(obj,out,0); return;
 *   }
 *   M32(obj,0x108) = 0;
 *   f = (u16)MU16(obj,0x62) >> 14;
 *   if (M8(obj,0x44) != 0) f = ~f;
 *   M16(obj,0x5C) = g_06086720[(f & 2) + (func_02210020() & 1)];
 *   M32(obj,0x84) = 0x800; *out = 0;
 * fidelity: HIGH - matches m2c exactly. 0x56 signed (cmp/pl); 0x62 zero-extended
 * then >>14; facing index = high facing bit (0/2) plus a random 0/1 bit from the
 * no-arg rng. */
extern void actor_apply_damage(void *obj, s32 amount);          /* 0x0222ab18 */
extern void actor_resolve_facing(void *obj);                       /* 0x00227704 */
extern s32  actor_debounce_frame_event(void *obj, s32 arg);            /* 0x02241556 */
extern s32  game_random_next(void);                          /* 0x02210020 rng, no args */
extern void actor_select_directional_animation(void *obj, s32 *out, s32 arg);  /* 0x0224124e (tail) */
extern s16  g_06086720[];                                 /* s16 facing table */

void actor_select_walk_or_random_pose(void *obj, s32 *out)
{
    u32 f;
    actor_apply_damage(obj, M16(obj, 0x64));
    if (ACTOR_M32(obj, state_flags) & 0x10000)
        actor_resolve_facing(obj);
    if (actor_debounce_frame_event(obj, 6) == 0 && ACTOR_M16(obj, health) > 0) {
        ACTOR_M32(obj, event_count) = 1;
        ACTOR_M32(obj, action_phase) = 1;
        ACTOR_M32(obj, action_category) = 0x1000;
        ACTOR_M32(obj, state_flags) |= 0x40000;
        *out = 1;
        actor_select_directional_animation(obj, out, 0);
        return;
    }
    ACTOR_M32(obj, action_phase) = 0;
    f = (u32)ACTOR_MU16(obj, motion_heading) >> 14;
    if (ACTOR_M8(obj, facing_right) != 0)
        f = ~f;
    ACTOR_M16(obj, animation_requested) = g_06086720[(f & 2) + (game_random_next() & 1)];
    ACTOR_M32(obj, action_category) = 0x800;
    *out = 0;
}
