#include "dh_work.h"
/* camera_update_slot_orbit [0x0220cdf6]: Retains the camera focus and computes proximity/state-driven zoom and orbital pose. */
#include "dh.h"
/* 0x0220cdf6 (camera orbit / zoom update, unkAE-slot variant) - task->unk8 = camera entity.
 * Reads slot = M16(entity,0xAE), retains the current focus, and sums a zoom accumulator
 * (start 0x10000, clamp 0x20000) over the 8 slots: skip slot==slot, include slots with
 * g_06099590[i] != 0 or (g_0604EBE4[i-1] & g_0609977C); each contributes
 * (0xA0000 - dist)>>4 when positive, dist from g_060995D0[slot<<5] row, plus 0x3000 /
 * 0x6000 from the player state flags (g_060995B0[i]->unk8->unk38 & 0x1E0 / & 0xFE00).
 * Then orbits: angle0 = M32(entity,0x74) + 0x100; c = sin_lookup(angle0);
 * b = M32(entity,0x70) + (c >> 3); s = sin_lookup(b);
 * M32(entity,0x9C) = func_022131c8(zoom + (zoom >> 1), s);
 * M32(entity,0xA0) = abs(b << 1); c2 = cos_lookup(b);
 * M32(entity,0xA4) = -func_022131c8(zoom, c2) + 0xFFFD8000;
 * M32(entity,0x8/C/10) = base unk90/94/98 + unk9C/A0/A4; unk24 = -0x400,
 * unk26 = (s16)(b >> 1), unk28 = 0.
 * fidelity: HIGH - unkAE word slot index, dist table rows 0x20 bytes, all shifts
 * arithmetic; call order/store-before-call (delay slot stores unk74 before first
 * sin/cos leaf) preserved; computed calls resolved to sin_lookup/cos_lookup/
 * func_022131c8 (fixed-point helpers). Verified vs asm. */
extern s32   sin_lookup(s32 a);   /* sin_lookup */
extern s32   cos_lookup(s32 a);   /* cos_lookup */
extern s32   fixed_mul16(s32 a, s32 b);
extern s32   g_actor_slot_ids[];        /* per-slot active flags */
extern void *g_actor_slot_tasks[];        /* per-slot object pointers */
extern s32   g_06096C58[];
extern s32   g_actor_slot_masks_legacy[];        /* per-type masks */
extern s32   g_0609977C;          /* active-type bitfield */
extern u8    g_060995D0[];        /* per-slot distance table, 0x20-byte rows */

void camera_update_slot_orbit(void *task) {
    void *entity = TASK_PTR(task, work);
    s32 slot = (s16)M16(entity, 0xAE);
    s32 zoom = 0x10000;
    s32 i, angle0, c, b, s, r, c2;

    /* ROM 06010e14 branches directly into the slot loop; focus is retained. */
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

    angle0 = M32(entity, 0x74) + 0x100;
    M32(entity, 0x74) = angle0;
    c = sin_lookup(angle0); /* 06010ee8 + pool 0x62a4 = 06017190 */
    b = M32(entity, 0x70) + (c >> 3);
    s = sin_lookup(b);
    CAMERA_M32(entity, orbit_offset_x) = fixed_mul16(zoom + (zoom >> 1), s);

    r = b << 1;
    CAMERA_M32(entity, orbit_offset_y) = (r >= 0) ? r : -r;

    c2 = cos_lookup(b);
    r = fixed_mul16(zoom, c2);
    CAMERA_M32(entity, orbit_offset_z) = -r + (s32)0xFFFD8000;

    WORLD_POSE(entity)->position_x  = CAMERA_M32(entity, focus_x) + CAMERA_M32(entity, orbit_offset_x);
    WORLD_POSE(entity)->position_y  = CAMERA_M32(entity, focus_y) + CAMERA_M32(entity, orbit_offset_y);
    WORLD_POSE(entity)->position_z = CAMERA_M32(entity, focus_z) + CAMERA_M32(entity, orbit_offset_z);
    WORLD_POSE(entity)->angle_x = (s16)0xFC00;
    WORLD_POSE(entity)->angle_y = (s16)(b >> 1);
    WORLD_POSE(entity)->angle_z = 0;
}
