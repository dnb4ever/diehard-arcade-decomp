/* enemy_update_variant_script [0x02246d0c]: Handles hit, clash and stage gates before dispatching the variant script and animation. */
#include "dh.h"
/* 0x02246d0c - slim per-object dispatcher (func_02246d0c), sibling of
 * func_02246ab8. Args: r4 = ctx (r13, forwarded only to the shared tail),
 * r5 = obj (r14).
 *   if (obj->0x4C != 0)              func_022489d0(obj);
 *   else if (actor_flag_rng_1in4(obj))       func_02249a82(obj);
 *   else if (*(s32*)0x06097418)      func_02249f08(obj);
 *   else (*(void(**)(void*))(0x06090214 + obj->0x6D * 4))(obj);
 *   actor_anim_frame_update(ctx, obj);           (shared tail, run on every path)
 * fidelity: HIGH - the r12 tail (mova 0x246d1c + 0xFFFE6F10) resolves to the
 * same actor_anim_frame_update as func_02246ab8. Computed handler table is 0x06090214
 * indexed by the signed byte obj->0x6D. m2c lost the bsrf return values. */
void actor_enter_hit_reaction(void *obj);             /* extern */
s32  actor_flag_rng_1in4(void *obj);             /* extern */
void enemy_enter_clash(void *obj);             /* extern */
void enemy_enter_stage_pose18(void *obj);             /* extern */
void actor_anim_frame_update(void *ctx, void *obj);  /* extern (shared tail) */

void enemy_update_variant_script(void *ctx, void *obj)
{
    if (M8(obj, 0x4C) != 0)
        actor_enter_hit_reaction(obj);
    else if (actor_flag_rng_1in4(obj) != 0)
        enemy_enter_clash(obj);
    else if (M32((void *)(uintptr_t)0x06097418, 0) != 0)
        enemy_enter_stage_pose18(obj);
    else
        (*(void (**)(void *))(uintptr_t)(0x06090214 + M8(obj, 0x6D) * 4))(obj);

    actor_anim_frame_update(ctx, obj);
}
