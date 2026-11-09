#include "dh_work.h"
/* hud_queue_action_label [0x0223e104]: Queues a twelve-frame action label and caches the actor's displayed flags. */
#include "dh.h"
/* 0x0223e104 (callC) - n = *(s8)(arg0+52) - 1.
 * If *(s8)(arg0+109) < 12 && (u32)n < 2 && (u8)0x060E106C[ *(s8)(arg0+109) ] < 23:
 *   e = 0x0609860C + 8*n; (e+0)=1 (word); (e+2) = (s8)0x060E106C[ *(s8)(arg0+109) ] (word);
 *   (e+4)=12 (long).
 * Always: call func_022380c0(4, (s16)0x0608D1CC[n], 12, 0);
 *   then *(0x060E0E58 + 16*n) = *(arg0+0x8C).
 * bsrf cart 0x2380c0 -> RAM 0x0603c0c0.
 * fidelity: HIGH - tables: 0x60E106C byte, 0x609860C stride 8, 0x608D1CC word, 0x60E0E58 stride 16. */
extern s8  g_060E106C;
extern s16 g_0609860C;
extern s16 g_0608D1CC;
extern s32 g_060E0E58;
extern void hud_draw_counter_icon(s32, s32, s32, s32);
void hud_queue_action_label(void *arg0) {
    s32 n = ACTOR_M8(arg0, slot_id) - 1;
    if (ACTOR_M8(arg0, secondary_effect_type) < 12 && (u32)n < 2 && (u8)(&g_060E106C)[ ACTOR_M8(arg0, secondary_effect_type) ] < 23) {
        void *e = (char *)&g_0609860C + 8 * n;
        M16(e, 0) = 1;
        M16(e, 2) = (&g_060E106C)[ ACTOR_M8(arg0, secondary_effect_type) ];
        M32(e, 4) = 12;
    }
    hud_draw_counter_icon(4, M16(&g_0608D1CC, n * 2), 12, 0);
    M32(&g_060E0E58, 16 * n) = ACTOR_M32(arg0, interaction_flags);
}
