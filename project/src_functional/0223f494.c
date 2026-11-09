/* enemy_arm_distance_band [0x0223f494]: Classifies the paired actor's X distance and arms the corresponding attack band. */
#include "dh.h"
/* 0x0223f494 - func_0223f494(entity): proximity-band trigger. Finds the paired
 * actor, and if the shutter/gate flag g_06098580 is clear and the two are more
 * than 0x28000 apart on axis +8, classifies the partner's +8 value into one of
 * up to 5 distance bands (table g_0608d610) and arms the entity's attack:
 *   v   = func_02245aec(entity);
 *   par = g_060995b0[(v & 1) + 1];
 *   val = *(*(par+8) + 8);                     // partner position (axis +8)
 *   if (g_06098580 != 0) return 0;             // shutter active -> no trigger
 *   d = |val - entity->8|;
 *   if (d > 0x28000) {
 *     for (i = 0; i < 5; i++)
 *       if (val < g_0608d610[i]) {             // first band whose bound exceeds val
 *         entity->0xD8 = 1; entity->0x41 = 1; entity->0x5C = 0x28F;
 *         entity->0x100 = func_0224a360(1, i); // 1 << i (shl helper, r0/r1 ABI)
 *         return 1;
 *       }
 *   }
 *   return 0;
 * fidelity: HIGH - both bsrf resolve; the band loop runs at most 5 (r7<5) and
 * the match index equals r6. func_0224a360 is the on-cart variable-left-shift
 * primitive, called with r0=value(1), r1=amount(i), i.e. 1<<i. g_06098580 is
 * the same global tested in func_0223f54e. +0x5C stored as u16 (0x028F). */
extern s32   enemy_select_farther_player_side(void *entity);      /* 0x02245aec */
extern s32   shc_shift_left(s32 val, s32 amt);  /* 0x0224a360 shl helper (r0=val, r1=amt) */
extern void *g_actor_slot_tasks_legacy[];                     /* paired-actor pointer table */
extern u32   g_scene_event_flags;                       /* shutter/gate flag word */
extern s32   g_0608d610[];                      /* distance-band bounds (>=5 entries) */

s32 enemy_arm_distance_band(void *entity)
{
    s32 v = enemy_select_farther_player_side(entity);
    void *par = g_actor_slot_tasks_legacy[(v & 1) + 1];
    s32 val = M32(MP(par, 8), 8);
    s32 d;
    s32 i;

    if (g_scene_event_flags != 0)
        return 0;

    d = val - M32(entity, 8);
    if (d < 0)
        d = -d;

    if (d > 0x28000) {
        for (i = 0; i < 5; i++) {
            if (val < g_0608d610[i]) {
                M32(entity, 0xD8) = 1;
                M8(entity, 0x41) = 1;
                M16(entity, 0x5C) = 0x28F;
                M32(entity, 0x100) = shc_shift_left(1, i);
                return 1;
            }
        }
    }
    return 0;
}
