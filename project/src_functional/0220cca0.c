/* camera_acquire_target [0x0220cca0]: Tests the player record and stage bounds before acquiring the camera target. */
#include "dh_work.h"
#include "dh.h"
/* 0x0220cca0 (camera target-acquisition test, returns 1 on acquire / 0 on reject) -
 * task->unk8 = entity (camera). Picks player slot idx = (unkAE-1)&1 and bails unless that
 * player's active-mask bit is set (g_0604ebe4[idx] & g_0609977c). Reads that player's
 * record pr = g_06099764 + idx*0xC (x=unk0, y=unk4, z=unk8) and the current zone record
 * rec = g_0606080c + g_060b3d54*0x2C, rejecting the target if the player is outside the
 * zone bounds (z within rec.unk1C+0x28000 .. rec.unk28-0x14000; x within rec.unk14+0x28000
 * .. rec.unk20-0x28000). It then sets up candidate framing fields (unk90/98/9C/A4) and runs
 * the visibility probe func_0220e000(ent+0x9C, ent+0x9C, &vis, &dummy); if vis>1 (occluded)
 * it rejects. On success it commits: unkAE=idx+1, unk90/94/98 = player pos (+0x4000 Y bias),
 * horizontal speed unk70 = +/-0x140 by unkB2 bit0, unk74=0; returns 1.
 * fidelity: HIGH - coords 32-bit (16.16), unkAE/B2 16-bit; record strides 0xC / 0x2C (MAC
 * index g_060b3d54); computed call resolved to func_0220e000 (in-band, writes its result
 * through the out pointer). Verified vs asm. */
extern s32 g_actor_slot_masks;
extern s32 g_0609977c;
extern s8  g_06099764;
extern s32 g_060b3d54;
extern s8  g_0606080c;
extern s32 camera_count_slots_in_rectangle(void *a, void *b, s32 *out0, s32 *out4);

s32 camera_acquire_target(void *task) {
    void *ent = TASK_PTR(task, work);
    s32 idx = (M16(ent, 0xAE) - 1) & 1;
    void *pr;
    void *rec;
    s32 px, pz;
    s32 vis, dummy;

    if (((&g_actor_slot_masks)[idx] & g_0609977c) == 0)
        return 0;

    pr = &g_06099764 + (s8)(idx * 0xC);
    rec = &g_0606080c + g_060b3d54 * 0x2C;

    pz = M32(pr, 8);
    if (pz > M32(rec, 0x28) + (s32)0xFFFEC000)
        return 0;
    if (pz < M32(rec, 0x1C) + 0x28000)
        return 0;
    px = M32(pr, 0);
    if (px < M32(rec, 0x14) + 0x28000 || px > M32(rec, 0x20) + (s32)0xFFFD8000)
        return 0;

    M32(ent, 0x90) = px + (s32)0xFFFD0000;
    M32(ent, 0x98) = M32(pr, 8) + (s32)0xFFFE0000;
    M32(ent, 0x9C) = M32(pr, 0) + 0x30000;
    M32(ent, 0xA4) = M32(pr, 8) + 0x20000;

    camera_count_slots_in_rectangle((char *)ent + 0x9C, (char *)ent + 0x9C, &vis, &dummy);
    if (vis > 1)
        return 0;

    M16(ent, 0xAE) = (s16)(idx + 1);
    M32(ent, 0x90) = M32(pr, 0);
    M32(ent, 0x94) = M32(pr, 4) + 0x4000;
    M32(ent, 0x98) = M32(pr, 8);
    M32(ent, 0x70) = (M16(ent, 0xB2) & 1) ? 0x140 : -0x140;
    M32(ent, 0x74) = 0;
    return 1;
}
