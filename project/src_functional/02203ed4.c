#include "dh_work.h"
#include "dh.h"
/* 0x02203ed4 (fx_sprite_ring_task) - task handler that plots a rotating ring/loop of
 * point sprites with a randomised per-period wobble. arg0 = task; obj = task->unk8 is
 * the sprite object whose unk8/unkC (x,y) are recomputed and emitted for each point.
 * task->unk10 is the task state byte (0 = first-run init, then 1 = run).
 *
 * state 0 (init, falls through to state 1):
 *   obj->unk30 = 1; obj->unk32 = 0; obj->unk84 = 0;      // phase accumulator
 *   obj->unk60 = 0x40;   // X amplitude    obj->unk64 = 0x10;  // Y amplitude
 *   obj->unk68 = 0x10000; // twist (16.16) obj->unk6C = unk70 = unk74 = 0;
 *   obj->unk78 = 0x20;   // reseed countdown   obj->unk7C = 0; // wobble flag bits
 *   task->unk10++;
 *
 * state 1 (per frame):
 *   every 0x20 frames reseed the wobble from the PRNG:
 *     obj->unk7C = rng_next() & 0x3F;  obj->unk6C = rng_next() & 7;
 *     obj->unk70 = rng_next() & 7;     obj->unk74 = rng_next() & 7;
 *   apply the flagged wobble steps (bit1 gate / bit0 sign -> unk60 += +-unk6C;
 *     bit7 / bit6 -> unk64 += +-unk70; bit5 / bit4 -> unk68 += +-(unk74 << 11)),
 *   clamp unk60 to [-0x50,0x50], unk64 to [-0x30,0x30], unk68 to [-0x50000,0x50000].
 *   advance phase unk84 += 0x180, then sweep the ring angle t from 0 by 0x400 while
 *   (u32)t < 0xFFFF, emitting one sprite per step:
 *     f  = fixmul_16_16(t, unk68);                         // twisted secondary angle
 *     x  = (((s16)((cos(unk84+t)*unk60 + cos(unk86+f)*unk64) >> 16)) - 8) * 2;
 *     y  =  ((s16)((sin(unk86+f)*unk64 + sin(unk84+t)*unk60) >> 16)) + 8;
 *     obj->unk8 = x; obj->unkC = y; draw_point_sprite(obj);
 *
 * fidelity: HIGH - hand-traced vs m2c_fn --asm (0x203ed4..0x20419e); zero computed
 * branches. mova+add call targets resolved: rng_next (0x210020), cos_lookup (0x2131a8),
 * sin_lookup (0x213190), fixmul_16_16 (0x2131c8), draw_point_sprite (0x2153a0, sprite
 * display-list emitter). unk84 is a 32-bit phase; unk86 is its adjacent low halfword.
 * The compiler unrolled the ring loop 2x over identical bodies; re-rolled here. The
 * unk74<<11 step is an explicit shll8/shll2/shll (not a dropped-shift). */

extern s32  rng_next(void);                     /* 0x210020 */
extern s32  cos_lookup(u16 angle);              /* 0x2131a8 */
extern s32  sin_lookup(u16 angle);              /* 0x213190 */
extern s32  fixmul_16_16(s32 a, s32 b);         /* 0x2131c8 */
extern void draw_point_sprite(void *obj);       /* 0x2153a0 */

void fx_sprite_ring_task(void *task) {
    void *obj = TASK_PTR(task, work);
    s32 t, f, x, y;

    switch ((s8)TASK_S8(task, state)) {
    case 0:
        M16(obj, 0x30) = 1;
        M16(obj, 0x32) = 0;
        M32(obj, 0x84) = 0;
        M32(obj, 0x60) = 0x40;
        M32(obj, 0x64) = 0x10;
        M32(obj, 0x68) = 0x10000;
        M32(obj, 0x6C) = 0;
        M32(obj, 0x70) = 0;
        M32(obj, 0x74) = 0;
        M32(obj, 0x78) = 0x20;
        M32(obj, 0x7C) = 0;
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        /* fallthrough */
    case 1:
        if (--M32(obj, 0x78) == 0) {
            M32(obj, 0x78) = 0x20;
            M32(obj, 0x7C) = rng_next() & 0x3F;
            M32(obj, 0x6C) = rng_next() & 7;
            M32(obj, 0x70) = rng_next() & 7;
            M32(obj, 0x74) = rng_next() & 7;
        }

        if (M32(obj, 0x7C) & 2)
            M32(obj, 0x60) += (M32(obj, 0x7C) & 1) ? M32(obj, 0x6C) : -M32(obj, 0x6C);
        if (M32(obj, 0x7C) & 0x80)
            M32(obj, 0x64) += (M32(obj, 0x7C) & 0x40) ? M32(obj, 0x70) : -M32(obj, 0x70);
        if (M32(obj, 0x7C) & 0x20)
            M32(obj, 0x68) += (M32(obj, 0x7C) & 0x10)
                                  ? (M32(obj, 0x74) << 11)
                                  : -(M32(obj, 0x74) << 11);

        if (M32(obj, 0x60) < -0x50)      M32(obj, 0x60) = -0x50;
        else if (M32(obj, 0x60) > 0x50)  M32(obj, 0x60) = 0x50;
        if (M32(obj, 0x64) < -0x30)      M32(obj, 0x64) = -0x30;
        else if (M32(obj, 0x64) > 0x30)  M32(obj, 0x64) = 0x30;
        if (M32(obj, 0x68) < -0x50000)   M32(obj, 0x68) = -0x50000;
        else if (M32(obj, 0x68) > 0x50000) M32(obj, 0x68) = 0x50000;

        M32(obj, 0x84) += 0x180;

        for (t = 0; (u32)t < 0xFFFF; t += 0x400) {
            s32 cx = cos_lookup((u16)(M32(obj, 0x84) + t)) * M32(obj, 0x60);
            s32 sx = sin_lookup((u16)(M32(obj, 0x84) + t)) * M32(obj, 0x60);
            f = fixmul_16_16(t, M32(obj, 0x68));
            s32 cy = cos_lookup((u16)(M16(obj, 0x86) + f)) * M32(obj, 0x64);
            s32 sy = sin_lookup((u16)(M16(obj, 0x86) + f)) * M32(obj, 0x64);

            x = (((s16)((u32)(cx + cy) >> 16)) - 8) * 2;
            y =  ((s16)((u32)(sy + sx) >> 16)) + 8;
            M32(obj, 0x8) = x;
            M32(obj, 0xC) = y;
            draw_point_sprite(obj);
        }
        break;
    }
}
