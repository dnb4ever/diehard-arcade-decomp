#include "dh_work.h"
/* actor_finish_arrival [0x022418fa]: Applies one-shot arrival effects and sound after the actor's count expires. */
#include "dh.h"
/* 0x022418fa - func_022418fa(ctrl, entity, play_sound): post-action "arrival"
 * hook. When the entity's countdown (+0x56) has run out and the one-shot flag
 * (+0xE0 bit 0x800) is clear, it optionally spawns helper actors, applies a
 * control-byte reaction, and plays an action sound; then it latches the flag.
 *   if (entity->0x56 > 0) return;              // still counting -> nothing
 *   r = rng_next();
 *   rec = (u8*)0x06054080 + g_060540e8[entity->0x5E] * 8;
 *   if ((entity->0xE0 & 0x800) == 0) {
 *     if (g_0609958c > 1) {                    // >1 player active
 *       if ((r & 3) == 0)
 *         func_0223ab04(entity, ((r >> 4) & 3) == 0 ? 14 : 13);
 *       t = (s8)(ctrl[15] & 0xC0);
 *       if (t != 0) func_0223ab5c(entity, (t >> 6) & 3);
 *     }
 *     if (play_sound != 0 && g_060e0ddc != 0)
 *       sound_queue_push((g_06053f04[(u8)rec[0]] << 8) | 0xA00000FF);
 *   }
 *   entity->0xE0 |= 0x800;
 * fidelity: HIGH - all 4 resolved bsrf plus the g_060540e8 table read (the
 * reported "TABLE/COMPUTED" braf is that mov.b table index, not a branch). +0x56
 * is s16; the rng recombine uses arithmetic >>4 then &3; the control reaction
 * casts the masked byte to s8 before the arithmetic >>6 (so 0x00/40/80/C0 map to
 * 0/1/2/3). rec[0] is zero-extended before indexing the u16 sound-id table. */
extern s32  game_random_next(void);                        /* 0x02210020 rng_next */
extern void actor_arm_effect_with_motion(void *entity, s32 arg);       /* 0x0223ab04 */
extern void actor_spawn_secondary_effect(void *entity, s32 arg);       /* 0x0223ab5c */
extern void sound_queue_push_entry(s32 cmd);                     /* 0x02215f6c sound_queue_push */
extern s32  g_0609958c;                                 /* active-player count */
extern s32  g_sound_effects_enabled;                                 /* sound-enable gate */
extern s8   g_060540e8[];                               /* u5E -> record index (s8) */
extern s16  g_06053f04[];                               /* action sound-id table */

void actor_finish_arrival(void *ctrl, void *entity, s32 play_sound)
{
    s32 r;
    u8 *rec;

    if (ACTOR_M16(entity, health) > 0)
        return;

    r = game_random_next();
    rec = (u8 *)(uintptr_t)0x06054080U + (s32)M8(g_060540e8, ACTOR_M16(entity, model_index)) * 8;

    if ((ACTOR_M32(entity, state_flags) & 0x800) == 0) {
        if (g_0609958c > 1) {
            if ((r & 3) == 0)
                actor_arm_effect_with_motion(entity, (((r >> 4) & 3) == 0) ? 14 : 13);
            {
                s8 t = (s8)(M8(ctrl, 15) & 0xC0);
                if (t != 0)
                    actor_spawn_secondary_effect(entity, (t >> 6) & 3);
            }
        }
        if (play_sound != 0 && g_sound_effects_enabled != 0)
            sound_queue_push_entry(((s32)M16(g_06053f04, (u8)M8(rec, 0) * 2) << 8) | DH_SOUND_CUE_BASE);
    }

    ACTOR_M32(entity, state_flags) |= 0x800;
}
