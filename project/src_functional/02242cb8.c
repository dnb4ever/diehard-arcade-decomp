/* actor_dispatch_motion_mode [0x02242cb8]: Task+13 selects ten modes; called three times by 02242a10. */
#include "dh.h"

/* 0x02242cb8 - func_02242cb8(ctx, obj): per-object motion/mode dispatcher.
 * Local helper called 3x by func_02242a10 (02242a10.c externs it).
 *
 * A single inline braf jump table: switch(*(s8)(ctx + 0x13)) 0..9
 * (unsigned-compared against 10 -> any other value falls straight to the
 * epilogue). Table @0x242d00, braf @0x242cfa, target = 0x242cfe + table[idx]:
 *   0 0x242d14  1 0x242d2a  2 0x242d42  3 0x242d6a  4 0x242dde
 *   5 0x242e02  6 0x242e94  7 0x242eee  8 0x242f30  9 0x242f3e   default 0x242f4e
 *
 * Decoded from clean disasm (fn.py) + pool literals. Register map at entry:
 *   r11=ctx  r13=obj  r10=obj+8  r14=obj+0x14  r12=obj+0x18
 *   r6=M32(obj,0x14) cached once (d14)  r8=2  r4=-3(mask ~2)  r5=7  r9=0
 *   r7=-0x400
 * Each case mutates obj's position (+8), velocity (+0x14), accel (+0x18),
 * counter (+0x74) and flag word (+0x80), and advances the substate ctx+0x13.
 *
 * CORRECTION to notes/functional_band240giants.md: this function is NOT
 * "self-contained / no subroutine calls" - case 3 does `bsr 0x242f60`
 * (mode_resolve_0x242f60 = mode_resolve_0x242f60, returns an index into g_060995b0;
 * takes no args, returns s32). It is the function's only call.
 */

#define OBJ_ARR DH_ACTOR_SLOT_TASKS_ADDR_U    /* void*[] per-slot source objects */

s32 mode_resolve_0x242f60(void);         /* extern: mode_resolve_0x242f60 (no args) */

void actor_dispatch_motion_mode(void *ctx, void *obj)
{
    s32 d14 = M32(obj, 0x14);          /* r6: cached @entry */
    s32 idx = M8(ctx, 0x13);           /* mov.b, unsigned-compared vs 10 */

    switch (idx) {
    case 0:
        M32(obj, 0x14)  = 0x4000;
        M32(obj, 0x18)  = -0x400;
        M32(obj, 0x80) |= 2;
        M8(ctx, 0x13)   = 1;
        break;

    case 1:
        M32(obj, 8) += d14;
        if (M32(obj, 8) >= 0x2f8000)
            M8(ctx, 0x13) = 2;
        break;

    case 2: {
        s32 v = M32(obj, 0x14) + M32(obj, 0x18);
        M32(obj, 0x14) = v;
        if (v > 0) {
            M32(obj, 8) += v;
        } else {
            M32(obj, 0x74)  = 90;
            M32(obj, 0x80) &= ~2;
            M8(ctx, 0x13)   = 3;
        }
        break;
    }

    case 3: {
        s32 t = M32(obj, 0x74);
        M32(obj, 0x74) = t - 1;
        if (t >= 0)
            break;
        {
            s32 i = mode_resolve_0x242f60();
            void *e, *p;
            s32 dir;
            if (i == 0)
                break;
            e = MP((void *)(uintptr_t)OBJ_ARR, i * 4);
            p = MP(e, 8);
            M32(obj, 0x14) = 0;
            dir = (M32(p, 8) > M32(obj, 8)) ? 0x200 : -0x200;
            M32(obj, 0x18) = dir;
            if (dir > 0) {
                if (M32(obj, 8) >= 0x310000)
                    break;
            } else {
                if (!(M32(obj, 8) > 0x2ca000))
                    break;
            }
            M32(obj, 0x74)  = M32(p, 8);
            M32(obj, 0x80) |= 2;
            M8(ctx, 0x13)   = 5;
        }
        break;
    }

    case 4:
        M32(obj, 0x14) = 0;
        M32(obj, 0x18) = (M32(obj, 8) >= 0x2ed000) ? -0x200 : 0x200;
        M32(obj, 0x80) |= 2;
        M8(ctx, 0x13)   = 6;
        break;

    case 5: {
        s32 flag, dir;
        M32(obj, 8)    += d14;
        M32(obj, 0x14) += M32(obj, 0x18);
        flag = (M32(obj, 0x74) > M32(obj, 8)) ? 0 : 1;
        dir  = M32(obj, 0x18);
        if (dir > 0) {
            if (M32(obj, 0x14) > 0x2000)
                M32(obj, 0x14) = 0x2000;
            if (flag == 0 && M32(obj, 8) < 0x310000)
                break;
            M32(obj, 0x18) = -0x400;
        } else {
            if (M32(obj, 0x14) < -0x2000)
                M32(obj, 0x14) = -0x2000;
            if (flag != 0 && M32(obj, 8) > 0x2ca000)
                break;
            M32(obj, 0x18) = 0x400;
        }
        M32(obj, 0x80) &= ~2;
        M8(ctx, 0x13)   = 7;
        break;
    }

    case 6:
        M32(obj, 8)    += d14;
        M32(obj, 0x14) += M32(obj, 0x18);
        if (M32(obj, 0x18) > 0) {
            if (M32(obj, 0x14) > 0x1000)
                M32(obj, 0x14) = 0x1000;
            if (M32(obj, 8) < 0x324000)
                break;
            M32(obj, 0x18) = -0x400;
        } else {
            if (M32(obj, 0x14) < -0x1000)
                M32(obj, 0x14) = -0x1000;
            if (M32(obj, 8) > 0x2c0000)
                break;
            M32(obj, 0x18) = 0x400;
        }
        M32(obj, 0x80) &= ~2;
        M8(ctx, 0x13)   = 7;
        break;

    case 7: {
        s32 old, sum;
        M32(obj, 8) += d14;
        old = M32(obj, 0x14);
        sum = old + M32(obj, 0x18);
        M32(obj, 0x14) = sum;
        if ((old ^ sum) < 0) {          /* sign flip */
            M32(obj, 0x74) = 16;
            M8(ctx, 0x13)  = 3;
        }
        break;
    }

    case 8:
        M32(obj, 0x14) = 0;
        M32(obj, 0x18) = -0x200;
        M8(ctx, 0x13) += 1;             /* 8 -> 9, fall through */
        /* fallthrough */
    case 9:
        M32(obj, 8)    += M32(obj, 0x14);
        M32(obj, 0x14) += M32(obj, 0x18);
        break;

    default:
        break;
    }
}
