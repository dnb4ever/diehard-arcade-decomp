/* prop_tick_axis_gate [0x0221ab1c]: Six states select X/Z pointer from table flags and apply trigger-driven motion. */
#include "dh_work.h"
/* 0x0221ab1c - 6-state door/gate movement state machine (task handler, ent = task->unk8).
 * base table entry at 0x06054904 + 8*task->unkF: {s16 flags, s16 rate, s16 dur, s16 pad}.
 * flags bit15 selects ent+0x10 (else ent+0x8) as the "value" pointer cached in ent->unk64;
 * flags bit14 gives movement direction; flags & 0x3fff seeds ent->unk30.
 * rate>>4 * 16 gives the per-frame step (ent->unk70); dur seeds the frame counter unk5C.
 *   0: parse entry, unk6C = unk68 ± step, unk74 = dur, unk78 = func_0224a420(0x4000, rate & 0xf),
 *      state++; fallthrough
 *   1: *unk64 = unk68; unk58 = 0; unk5C = dur; state++; fallthrough
 *   2: if (*0x06098580 & unk74) { clear; state++; }
 *   3: unk58 += unk78; if (>= 0x4000) { *unk64 = unk6C; state++; }
 *      else *unk64 = unk68 ± fixmul_16_16(unk70, sin_lookup(unk58)) (sign from unk60).
 *   4: if (--unk5C wrapped to -1) state++.
 *   5: unk58 -= unk78; if (> 0) { if (!(*0x06098580 & unk74))
 *          *unk64 = unk68 ± fixmul_16_16(unk70, sin_lookup(unk58));
 *      else { clear bit; *unk64 = unk68; unk5C = dur; state = 3; } }
 *      else state = 1.
 * Tail: func_02246282(ent). fidelity: hand-traced; bsrf targets resolved
 *   (sin_lookup 0x02213190, fixmul 0x022131c8, func_0224a420, func_02246282).
 *   func_0224a420 takes its first argument in r0 (custom convention: mov r7,r0 delay slot). */
#include "dh.h"

extern s32  sin_lookup(s32 angle);       /* func_02213190 */
extern s32  fixed_mul16(s32 a, s32 b); /* fixmul_16_16 */
extern s32  shc_shift_right_signed(s32 value, s32 count);
extern void actor_draw_transformed(u32 ent);      /* shared integrate/draw tail */

void prop_tick_axis_gate(u32 task)
{
    u32 ent = MU32(task, TASK_WORK);
    u32 base = 0x06054904 + (u32)M8(task, 15) * 8;
    u32 ptr = M32(ent, 0x64);
    s32 step = M32(ent, 0x78);
    s8 state = (s8)TASK_S8(task, state);

    switch (state) {
    case 0: {
        s16 flags = (s16)M16(base, 0);
        s16 rate;
        s32 mag;
        M16(ent, 0x30) = (s16)(flags & 0x3fff);
        M32(ent, 0x60) = (s32)flags & 0x4000;
        ptr = (flags < 0) ? ent + 0x10 : ent + 0x8;
        M32(ent, 0x64) = ptr;
        M32(ent, 0x68) = M32(ptr, 0);
        rate = (s16)M16(base, 2);
        mag = (rate & ~0xf) << 4;
        M32(ent, 0x6c) = (M32(ent, 0x60) ? -mag : mag) + M32(ent, 0x68);
        M32(ent, 0x70) = mag;
        M32(ent, 0x74) = (s32)(s16)M16(base, 4);
        M32(ent, 0x78) = shc_shift_right_signed(0x4000, rate & 0xf);
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        step = M32(ent, 0x78);
        /* fallthrough */
    }
    case 1:
        M32(ptr, 0) = M32(ent, 0x68);
        M32(ent, 0x58) = 0;
        M32(ent, 0x5c) = (s32)(s16)M16(base, 6);
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        /* fallthrough */
    case 2:
        if (M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) & M32(ent, 0x74)) {
            M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) &= ~M32(ent, 0x74);
            TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        }
        break;
    case 3: {
        s32 x = M32(ent, 0x58) + step;
        M32(ent, 0x58) = x;
        if (x >= 0x4000) {
            M32(ptr, 0) = M32(ent, 0x6c);
            TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        } else {
            s32 v = fixed_mul16(M32(ent, 0x70), sin_lookup(M32(ent, 0x58)));
            M32(ptr, 0) = (M32(ent, 0x60) ? -v : v) + M32(ent, 0x68);
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
            if (!(M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) & M32(ent, 0x74))) {
                s32 v = fixed_mul16(M32(ent, 0x70), sin_lookup(M32(ent, 0x58)));
                M32(ptr, 0) = (M32(ent, 0x60) ? -v : v) + M32(ent, 0x68);
            } else {
                M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) &= ~M32(ent, 0x74);
                M32(ptr, 0) = M32(ent, 0x68);
                M32(ent, 0x5c) = (s32)(s16)M16(base, 6);
                TASK_S8(task, state) = 3;
            }
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
