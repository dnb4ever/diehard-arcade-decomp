/* actor_hit_countdown [0x0222ac60]: Existing map corroborated by source: (member) (entity,dec): entity->unk56-=dec; if <=0 clamp */
#include "dh_work.h"
#include "dh.h"
/* 0x0222ac60 actor_hit_countdown(entity, dec) - subtracts dec from entity->unk56; when
 * it reaches <= 0 it clamps to 0 and, if the matching join slot is still empty and the
 * two-player game is armed and far enough in (g_06098830 >= 5), fires a late-join spawn:
 * sets g_06098830 = 5, (*g_0609882C)->unkD = 1, and into (*g_0609882C)->unk8 (an entity):
 *   ->unkAE = entity->unk34; ->unkB0 = func_02210020(); ->unkB2 = 0.
 * Always clears *(s16)g_06099798 = 0 on exit.
 *   slot = (entity->unk34 == 1) ? 2 : 1;  guard: g_06099590[slot]==0 && g_06098670==1
 * fidelity: HIGH - func_02210020 is void/u32 (existing file); r4 at its call is the
 * ORIGINAL entity arg (never reassigned); bsrf resolved to 0x210020. m2c dropped the
 * bsrf (so its ->unkB0 = 0xAE is wrong - that word is the call return). */
extern void *g_actor_slot_ids[];
extern s32   g_play_session_mode;
extern s32   g_view_request_priority;
extern void *g_view_task_legacy;
extern s16   g_06099798;
extern u32   game_random_next(void);

void actor_hit_countdown(void *entity, s32 dec) {
    ACTOR_M16(entity, health) = (s16)(ACTOR_M16(entity, health) - dec);
    if (ACTOR_M16(entity, health) <= 0) {
        s32 slot;
        ACTOR_M16(entity, health) = 0;
        slot = (M8(entity, ACTOR_SLOT_ID) == 1) ? 2 : 1;
        if (g_actor_slot_ids[slot] == 0 && g_play_session_mode == 1 && g_view_request_priority >= 5) {
            void *ent2 = TASK_PTR(g_view_task_legacy, work);
            g_view_request_priority = 5;
            M8(g_view_task_legacy, 0xD) = 1;
            M16(ent2, 0xAE) = M8(entity, ACTOR_SLOT_ID);
            M16(ent2, 0xB0) = (s16)game_random_next();
            M16(ent2, 0xB2) = 0;
        }
    }
    g_06099798 = 0;
}
