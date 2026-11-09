#include "dh_work.h"
/* actor_enter_random_launch [0x022413e6]: Randomizes launch pitch, resolves its animation and optionally queues the action cue. */
#include "dh.h"
/* 0x022413e6 - actor facing-sprite resolve with randomized launch angle and an
 * optional PIC trigger (sibling of func_0224124e / func_02241324).
 * Args: obj=r4, out=r5 (s32*).
 *
 * SCOPE: fn.py reports [0x2413e6, 0x2414ee) (code 0x108). find_entry flags
 * 0x24148e as a "real" entry, but it is INTERIOR to this function (it begins
 * mid-expression with `shar r5`, no prologue) - it is NOT a separate function.
 *
 *   ptrB = 0x06054080 + (*(s8*)(0x060540E8 + obj->0x5E)) * 8;   // saved for PIC
 *   rand = func_02210020();                        // no args
 *   obj->0x108 = 1;
 *   obj->0x74  = 0x4000;
 *   obj->0x60  = (rand & 0xFFF) + 0x2800;         // 16-bit
 *   actor_motion_init(obj);                             // actor_motion_init
 *   obj->0xF0 += (((u16)rand >> 16) & 0x7FF) + 0x1400;
 *   obj->0x84  = 0x1000;
 *   *out = 2;
 *   q62 = ((u16)obj->0x62 + (obj->0x44 ? 0x9000 : 0x1000)) >> 11;  // 11x shar
 *   idx = ((u16)q62 & 0x1C) + 2;
 *   pA  = ((void**)obj->0x164)[*out];
 *   pB  = *(void**)((u8*)pA + 12);
 *   pC  = *(void**)pB;
 *   obj->0x5C = *(s16*)((u8*)pC + idx*2);
 *   if (obj->0x56 > 0 && *(s32*)0x060E0DDC != 0) {
 *       u8 k = *(u8*)ptrB;
 *       sound_queue_push((*(s16*)(0x06053F04 + k*2) << 8) | 0xA00000FF);
 *   }
 *
 * fidelity: HIGH - NOTE the "(u16)rand >> 16" term is a dead (always-zero)
 * remnant of the compiled code (the value is zero-extended to 16 bits by extu.w
 * before the shlr16), kept faithful: the net effect is obj->0xF0 += 0x1400.
 * shar count 11 verified; PIC command format matches func_022402c0. */
s32  game_random_next(void);        /* extern (no args) */
void actor_motion_init(void *a);   /* extern (actor_motion_init) */
void sound_queue_push(s32 code);    /* extern (PIC trigger) */

void actor_enter_random_launch(void *obj, s32 *out)
{
    s32 rand, q62, idx, dir;
    u32 hi;
    void *pA, *pB, *pC, *ptrB;

    ptrB = (void *)(uintptr_t)(0x06054080 +
            (s32)M8((void *)(uintptr_t)(0x060540E8 + (s32)ACTOR_M16(obj, model_index)), 0) * 8);

    rand = game_random_next();

    ACTOR_M32(obj, action_phase) = 1;
    ACTOR_M32(obj, motion_speed)  = 0x4000;
    ACTOR_M16(obj, motion_pitch)  = (s16)((rand & 0xFFF) + 0x2800);
    actor_motion_init(obj);

    hi = (u16)rand;          /* extu.w */
    hi >>= 16;               /* shlr16 -> always 0 */
    hi &= 0x7FF;
    M32(obj, 0xF0) += (s32)hi + 0x1400;

    ACTOR_M32(obj, action_category) = 0x1000;
    *out = 2;

    dir = ACTOR_M8(obj, facing_right) ? 0x9000 : 0x1000;
    q62 = ((s32)(u16)ACTOR_M16(obj, motion_heading) + dir) >> 11;
    idx = ((u16)q62 & 0x1C) + 2;

    pA = ((void **)ACTOR_MP(obj, response_config_tables))[*out];
    pB = MP(pA, 12);
    pC = MP(pB, 0);
    ACTOR_M16(obj, animation_requested) = *(s16 *)((u8 *)pC + idx * 2);

    if (ACTOR_M16(obj, health) > 0 && M32((void *)(uintptr_t)DH_SOUND_EFFECTS_ENABLED_ADDR, 0) != 0) {
        u8 k = MU8(ptrB, 0);
        sound_queue_push(((s32)M16((void *)(uintptr_t)(0x06053F04 + k * 2), 0) << 8)
                    | (s32)DH_SOUND_CUE_BASE);
    }
}
