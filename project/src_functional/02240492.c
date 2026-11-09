/* enemy_action_init_phase [0x02240492]: Damage/update, facing resolution, phase=1 then motion setup. */
#include "dh_work.h"
#include "dh.h"
/* 0x02240492 - func_02240492: per-state actor setup (blob sibling of func_022402c0).
 * real fn [0x240492, 0x2404ce) - the entry is 0x240492 (two scratch loads precede
 * the register pushes at 0x240496; census/prologue scan under-counts them).
 *   actor_apply_damage(obj, M16(obj,0x64));
 *   func_02227704(obj);
 *   M32(obj,0x108) = 1;
 *   M32(obj,0x84)  = 0x1000;
 *   *out = 1;
 *   func_0224124e(obj, out, 1);            // shared facing-resolve tail (bra)
 * fidelity: HIGH - exact. r13 holds the constant 1 reused for 0x108, *out and the
 * r6 (3rd) argument to the tail. func_02227704 is called unconditionally here
 * (unlike func_022402c0 which gates it on obj->0xE0 & 0x10000). */
extern void actor_apply_damage(void *obj, s32 amount);          /* 0x0222ab18 */
extern void actor_resolve_facing(void *obj);                       /* 0x00227704 */
extern void actor_select_directional_animation(void *obj, s32 *out, s32 arg);  /* 0x0224124e (tail) */

void enemy_action_init_phase(void *obj, s32 *out)
{
    actor_apply_damage(obj, M16(obj, 0x64));
    actor_resolve_facing(obj);
    ACTOR_M32(obj, action_phase) = 1;
    ACTOR_M32(obj, action_category) = 0x1000;
    *out = 1;
    actor_select_directional_animation(obj, out, 1);
}
