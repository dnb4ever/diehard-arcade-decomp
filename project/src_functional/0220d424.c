/* camera_patrol_x_task [0x0220d424]: Refreshes actor bounds and runs a linear ping-pong move on X. */
#include "dh_work.h"
#include "dh.h"
/* 0x0220d424 - linear ping-pong mover (X axis) handler. task->unk8 = entity.
 * First calls view_bounds_from_actors(entity, sp+0xC, sp) to refresh local
 * min/max bounds (sp+0x14 = max.Y used later). On the trigger frame
 * (entity->unkA8 == g_06052098 - 1) it latches the zone descriptor
 * (g_0606080c + g_060b3d54 * 0x2C): unk14/18/1C -> entity 0x90/94/98 and
 * unk20/24/28 -> 0x9C/A0/A4, then picks a direction from rng_next() & 1:
 *   dir != 0: X = unk90 + 0x30000, vel unk70 =  0x600;
 *   dir == 0: X = unk9C - 0x30000, vel unk70 = -0x600;
 * both set Y unkC = 0x54000, angles unk24 = -0x2000, unk26 = unk28 = 0.
 * Every frame: X += unk70; if X passes either bound (unk90+0x30000 low,
 * unk9C-0x30000 high) reverse unk70 and re-apply. Z unk10 is then clamped:
 * unk10 = min(max_Y(sp+0x14), unkA4 - 0x20000) - 0x3C000.
 * fidelity: HIGH - coords 32-bit (16.16), unkA8 16-bit; zone stride 0x2C via
 * g_060b3d54; bsrf at 0x20d488 resolved to func_02210020 (rng_next, no args).
 * sp+0x14 holds the max bound from view_bounds_from_actors. */
extern void view_bounds_from_actors_raw_entry(void *self, void *minOut, void *maxOut);
extern u32 game_random_next(void);

void camera_patrol_x_task(void *task) {
    u8  sp[24];
    void *e = TASK_PTR(task, work);
    s32 vel;

    view_bounds_from_actors_raw_entry(e, sp + 12, sp);

    if (M16(e, 0xA8) == (s16)(MU16(0x06052098, 0) - 1)) {
        u8 *rec = (u8 *)(uintptr_t)(0x0606080c + MU32(0x060b3d54, 0) * 0x2C);

        M32(e, 0x90) = M32(rec, 0x14);
        M32(e, 0x94) = M32(rec, 0x18);
        M32(e, 0x98) = M32(rec, 0x1C);
        M32(e, 0x9C) = M32(rec, 0x20);
        M32(e, 0xA0) = M32(rec, 0x24);
        M32(e, 0xA4) = M32(rec, 0x28);

        if (game_random_next() & 1) {
            M32(e, 0x08) = M32(e, 0x90) + 0x30000;
            M32(e, 0x0C) = 0x54000;
            M32(e, 0x10) = M32(sp, 0x14) < M32(e, 0xA4) + (s32)0xFFFE0000
                         ? M32(sp, 0x14) + (s32)0xFFFC4000
                         : M32(e, 0xA4) + (s32)0xFFFE0000 + (s32)0xFFFC4000;
            M16(e, 0x24) = (s16)(s32)0xFFFFE000;
            M16(e, 0x26) = 0;
            M16(e, 0x28) = 0;
            vel = 0x600;
        } else {
            M32(e, 0x08) = M32(e, 0x9C) + (s32)0xFFFD0000;
            M32(e, 0x0C) = 0x54000;
            M32(e, 0x10) = M32(sp, 0x14) < M32(e, 0xA4) + (s32)0xFFFE0000
                         ? M32(sp, 0x14) + (s32)0xFFFC4000
                         : M32(e, 0xA4) + (s32)0xFFFE0000 + (s32)0xFFFC4000;
            M16(e, 0x24) = (s16)(s32)0xFFFFE000;
            M16(e, 0x26) = 0;
            M16(e, 0x28) = 0;
            vel = (s32)0xFFFFFA00;
        }
        M32(e, 0x70) = vel;
    }

    {
        s32 x = M32(e, 0x08) + M32(e, 0x70);

        M32(e, 0x08) = x;
        if (x < M32(e, 0x90) + 0x30000 ||
            x > M32(e, 0x9C) + (s32)0xFFFD0000) {
            M32(e, 0x70) = -M32(e, 0x70);
            M32(e, 0x08) = M32(e, 0x08) + M32(e, 0x70);
        }
    }

    M32(e, 0x10) = (M32(sp, 0x14) < M32(e, 0xA4) + (s32)0xFFFE0000
                    ? M32(sp, 0x14)
                    : M32(e, 0xA4) + (s32)0xFFFE0000) + (s32)0xFFFC4000;
}
