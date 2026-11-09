/* enemy_action_walk_or_face_alternate [0x02240efe]: Separate walk gate/facing variant in action handler table. */
#include "dh_work.h"
#include "dh.h"
/* 0x02240efe - func_02240efe: per-state "enter statewalk else face" handler
 * (blob sibling of func_022402c0). real fn [0x240efe, 0x240fc8) - two exits
 *   (bra-tail to func_0224124e, and a plain rts).
 *   actor_apply_damage(obj, M16(obj,0x64));
 *   if (M32(obj,0xE0) & 0x10000) func_02227704(obj);
 *   if (func_02241556(obj, 3) == 0) {              // can enter statewalk
 *       M32(obj,0x10C)=1; M32(obj,0x108)=1; M32(obj,0x84)=0x1000;
 *       M32(obj,0xE0)|=0x40000; *out=1; func_0224124e(obj,out,0); return;
 *   }
 *   f = (u16)MU16(obj,0x62) >> 15;                 // facing bit
 *   if (M8(obj,0x44) != 0) f = ~f;
 *   M16(obj,0x5C) = g_0608d890[f & 1];
 *   M32(obj,0x84)=0x800; M32(obj,0x108)=0; *out=0;
 * fidelity: HIGH - exact ROM store order in the face path: 0x5C word (0x240fb0),
 * then 0x84=0x800 (0x240fb6, r0=92+40), then 0x108=0 (0x240fba), then *out
 * (0x240fbc). 0x62 zero-extended then >>15 (shlr8+shlr2x3+shlr); facing index
 * toggled by (M8(0x44)!=0) via one's-complement. */
extern void actor_apply_damage(void *obj, s32 amount);          /* 0x0222ab18 */
extern void actor_resolve_facing(void *obj);                       /* 0x00227704 */
extern s32  actor_debounce_frame_event(void *obj, s32 arg);            /* 0x02241556 */
extern void actor_select_directional_animation(void *obj, s32 *out, s32 arg);  /* 0x0224124e (tail) */
extern s16  g_0608d890[];                                 /* s16 facing table */

void enemy_action_walk_or_face_alternate(void *obj, s32 *out)
{
    u32 f;
    actor_apply_damage(obj, M16(obj, 0x64));
    if (ACTOR_M32(obj, state_flags) & 0x10000)
        actor_resolve_facing(obj);
    if (actor_debounce_frame_event(obj, 3) == 0) {
        ACTOR_M32(obj, event_count) = 1;
        ACTOR_M32(obj, action_phase) = 1;
        ACTOR_M32(obj, action_category) = 0x1000;
        ACTOR_M32(obj, state_flags) |= 0x40000;
        *out = 1;
        actor_select_directional_animation(obj, out, 0);
        return;
    }
    f = (u32)ACTOR_MU16(obj, motion_heading) >> 15;
    if (ACTOR_M8(obj, facing_right) != 0)
        f = ~f;
    ACTOR_M16(obj, animation_requested) = g_0608d890[f & 1];
    ACTOR_M32(obj, action_category) = 0x800;
    ACTOR_M32(obj, action_phase) = 0;
    *out = 0;
}
