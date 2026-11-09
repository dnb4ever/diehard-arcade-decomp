/* enemy_animate_category_e [0x0223fc58]: Handles clash or hit before dispatching the fifth variant table and animation. */
#include "dh.h"
/* 0x0223fc58 - func_0223fc58: actor post/render dispatcher (ctx, ent).
 * Twin of 0x0223e632 (second actor category).
 *   If actor_flag_rng_1in4(ent) != 0: func_02249a82(ent).
 *   else if (s8)ent->unk4C != 0: func_022489d0(ent).
 *   else: g_0608d808[(s8)ent->unk6D](ent).
 *   then func_0222dc2c(ctx, ent) via jsr @r12,
 *     r12 = mova(.Lpool_23fc6c)+0xFFFEDFC0 = 0x0222dc2c.
 * fidelity: HIGH - bsrf targets resolved: 0x246208, 0x249a82, 0x2489d0. */
extern void (*g_0608d808[])(void *ent);
extern s32  actor_flag_rng_1in4(void *ent);
extern void enemy_enter_clash(void *ent);
extern void actor_enter_hit_reaction(void *ent);
extern void actor_anim_frame_update_entry(void *ctx, void *ent);

void enemy_animate_category_e(void *ctx, void *ent) {
    if (actor_flag_rng_1in4(ent) != 0)
        enemy_enter_clash(ent);
    else if (M8(ent, 0x4C) != 0)
        actor_enter_hit_reaction(ent);
    else
        g_0608d808[M8(ent, 0x6D)](ent);
    actor_anim_frame_update_entry(ctx, ent);
}
