/* prop_tick_triggered_sine_arm [0x0221a80c]: Scene mask triggers sinusoidal angular motion from 8-byte parameter table. */
/* 0x0221a80c - oscillating prop/arm actor, phase 1 (task handler, r4 = task, ent = task->unk8).
 * Drives a member that swings sinusoidally between a rest angle (unk66) and a target, gated by
 * event bits in *0x06098580. Parameters come from the per-actor record 0x060548BC[task->unkF]
 * (stride 8: [0]=start/model word, [2]=amplitude<<4|shift, [4]=flag mask, [6]=hold count).
 * After its state step it ALWAYS tail-calls the shared integrate/draw routine func_02246282(ent).
 *   0: v = rec[0]; ent->unk30 = v & 0x7FFF; ent->unk60 = (v > 0) ? 0 : 1;
 *      ent->unk64 = (u16)ent->unk26; sh = rec[2] & 0xF; hi = rec[2] & ~0xF;
 *      ent->unk68 = (unk60 ? -hi : hi) + unk64; ent->unk6C = hi; ent->unk70 = rec[4];
 *      ent->unk74 = ashr_var(0x4000, sh)  [= 0x4000 >> sh, the per-frame step]; state++.
 *   1: ent->unk26 = ent->unk66; ent->unk58 = 0; ent->unk5C = rec[6]; state++.
 *   2: if (*0x06098580 & ent->unk70) { clear those bits; state++. }
 *   3 (swing out): ent->unk58 += step; if (unk58 >= 0x4000) { unk26 = unk6A; state++; }
 *      else unk26 = (unk60 ? -m : m) + unk66, m = fixmul(unk6C, sin(unk58)).
 *   4: if (--ent->unk5C wrapped from 0) state++.
 *   5 (swing back): ent->unk58 -= step; if (unk58 > 0) { if (!(*0x06098580 & unk70))
 *      unk26 = (unk60 ? -m : m) + unk66; else { clear bits; unk5C = rec[6]; state = 3; } }
 *      else state = 1.
 *   other: no-op.  Always: func_02246282(ent).
 * ABI: r4 = task; no return.
 * fidelity: HIGH - reconstructed from fn.py (m2c mis-merges this with the two sibling functions
 *   that follow, all three ending in a braf tail-jump to 0x246282). bsrf resolved (braf_resolve):
 *   ashr_var 0x24a420, sin_lookup 0x213190, fixmul_16_16 0x2131c8; braf tail-jump 0x21a9dc ->
 *   0x246282 (delta 0x2b8a2) modelled as the final call func_02246282(ent). rec stride 8; the
 *   amplitude word splits into (<<4-aligned) hi and a low-nibble shift count. */
#include "dh_band21.h"

extern s32  shc_shift_right_signed(s32 value, s32 count);   /* ashr_var */
extern s32  sin_lookup(s32 angle);                 /* sin_lookup */
extern s32  fixed_mul16(s32 a, s32 b);           /* fixmul_16_16 */
extern void actor_draw_transformed(u32 ent);                /* shared integrate/draw tail */

void prop_tick_triggered_sine_arm(u32 task)
{
    u32 ent = MEMU32(task + 8);
    u32 rec = 0x060548bc + MEM8(task + 15) * 8;
    s32 step = MEM32(ent + 0x74);

    switch (MEM8(task + 16)) {
    case 0: {
        s16 v = MEM16(rec);
        s32 sh = MEM16(rec + 2) & 0xf;
        s32 hi = MEM16(rec + 2) & ~0xf;
        s32 off;
        MEM16(ent + 0x30) = (s16)(v & 0x7fff);
        MEM32(ent + 0x60) = (v > 0) ? 0 : 1;
        MEM32(ent + 0x64) = (u16)MEM16(ent + 0x26);
        off = (MEM32(ent + 0x60) != 0) ? -hi : hi;
        MEM32(ent + 0x68) = off + MEM32(ent + 0x64);
        MEM32(ent + 0x6c) = hi;
        MEM32(ent + 0x70) = MEM16(rec + 4);
        MEM32(ent + 0x74) = shc_shift_right_signed(0x4000, sh);
        MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
    }
    /* fallthrough */
    case 1:
        MEM16(ent + 0x26) = MEM16(ent + 0x66);
        MEM32(ent + 0x58) = 0;
        MEM32(ent + 0x5c) = MEM16(rec + 6);
        MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        /* fallthrough */
    case 2:
        if (MEM32(DH_SCENE_EVENT_FLAGS_ADDR) & MEM32(ent + 0x70)) {
            MEM32(DH_SCENE_EVENT_FLAGS_ADDR) &= ~MEM32(ent + 0x70);
            MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        }
        break;
    case 3:
        MEM32(ent + 0x58) += step;
        if (MEM32(ent + 0x58) >= 0x4000) {
            MEM16(ent + 0x26) = MEM16(ent + 0x6a);
            MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        } else {
            s32 m = fixed_mul16(MEM32(ent + 0x6c), sin_lookup(MEM32(ent + 0x58)));
            s32 d = (MEM32(ent + 0x60) != 0) ? -m : m;
            MEM16(ent + 0x26) = (s16)(d + MEM16(ent + 0x66));
        }
        break;
    case 4: {
        s32 c = MEM32(ent + 0x5c) - 1;
        MEM32(ent + 0x5c) = c;
        if (c + 1 == 0)
            MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        break;
    }
    case 5: {
        s32 x = MEM32(ent + 0x58) - step;
        MEM32(ent + 0x58) = x;
        if (x > 0) {
            if ((MEM32(DH_SCENE_EVENT_FLAGS_ADDR) & MEM32(ent + 0x70)) == 0) {
                s32 m = fixed_mul16(MEM32(ent + 0x6c), sin_lookup(MEM32(ent + 0x58)));
                s32 d = (MEM32(ent + 0x60) != 0) ? -m : m;
                MEM16(ent + 0x26) = (s16)(d + MEM16(ent + 0x66));
            } else {
                MEM32(DH_SCENE_EVENT_FLAGS_ADDR) &= ~MEM32(ent + 0x70);
                MEM32(ent + 0x5c) = MEM16(rec + 6);
                MEM8(task + 16) = 3;
            }
        } else {
            MEM8(task + 16) = 1;
        }
        break;
    }
    default:
        break;
    }

    actor_draw_transformed(ent);
}
