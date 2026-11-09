/* camera_count_slots_in_rectangle [0x0220e000]: Counts active slot positions inside an open XZ rectangle and returns the highest matching slot. */
#include "dh.h"
/* 0x0220e000 (count players/objects inside an XZ rectangle, leaf) - *count is set to the
 * number of active slots (i=1..7) whose test point lies strictly inside the open rectangle
 * (pmin.x < px < pmax.x) && (pmin.z < pz < pmax.z); *last receives the highest matching
 * slot index. A slot is considered only when its flag (g_06099590[i]) is set or its
 * player-active mask bit (g_0604ebe4[i-1] & g_0609977c) is set. The test point is the
 * player record g_06099764 + (i-1)*0xC when that mask bit is set, otherwise the object's
 * entity position (g_060995B0[i])->unk8 + 8.
 * fidelity: HIGH - coords 32-bit (16.16); slot flag array at g_06099584+0xC, object-ptr
 * array at +0x2C; record stride 0xC. Leaf. Verified vs asm. */
extern s8  g_actor_slot_registry;
extern s8  g_06099764;
extern s32 g_actor_slot_masks;
extern s32 g_0609977c;

void camera_count_slots_in_rectangle(void *pmin, void *pmax, s32 *count, s32 *last) {
    s32 i;

    *count = 0;
    for (i = 1; i < 8; i++) {
        void *p;
        s32 px, pz;

        if (MP(&g_actor_slot_registry, 0xC + i * 4) == (void *)0 &&
            ((&g_actor_slot_masks)[i - 1] & g_0609977c) == 0)
            continue;

        if ((&g_actor_slot_masks)[i - 1] & g_0609977c)
            p = &g_06099764 + (s8)((i - 1) * 0xC);
        else
            p = (char *)MP(MP(&g_actor_slot_registry, 0x2C + i * 4), 8) + 8;

        px = M32(p, 0);
        pz = M32(p, 8);
        if (M32(pmin, 0) < px && M32(pmin, 8) < pz &&
            M32(pmax, 0) > px && M32(pmax, 8) > pz) {
            *count += 1;
            *last = i;
        }
    }
}
