/* enemy_update_hit_or_clash [0x02249f52]: Handles hit, random clash and animation-ready transitions before animation. */
#include "dh.h"
/* 0x02249f52 - actor per-tick dispatch. If +0x4C set, run func_022489d0; else if
 * actor_flag_rng_1in4 returns nonzero, run func_02249a82; else if +0x54 bit 2 set, run
 * func_022469f2 and copy +0x140 into +0xBC. Finally tail into animation_update
 * (actor_anim_frame_update) with (arg0, a).
 * fidelity: HIGH - +0x4C mov.b, +0x54 mov.w, +0xBC/+0x140 mov.l; PIC targets resolved. */
extern void actor_enter_hit_reaction(void *a);
extern s32  actor_flag_rng_1in4(void *a);
extern void enemy_enter_clash(void *a);
extern void enemy_reset_idle_or_walk(void *a);
extern void actor_anim_frame_update(s32 arg0, void *a);   /* animation_update */

void enemy_update_hit_or_clash(s32 arg0, void *a) {
    if (M8(a, 0x4C) != 0) {
        actor_enter_hit_reaction(a);
    } else if (actor_flag_rng_1in4(a) != 0) {
        enemy_enter_clash(a);
    } else if (M16(a, 0x54) & 2) {
        enemy_reset_idle_or_walk(a);
        M32(a, 0xBC) = M32(a, 0x140);
    }
    actor_anim_frame_update(arg0, a);
}
