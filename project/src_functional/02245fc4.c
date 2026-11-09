#include "dh_work.h"
/* enemy_classify_attack_timing [0x02245fc4]: Classifies target action and elapsed timestamps against two duration tables. */
#include "dh.h"
/* 0x02245fc4  classify an actor's timing/state -> 0/1/2/3.
 * base = field[0x6d]*8 + field[0xe8] (index into two duration tables).
 *   if (q->field[0x84] & 0xA00E3800)                       -> 1
 *   if ((now - field[0x114]) <  g_0608dc04[base])          -> 2   (signed)
 *   if ((now - g_06099780[q->field[0x34]-1]) < g_0608dce4[base]) -> 3
 *   else                                                   -> 0
 * now = g_060987bc.
 * fidelity: HIGH - exact (cmp/ge signed; q is 2nd arg). */
extern u32 g_frame_tick_legacy;
extern s32 g_0608dc04[];
extern s32 g_0608dce4[];
extern s32 g_06099780[];
s32 enemy_classify_attack_timing(void *p, void *q) {
    s32 base = ACTOR_M8(p, secondary_effect_type) * 8 + M32(p, 0xE8);
    if (ACTOR_M32(q, action_category) & 0xA00E3800)
        return 1;
    u32 now = g_frame_tick_legacy;
    s32 e1 = (s32)(now - ACTOR_MU32(p, action_start_tick));
    if (e1 < g_0608dc04[base])
        return 2;
    s32 e2 = (s32)(now - (u32)g_06099780[ACTOR_M8(q, slot_id) - 1]);
    if (e2 < g_0608dce4[base])
        return 3;
    return 0;
}
