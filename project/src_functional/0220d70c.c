/* camera_follow_slot_task [0x0220d70c]: Latches the tracked actor slot and updates camera anchor and world pose. */
#include "dh_work.h"
#include "dh.h"
/* 0x0220d70c (object-following camera task handler) - task->unk8 = entity (camera). On the
 * frame it reaches its final zone slot (unkA8 == g_06052098 - 1) it latches onto a tracked
 * handle (slot unkAE&7 of the object array g_060995B0): the anchor X is that handle's
 * entity->unk8, or the camera's own home unk84 when the slot is empty. It copies two zone
 * record vectors (g_06060820 and g_0606082c, stride 0x2C via g_060b3d54) into unk90..unkA4,
 * seeds the eye (unk8=anchorX, unkC=0x18000, unk10=unk98-0x30000), the view angles
 * (unk24=-0x800, unk26=+/-0x1000 by anchor-vs-home) and clears unk28/unk70. Every frame it
 * then snaps unk8 to the anchor X clamped into [unk90+0x18000, unk9C-0x18000] and bobs the
 * height: unk78 += 0x100; unkC = 0x18000 - (sin_lookup(unk78) >> 1).
 * fidelity: HIGH - coords 32-bit (16.16), unkA8/AE/24/26/28 16-bit; handle->entity->pos is a
 * double indirection; dead scratch copies of Y/Z dropped; computed call resolved to sin_lookup
 * (sin_lookup). Verified vs asm. */
extern s8  g_actor_slot_registry;
extern s16 g_06052098;
extern s32 g_060b3d54;
extern s8  g_06060820;
extern s8  g_0606082c;
extern s32 sin_lookup(s32 angle);

void camera_follow_slot_task(void *task) {
    void *ent = TASK_PTR(task, work);
    void *handle;
    s32 ax;

    if (M16(ent, 0xA8) == g_06052098 - 1) {
        void *rec1, *rec2;

        M16(ent, 0xAE) = (s16)(M16(ent, 0xAE) & 7);
        handle = MP(&g_actor_slot_registry, 0x2C + M16(ent, 0xAE) * 4);
        ax = handle ? M32(MP(handle, 8), 8) : M32(ent, 0x84);

        rec1 = &g_06060820 + g_060b3d54 * 0x2C;
        M32(ent, 0x90) = M32(rec1, 0);
        M32(ent, 0x94) = M32(rec1, 4);
        M32(ent, 0x98) = M32(rec1, 8);
        rec2 = &g_0606082c + g_060b3d54 * 0x2C;
        M32(ent, 0x9C) = M32(rec2, 0);
        M32(ent, 0xA0) = M32(rec2, 4);
        M32(ent, 0xA4) = M32(rec2, 8);

        M32(ent, 0x8)  = ax;
        M32(ent, 0xC)  = 0x18000;
        M32(ent, 0x10) = M32(ent, 0x98) + (s32)0xFFFD0000;
        M16(ent, 0x24) = -0x800;
        M16(ent, 0x26) = (ax < M32(ent, 0x84)) ? -0x1000 : 0x1000;
        M16(ent, 0x28) = 0;
        M32(ent, 0x70) = 0;
    }

    handle = MP(&g_actor_slot_registry, 0x2C + M16(ent, 0xAE) * 4);
    ax = handle ? M32(MP(handle, 8), 8) : M32(ent, 0x84);
    M32(ent, 0x8) = ax;
    if (M32(ent, 0x8) < M32(ent, 0x90) + 0x18000)
        M32(ent, 0x8) = M32(ent, 0x90) + 0x18000;
    else if (M32(ent, 0x8) > M32(ent, 0x9C) + (s32)0xFFFE8000)
        M32(ent, 0x8) = M32(ent, 0x9C) + (s32)0xFFFE8000;

    M32(ent, 0x78) += 0x100;
    M32(ent, 0xC) = 0x18000 - (sin_lookup(M32(ent, 0x78)) >> 1);
}
