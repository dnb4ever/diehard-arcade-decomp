/* camera_patrol_zone_task [0x0220d310]: Seeds stage bounds and a random side before moving back and forth through the zone. */
#include "dh_work.h"
#include "dh.h"
/* 0x0220d310 (patrol / ping-pong mover task handler) - task->unk8 = entity. On the frame
 * the entity reaches its final zone slot (unkA8 == g_06052098 - 1) it latches the current
 * zone record (g_0606080c + g_060b3d54*0x2C) bounds into unk90..unkA4 and, picking a side
 * at random (rng_next & 1), places itself at one bound and sets a per-frame velocity unk70
 * of +0xE00 or -0xE00 (fixed Y=0xC000, Z=unk98-0x30000, zero angles). Every frame it then
 * advances unk8 by unk70 and, when it passes either bound (unk90+0x18000 low / unk9C-0x18000
 * high), reverses unk70 and re-applies it (bounce).
 * fidelity: HIGH - coords 32-bit (16.16), unkA8 16-bit; zone record stride 0x2C via the MAC
 * index g_060b3d54; computed call resolved to rng_next (func_02210020). Verified vs asm. */
extern s16 g_06052098;
extern s32 g_060b3d54;
extern s8  g_0606080c;
extern u32 game_random_next(void);

void camera_patrol_zone_task(void *task) {
    void *ent = TASK_PTR(task, work);

    if (M16(ent, 0xA8) == g_06052098 - 1) {
        void *rec = &g_0606080c + g_060b3d54 * 0x2C;

        M32(ent, 0x90) = M32(rec, 0x14);
        M32(ent, 0x94) = M32(rec, 0x18);
        M32(ent, 0x98) = M32(rec, 0x1C);
        M32(ent, 0x9C) = M32(rec, 0x20);
        M32(ent, 0xA0) = M32(rec, 0x24);
        M32(ent, 0xA4) = M32(rec, 0x28);

        if (game_random_next() & 1) {
            M32(ent, 0x8)  = M32(ent, 0x90) + 0x18000;
            M32(ent, 0xC)  = 0xC000;
            M32(ent, 0x10) = M32(ent, 0x98) + (s32)0xFFFD0000;
            M16(ent, 0x24) = 0;
            M16(ent, 0x26) = 0;
            M16(ent, 0x28) = 0;
            M32(ent, 0x70) = 0xE00;
        } else {
            M32(ent, 0x8)  = M32(ent, 0x9C) + (s32)0xFFFE8000;
            M32(ent, 0xC)  = 0xC000;
            M32(ent, 0x10) = M32(ent, 0x98) + (s32)0xFFFD0000;
            M16(ent, 0x24) = 0;
            M16(ent, 0x26) = 0;
            M16(ent, 0x28) = 0;
            M32(ent, 0x70) = -0xE00;
        }
    }

    M32(ent, 0x8) += M32(ent, 0x70);
    if (M32(ent, 0x8) < M32(ent, 0x90) + 0x18000 ||
        M32(ent, 0x8) > M32(ent, 0x9C) + (s32)0xFFFE8000) {
        M32(ent, 0x70) = -M32(ent, 0x70);
        M32(ent, 0x8) += M32(ent, 0x70);
    }
}
