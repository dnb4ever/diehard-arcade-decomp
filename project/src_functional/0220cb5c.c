/* camera_acquire_leap_target [0x0220cb5c]: Tests the player against stage gates and acquires the leap tracking target. */
#include "dh_work.h"
#include "dh.h"
/* 0x0220cb5c (camera target-acquisition test, leap variant; returns 1 acquire / 0 reject) -
 * close twin of func_0220cca0. Picks player slot idx=(unkAE-1)&1, requires its active-mask
 * bit (g_0604ebe4[idx] & g_0609977c), reads the player record pr=g_06099764+idx*0xC and
 * zone record rec=g_0606080c+g_060b3d54*0x2C, and rejects if the player is out of bounds
 * (z <= rec.unk28-0x18000; x within rec.unk14+0x18000 .. rec.unk20-0x18000). It builds a
 * candidate box around the player (unk90=px-0x20000, unk98=pz-0x30000, unk9C=px+0x20000,
 * unkA4=pz+0x20000) and runs func_0220e000(ent+0x90, ent+0x9C, &vis, &dummy); if vis>1
 * (crowded) it rejects. On success: unkAE=idx+1, unk90/94/98=player pos (+0x4000 Y),
 * and a leap setup keyed on unkB2 bit0 (unk9C=0xFFFF4000/0xC000, unk74=+/-0x1D4), plus
 * unkA0=0x8000, unkA4=0xFFFE4000, unk70=0xD2; returns 1.
 * fidelity: HIGH - coords 32-bit (16.16), unkAE/B2 16-bit; record strides 0xC/0x2C (MAC
 * index g_060b3d54); computed call resolved to func_0220e000 (writes via out ptr). Verified vs asm. */
extern s32 g_actor_slot_masks;
extern s32 g_0609977c;
extern s8  g_06099764;
extern s32 g_060b3d54;
extern s8  g_0606080c;
extern s32 camera_count_slots_in_rectangle(void *a, void *b, s32 *out0, s32 *out4);

s32 camera_acquire_leap_target(void *task) {
    void *ent = TASK_PTR(task, work);
    s32 idx = (M16(ent, 0xAE) - 1) & 1;
    void *pr, *rec;
    s32 px, pz;
    s32 vis, dummy;

    if (((&g_actor_slot_masks)[idx] & g_0609977c) == 0)
        return 0;

    pr = &g_06099764 + (s8)(idx * 0xC);
    rec = &g_0606080c + g_060b3d54 * 0x2C;

    pz = M32(pr, 8);
    if (pz > M32(rec, 0x28) + (s32)0xFFFE8000)
        return 0;
    px = M32(pr, 0);
    if (px < M32(rec, 0x14) + 0x18000 || px > M32(rec, 0x20) + (s32)0xFFFE8000)
        return 0;

    M32(ent, 0x90) = px + (s32)0xFFFE0000;
    M32(ent, 0x98) = M32(pr, 8) + (s32)0xFFFD0000;
    M32(ent, 0x9C) = M32(pr, 0) + 0x20000;
    M32(ent, 0xA4) = M32(pr, 8) + 0x20000;

    camera_count_slots_in_rectangle((char *)ent + 0x90, (char *)ent + 0x9C, &vis, &dummy);
    if (vis > 1)
        return 0;

    M16(ent, 0xAE) = (s16)(idx + 1);
    M32(ent, 0x90) = M32(pr, 0);
    M32(ent, 0x94) = M32(pr, 4) + 0x4000;
    M32(ent, 0x98) = M32(pr, 8);
    if (M16(ent, 0xB2) & 1) {
        M32(ent, 0x9C) = (s32)0xFFFF4000;
        M32(ent, 0x74) = 0x1D4;
    } else {
        M32(ent, 0x9C) = 0xC000;
        M32(ent, 0x74) = -0x1D4;
    }
    M32(ent, 0xA0) = 0x8000;
    M32(ent, 0xA4) = (s32)0xFFFE4000;
    M32(ent, 0x70) = 0xD2;
    return 1;
}
