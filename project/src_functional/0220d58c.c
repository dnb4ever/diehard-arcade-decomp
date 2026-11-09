/* camera_pendulum_task [0x0220d58c]: Seeds stage bounds and advances a swinging camera pose. */
#include "dh.h"
/* 0x0220d58c - swing / pendulum motion handler for an entity (arg0->unk8).
 *
 * On the trigger frame (entity->unkA8 == g_06052098 - 1) it re-seeds from a
 * 0x2C-byte descriptor desc = g_0606080c + g_060b3d54 * 0x2C, copying desc fields
 * unk14/18/1C -> entity unk90/94/98 and desc unk20/24/28 -> entity unk9C/A0/A4.
 * It then picks a swing direction from func_02210020() & 1 (stored in unk70):
 *   dir != 0: unk8 = unk90 + 0x18000; unk10 = unk98 - 0x18000; unk24 = -0x1000;
 *             unk26 = -0x400; unk74 = -0x40;
 *   dir == 0: unk8 = unk9C - 0x18000; unk10 = unk98 - 0x18000; unk24 = -0x1000;
 *             unk26 =  0x400; unk74 =  0x40;
 * both set unkC = 0x28000, unk28 = 0, unk78 = 0.
 *
 * Every frame (unconditional): swing angle unk26 += unk76; if unk26 leaves its
 * direction band (dir!=0: [-0x1E00,-0x400]; dir==0: [0x400,0x1E00]) reverse the
 * step (unk74 = -unk74) and re-apply unk26 += unk76. Advance phase unk78 += 0x200
 * and set the vertical position unkC = 0x20000 - (sin(unk78) >> 2) via
 * sin_lookup (sin_lookup).
 *
 * fidelity: HIGH - hand-verified vs m2c_fn --asm + braf_resolve (fn.py 0x20d58c).
 * Calls: func_02210020 (no args), sin_lookup (sin_lookup). The bsrf at 0x20d630
 * is a PHANTOM (embedded literal pool 0x20d620-0x20d65a: 0x0003==bsrf r0 pattern,
 * plus the g_060b3d54 / g_0606080c longs) - ignored. unk90.. are longs, the swing
 * angle fields unk24/26/28/76 are halfwords, unk70/74/78 longs. */
#define G16(a)  (*(s16 *)(uintptr_t)(a))
#define G32(a)  (*(s32 *)(uintptr_t)(a))

extern s32 game_random_next(void);                    /* 0x0210020 (no args) */
extern s32 sin_lookup(s32 angle);               /* 0x0213190 sin_lookup */

void camera_pendulum_task(void *arg0) {
    void *e = MP(arg0, 8);
    s32 dir;

    if (M16(e, 0xA8) == (s16)(G16(0x06052098) - 1)) {
        void *desc = (void *)(uintptr_t)(0x0606080c + G32(0x060b3d54) * 0x2C);

        M32(e, 0x90) = M32(desc, 0x14);
        M32(e, 0x94) = M32(desc, 0x18);
        M32(e, 0x98) = M32(desc, 0x1C);
        M32(e, 0x9C) = M32(desc, 0x20);
        M32(e, 0xA0) = M32(desc, 0x24);
        M32(e, 0xA4) = M32(desc, 0x28);

        dir = game_random_next() & 1;
        M32(e, 0x70) = dir;

        if (dir != 0) {
            M32(e, 0x08) = M32(e, 0x90) + 0x18000;
            M32(e, 0x0C) = 0x28000;
            M32(e, 0x10) = M32(e, 0x98) + 0xFFFE8000;
            M16(e, 0x24) = -0x1000;
            M16(e, 0x26) = -0x400;
            M16(e, 0x28) = 0;
            M32(e, 0x74) = -0x40;
        } else {
            M32(e, 0x08) = M32(e, 0x9C) + 0xFFFE8000;
            M32(e, 0x0C) = 0x28000;
            M32(e, 0x10) = M32(e, 0x98) + 0xFFFE8000;
            M16(e, 0x24) = -0x1000;
            M16(e, 0x26) = 0x400;
            M16(e, 0x28) = 0;
            M32(e, 0x74) = 0x40;
        }
        M32(e, 0x78) = 0;
    }

    M16(e, 0x26) = (s16)(M16(e, 0x26) + M16(e, 0x76));
    dir = M32(e, 0x70);
    if ((dir != 0 && (M16(e, 0x26) < -0x1E00 || M16(e, 0x26) > -0x400)) ||
        (dir == 0 && (M16(e, 0x26) <  0x400  || M16(e, 0x26) >  0x1E00))) {
        M32(e, 0x74) = -M32(e, 0x74);
        M16(e, 0x26) = (s16)(M16(e, 0x26) + M16(e, 0x76));
    }

    M32(e, 0x78) += 0x200;
    M32(e, 0x0C) = 0x20000 - (sin_lookup(M32(e, 0x78)) >> 2);
}
