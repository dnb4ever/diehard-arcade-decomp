/* enemy_animate_skeletal_category [0x0223e8ca]: Handles clash or hit before dispatching the skeletal variant table and animation. */
#include "dh.h"
/* func_0223e8ca(task, entity): per-frame actor update. If func_02246208(entity) != 0,
 * run func_02249a82(entity); else if entity->unk4C != 0 run func_022489d0(entity);
 * else dispatch g_06090214[unk6D](entity). Always tail-run func_0222dc2c(task, entity)
 * (animation_update).
 * fidelity: HIGH - offsets 0x4C/0x6D, 4-byte table g_06090214, and func_0222dc2c
 *   (mova+add) verified; computed calls resolved via braf. */
extern s32  actor_flag_rng_1in4_entry(void *ent);
extern void enemy_enter_clash(void *ent);
extern void actor_enter_hit_reaction(void *ent);
extern void actor_anim_frame_update_entry(void *task, void *ent);   /* animation_update */
extern u8   g_06090214[];   /* fn-ptr table (stride 4) */

void enemy_animate_skeletal_category(void *task, void *ent) {
    if (actor_flag_rng_1in4_entry(ent) != 0) {
        enemy_enter_clash(ent);
    } else if (M8(ent, 0x4C) != 0) {
        actor_enter_hit_reaction(ent);
    } else {
        ((void (*)(void *))MP(g_06090214, M8(ent, 0x6D) * 4))(ent);
    }
    actor_anim_frame_update_entry(task, ent);
}
