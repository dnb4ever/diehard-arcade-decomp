/* actor_draw_by_model_mode [0x022462bc]: Dispatches the articulated draw path according to actor flag bit 0x400. */
#include "dh.h"
/* 0x022462bc - func_022462bc(actor): 2-way draw dispatch thunk on flag bit
 * 0x400 of actor->0x38.
 *   tst 0x400, actor->0x38 sets T = (AND == 0);
 *   bf 0x2462ca is taken when T == 0, i.e. when the flag bit IS SET:
 *     bit SET   -> braf -> 0x0222d450  func_0222d450(actor)
 *     bit CLEAR -> fallthrough braf -> 0x0222ca98 draw_actor_hier_shadowed(actor)
 * Both targets are reached via braf with a nop delay slot: every register
 * (r4 = actor and the rest) passes through unchanged, so this is a pure tail
 * call forwarding the identical ABI. The thunk has no rts of its own, so the
 * callee's residual r0 is what the caller observes; the s32 return forwards it.
 * fidelity: HIGH - braf targets resolved as PC+4+sext(disp):
 *   0x2462c8 + sext(0xfffe67ce) -> 0x0222ca98 (draw_actor_hier_shadowed)
 *   0x2462d0 + sext(0xfffe7180) -> 0x0222d450 */
extern s32 draw_actor_hier_shadowed(void *actor); /* 0x0222ca98 */
extern s32 actor_draw_skeletal_mesh(void *actor);            /* 0x0222d450 */

s32 actor_draw_by_model_mode(void *actor)
{
    if (M32(actor, 0x38) & 0x400)
        return actor_draw_skeletal_mesh(actor);
    else
        return draw_actor_hier_shadowed(actor);
}
