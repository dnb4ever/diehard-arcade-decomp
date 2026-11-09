#include "dh_work.h"
/* actor_enter_directional_launch [0x02241324]: Seeds launch speed/pitch and resolves the launch animation through the direction table. */
#include "dh.h"
/* 0x02241324 - actor facing-sprite resolve (sibling of func_0224124e). Sets up
 * motion state, then resolves obj->0x5C through a 3-level pointer table.
 * Args: obj=r4, out=r5 (s32*).
 *   obj->0x108 = 1;
 *   obj->0x74  = 0x4000;             // 32-bit
 *   obj->0x60  = 0x2000;             // 16-bit
 *   actor_motion_init(obj);               // actor_motion_init (may rewrite 0x60)
 *   obj->0x78  = obj->0x78 * 2;
 *   obj->0x84  = 0x1000;
 *   *out = 2;
 *   q60 = ((u16)obj->0x60 + 0x6000) >> 14;                          // 14x shar
 *   q62 = ((u16)obj->0x62 + (obj->0x44 ? 0x9000 : 0x1000)) >> 11;   // 11x shar
 *   idx = ((u16)q62 & 0x1C) + ((u16)q60 & 3);
 *   pA  = ((void**)obj->0x164)[*out];
 *   pB  = *(void**)((u8*)pA + 12);
 *   pC  = *(void**)pB;
 *   obj->0x5C = *(s16*)((u8*)pC + idx*2);
 * fidelity: HIGH - shar counts 14/11 verified; shifted values non-negative. */
void actor_motion_init(void *a);   /* extern (actor_motion_init) */

void actor_enter_directional_launch(void *obj, s32 *out)
{
    s32 q60, q62, idx, dir;
    void *pA, *pB, *pC;

    ACTOR_M32(obj, action_phase) = 1;
    ACTOR_M32(obj, motion_speed)  = 0x4000;
    ACTOR_M16(obj, motion_pitch)  = 0x2000;
    actor_motion_init(obj);
    M32(obj, 0x78)  = M32(obj, 0x78) * 2;
    ACTOR_M32(obj, action_category)  = 0x1000;
    *out = 2;

    q60 = ((s32)(u16)ACTOR_M16(obj, motion_pitch) + 0x6000) >> 14;
    dir = ACTOR_M8(obj, facing_right) ? 0x9000 : 0x1000;
    q62 = ((s32)(u16)ACTOR_M16(obj, motion_heading) + dir) >> 11;
    idx = ((u16)q62 & 0x1C) + ((u16)q60 & 3);

    pA = ((void **)ACTOR_MP(obj, response_config_tables))[*out];
    pB = MP(pA, 12);
    pC = MP(pB, 0);
    ACTOR_M16(obj, animation_requested) = *(s16 *)((u8 *)pC + idx * 2);
}
