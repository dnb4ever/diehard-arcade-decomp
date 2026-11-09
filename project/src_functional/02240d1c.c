/* enemy_action_select_hit_kind [0x02240d1c]: Hit-kind byte+4a bounds select action continuation or shared expire path. */
#include "dh_work.h"
#include "dh.h"
/* 0x02240d1c - func_02240d1c: large per-state handler (blob sibling of
 * func_022402c0). real fn [0x240d1c, 0x240e0e) - prologue pushes r11..r14;
 * internal literal pool at 0x240e1e..; two returns (func_0224124e tail and the
 * shared rts at 0x240e02/0x240e0a); rejects tail-call func_022402c0.
 *   kind = M8(obj,0x4A);
 *   if (kind < 1 || kind > 2) { func_022402c0(obj,out); return; }
 *   if (func_0224158c(obj,3) == 0 || M16(obj,0x56) <= 0) {
 *       actor_apply_damage(obj, M16(obj,0x64));
 *       M32(obj,0xE0) |= 0x40000; M32(obj,0x108)=1; M32(obj,0x84)=0x1000;
 *       *out = 1; func_0224124e(obj,out,0); return;
 *   }
 *   if (g_0608d824[(u8)M8(obj,0x49)] == 2 && !(M32(obj,0x38) & 0x520)) {
 *       actor_apply_damage(obj, M16(obj,0x64));
 *       if (g_060e0ddc != 0) sound_queue_push(0xA01224FF);
 *       f = (u16)MU16(obj,0x62) >> 15; if (M8(obj,0x44) != 0) f = ~f;
 *       M16(obj,0x5C) = g_0608d888[f & 1];
 *       M32(obj,0x84)=0x800; M32(obj,0x108)=0;
 *       M32(obj,0x114) = g_060987bc + 0x3C;
 *       *out = 0; return;
 *   }
 *   func_022402c0(obj, out);
 * fidelity: HIGH - matches m2c exactly. 0x4A/0x56 signed; 0x49 unsigned table
 * index; 0x38 & 0x520 reg-form tst; 0x62 zero-extended then >>15. */
extern void actor_check_expiry_or_lock(void *obj, s32 *out);            /* 0x022402c0 (tail) */
extern s32  actor_debounce_long_event(void *obj, s32 limit);          /* 0x0224158c */
extern void actor_apply_damage(void *obj, s32 amount);          /* 0x0222ab18 */
extern void sound_queue_push(s32 cmd);                       /* 0x02215f6c sound_queue_push */
extern void actor_select_directional_animation(void *obj, s32 *out, s32 arg);  /* 0x0224124e (tail) */
extern s32  g_0608d824[];                                 /* s32 table, idx (u8)obj->0x49 */
extern s16  g_0608d888[];                                 /* s16 facing table */
extern s32  g_sound_effects_enabled;                                   /* s32 gate flag */
extern s32  g_frame_tick_legacy;                                   /* s32 (counter/base) */

void enemy_action_select_hit_kind(void *obj, s32 *out)
{
    s32 kind = ACTOR_M8(obj, hit_source_slot);

    if (kind < 1 || kind > 2) {
        actor_check_expiry_or_lock(obj, out);
        return;
    }
    if (actor_debounce_long_event(obj, 3) == 0 || ACTOR_M16(obj, health) <= 0) {
        actor_apply_damage(obj, M16(obj, 0x64));
        ACTOR_M32(obj, state_flags) |= 0x40000;
        ACTOR_M32(obj, action_phase) = 1;
        ACTOR_M32(obj, action_category) = 0x1000;
        *out = 1;
        actor_select_directional_animation(obj, out, 0);
        return;
    }
    if (g_0608d824[MU8(obj, 0x49)] == 2 && !(ACTOR_M32(obj, render_flags) & 0x520)) {
        u32 f;
        actor_apply_damage(obj, M16(obj, 0x64));
        if (g_sound_effects_enabled != 0)
            sound_queue_push(0xA01224FF);
        f = (u32)ACTOR_MU16(obj, motion_heading) >> 15;
        if (ACTOR_M8(obj, facing_right) != 0)
            f = ~f;
        ACTOR_M16(obj, animation_requested) = g_0608d888[f & 1];
        ACTOR_M32(obj, action_category) = 0x800;
        ACTOR_M32(obj, action_phase) = 0;
        ACTOR_M32(obj, action_start_tick) = g_frame_tick_legacy + 0x3C;
        *out = 0;
        return;
    }
    actor_check_expiry_or_lock(obj, out);
}
