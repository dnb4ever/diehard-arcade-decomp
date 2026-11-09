/* prop_tick_pitch_cycle [0x0221a9e0]: Six phases rotate pitch through sine arc, hold eight ticks and return. */
#include "dh_work.h"
/* 0x0221a9e0 - 6-state sinusoidal step state machine (task handler, ent = task->unk8).
 * step = ent->unk64 (init 0x400), angle accumulates in ent->unk58, base = ent->unk62.
 *   0: ent->unk30 = tab[task->unkF] @0x0605449A; unk60=0; unk24=0; unk64=0x400.
 *   1: unk58=0; unk24=unk62; state++.
 *   2: if (*0x06098580 & 4) { clear; state++. }
 *   3: unk58 += step; if (<0x4000) unk24 = unk62 - fixmul(0x4000,sin(unk58));
 *      else { unk24 = unk62+0xC000; unk5C=8; state++. }
 *   4: if (--unk5C wrapped to -1) state++.
 *   5: unk58 -= step; if (>0) unk24 = unk62 - fixmul(0x4000,sin(unk58)); else state=1.
 * Tail: func_02246282(ent). */
#include "dh.h"

extern s32  sin_lookup(s32 angle);                /* func_02213190 */
extern s32  fixed_mul16(s32 a, s32 b);          /* fixmul_16_16 */
extern void actor_draw_transformed(u32 ent);               /* shared integrate/draw tail */

void prop_tick_pitch_cycle(u32 task)
{
    u32 ent = MU32(task, TASK_WORK);
    s32 step = M32(ent, 0x64);
    s8 state = (s8)TASK_S8(task, state);

    switch (state) {
    case 0:
        M16(ent, 0x30) = (s16)M32(0x0605449a + (u32)M8(task, 15) * 2, 0);
        M32(ent, 0x60) = 0;
        M16(ent, 0x24) = 0;
        M32(ent, 0x64) = 0x400;
        /* fallthrough */
    case 1:
        M32(ent, 0x58) = 0;
        M16(ent, 0x24) = (s16)M16(ent, 0x62);
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        /* fallthrough */
    case 2:
        if (M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) & 4) {
            M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) &= ~4;
            TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        }
        break;
    case 3: {
        s32 x = M32(ent, 0x58) + step;
        M32(ent, 0x58) = x;
        if (x < 0x4000) {
            s32 v = fixed_mul16(0x4000, sin_lookup(M32(ent, 0x58)));
            M16(ent, 0x24) = (s16)(M16(ent, 0x62) - v);
        } else {
            M16(ent, 0x24) = (s16)(M16(ent, 0x62) + 0xc000);
            M32(ent, 0x5c) = 8;
            TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        }
        break;
    }
    case 4: {
        s32 c = M32(ent, 0x5c) - 1;
        M32(ent, 0x5c) = c;
        if (c + 1 == 0)
            TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        break;
    }
    case 5: {
        s32 x = M32(ent, 0x58) - step;
        M32(ent, 0x58) = x;
        if (x > 0) {
            s32 v = fixed_mul16(0x4000, sin_lookup(M32(ent, 0x58)));
            M16(ent, 0x24) = (s16)(M16(ent, 0x62) - v);
        } else {
            TASK_S8(task, state) = 1;
        }
        break;
    }
    default:
        break;
    }

    actor_draw_transformed(ent);
}
