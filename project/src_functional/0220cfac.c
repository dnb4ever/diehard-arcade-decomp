#include "dh_work.h"
/* camera_update_player_follow [0x0220cfac]: Follows active player with proximity zoom, orbital offset and eased position. */
#include "dh.h"
/* 0x0220cfac (task handler, camera follow / zoom) - task->unk8 = the camera entity. Tracks the
 * active player slot entity->unkB0: copies that player's world pos (obj->unk8/C/10) into the
 * camera's target base (unk90/94/98) with a Y bias from g_06096C58[slot]. Then sums a zoom
 * accumulator (start 0x10000, clamp 0x20000) over the other 7 slots that are active (or whose
 * type mask g_0604EBE4[i-1] is in the active-type set g_0609977C): each contributing
 * (0xA0000 - dist)>>4 when the player is near, plus 0x3000 / 0x6000 depending on that player's
 * state flags (obj->unk8->unk38 & 0x1E0 / & 0xFE00). Finally orbits the camera: unk9C = cos(unk74
 * += 0x170), unkA0 = (sin(unk78 += 0x250)>>2)+0x4000, unkA4 = (zoom>>2)-(zoom*2), and eases the
 * live camera pos (unk8>>3, unkC/unk10>>4) toward base+orbit; sets fixed roll unk24 = -0x400,
 * yaw unk26 = unk9C>>5, unk28 = 0.
 * fidelity: HIGH - unkB0 word slot index, flag/pos/accumulator fields long, state flags word;
 * all shifts arithmetic (signed); dist table g_060995D0 has 0x20-byte rows (unkB0<<5); sin/cos
 * are the named math leaves; computed calls resolved via braf_rewrite.py. */
extern s32   sin_lookup(s32 a);   /* sin_lookup */
extern s32   cos_lookup(s32 a);   /* cos_lookup */
extern s32   g_actor_slot_ids[];           /* per-slot active flags */
extern void *g_actor_slot_tasks[];           /* per-slot object pointers */
extern s32   g_06096C58[];
extern s32   g_actor_slot_masks_legacy[];           /* per-type masks */
extern s32   g_0609977C;             /* active-type bitfield */
extern u8    g_060995D0[];           /* per-slot distance table, 0x20-byte rows */

void camera_update_player_follow(void *task) {
    void *entity = TASK_PTR(task, work);
    s32 slot = M16(entity, 0xB0);
    s32 zoom = 0x10000;
    s32 i;
    s32 a;

    if (g_actor_slot_ids[slot] != 0) {
        void *obj = TASK_PTR(g_actor_slot_tasks[slot], work);
        CAMERA_M32(entity, focus_x) = M32(obj, 8);
        CAMERA_M32(entity, focus_y) = M32(obj, 0xC);
        CAMERA_M32(entity, focus_z) = M32(obj, 0x10);
        CAMERA_M32(entity, focus_y) += g_06096C58[slot];
    }
    for (i = 0; i < 8; i++) {
        s32 d;
        if (i == slot)
            continue;
        if (g_actor_slot_ids[i] == 0 && !(g_actor_slot_masks_legacy[i - 1] & g_0609977C))
            continue;
        d = -M32(&g_060995D0[slot << 5], i * 4) + 0xA0000;
        if (d > 0) {
            s32 flags = M32(TASK_PTR(g_actor_slot_tasks[i], work), 0x38);
            zoom += d >> 4;
            if (flags & 0x1E0)
                zoom += 0x3000;
            else if (flags & 0xFE00)
                zoom += 0x6000;
        }
    }
    if (zoom > 0x20000)
        zoom = 0x20000;

    a = M32(entity, 0x74) + 0x170;
    M32(entity, 0x74) = a;
    CAMERA_M32(entity, orbit_offset_x) = cos_lookup(a);
    a = M32(entity, 0x78) + 0x250;
    M32(entity, 0x78) = a;
    CAMERA_M32(entity, orbit_offset_y) = (sin_lookup(a) >> 2) + 0x4000;
    CAMERA_M32(entity, orbit_offset_z) = (zoom >> 2) - (zoom * 2);
    CAMERA_M32(entity, position_x)  += ((CAMERA_M32(entity, focus_x) + CAMERA_M32(entity, orbit_offset_x)) - CAMERA_M32(entity, position_x))  >> 3;
    CAMERA_M32(entity, position_y)  += ((CAMERA_M32(entity, focus_y) + CAMERA_M32(entity, orbit_offset_y)) - CAMERA_M32(entity, position_y))  >> 4;
    CAMERA_M32(entity, position_z) += ((CAMERA_M32(entity, focus_z) + CAMERA_M32(entity, orbit_offset_z)) - CAMERA_M32(entity, position_z)) >> 4;
    CAMERA_M16(entity, angle_x) = -0x400;
    CAMERA_M16(entity, angle_y) = (s16)(CAMERA_M32(entity, orbit_offset_x) >> 5);
    CAMERA_M16(entity, angle_z) = 0;
}
