/* camera_track_two_slots_task [0x0220c3e0]: Tracks the two selected actor slots and builds their framing pose. */
#include "dh_work.h"
#include "dh.h"

/* func_0220c3e0 (0x0220c3e0) - two-target camera tracker.
 * task handler: task = r4; cam = *(u32 *)(task + 8).
 * Focus slots cam->unkAE / cam->unkB0 (& 7 each, re-stored). */

extern u8  g_actor_slot_registry[];   /* object slot table */
extern s32 g_06096C58[];   /* per-slot focus Y bias */
extern s32 g_actor_slot_masks_legacy[];   /* per-slot enable-mask table */
extern s32 g_0609977C;     /* active-slot mask bits */

extern void camera_compute_lookat_angles(s32, s32, s32, s32, s32, s32, s16 *, s16 *);
extern void camera_ease_to_target(void *);
extern void camera_tick_shake(void *);
extern void mtx_build_fwd_inv_entry(void *);

void camera_track_two_slots_task(void *task)
{
    u32 cam = (u32)TASK_PTR(task, work);
    s32 f0[3], f1[3];
    s32 zoom = 0x10000;
    s32 idx0, idx1, i, t, flags;

    idx0 = M16(cam, 0xAE) & 7;
    M16(cam, 0xAE) = (s16)idx0;

    if (M32((u32)g_actor_slot_registry, 0xC + idx0 * 4) != 0) {
        u32 coord = (u32)MP(MP(g_actor_slot_registry, 0x2C + idx0 * 4), 8);
        f0[0] = M32(coord, 0x8);
        f0[1] = M32(coord, 0xC);
        f0[2] = M32(coord, 0x10);
        f0[1] += g_06096C58[idx0];
        if (f0[1] > 0x24000)
            f0[1] = 0x24000;
        else if (f0[1] < 0xC000)
            f0[1] = 0xC000;
        flags = M32(coord, 0x38);
        if (flags & 0x1E0)
            zoom = 0x13000;
        else if (flags & 0xFE00)
            zoom = 0x16000;
    } else {
        f0[0] = CAMERA_M32(cam, stage_origin_x);
        f0[1] = CAMERA_M32(cam, stage_origin_y);
        f0[2] = CAMERA_M32(cam, stage_origin_z);
        f0[1] = 0x10000;
    }

    idx1 = M16(cam, 0xB0) & 7;
    M16(cam, 0xB0) = (s16)idx1;

    if (M32((u32)g_actor_slot_registry, 0xC + idx1 * 4) != 0) {
        u32 coord = (u32)MP(MP(g_actor_slot_registry, 0x2C + idx1 * 4), 8);
        f1[0] = M32(coord, 0x8);
        f1[1] = M32(coord, 0xC);
        f1[2] = M32(coord, 0x10);
        f1[1] += g_06096C58[idx0];   /* indexes idx0 (focus0 slot), original quirk */
        if (f1[1] > 0x24000)
            f1[1] = 0x24000;
        else if (f1[1] < 0xC000)
            f1[1] = 0xC000;
        flags = M32(coord, 0x38);
        if (flags & 0x1E0)
            zoom += 0x3000;
        else if (flags & 0xFE00)
            zoom += 0x6000;
    } else {
        f1[0] = CAMERA_M32(cam, stage_origin_x);
        f1[1] = CAMERA_M32(cam, stage_origin_y);
        f1[2] = CAMERA_M32(cam, stage_origin_z);
        f1[1] = 0x10000;
    }

    CAMERA_M32(cam, focus_x) = (f0[0] + f1[0]) >> 1;
    CAMERA_M32(cam, focus_y) = (f0[1] + f1[1]) >> 1;
    CAMERA_M32(cam, focus_z) = (f0[2] + f1[2]) >> 1;

    for (i = 0; i < 8; i++) {
        u32 coord;
        if (i == idx0 || i == idx1)
            continue;
        if (M32((u32)g_actor_slot_registry, 0xC + i * 4) == 0 &&
            !(g_actor_slot_masks_legacy[i - 1] & g_0609977C))
            continue;
        t = 0xA0000 - M32((u32)g_actor_slot_registry, 0x4C + idx0 * 0x20 + i * 4);
        if (t <= 0)
            continue;
        zoom += t >> 4;
        coord = (u32)MP(MP(g_actor_slot_registry, 0x2C + i * 4), 8);
        flags = M32(coord, 0x38);
        if (flags & 0x1E0)
            zoom += 0x3000;
        else if (flags & 0xFE00)
            zoom += 0x6000;
    }
    if (zoom > 0x20000)
        zoom = 0x20000;

    {
        s32 q = zoom >> 2;
        CAMERA_M32(cam, orbit_offset_x) = q * 3;
        CAMERA_M32(cam, orbit_offset_y) = 0x8000;
        CAMERA_M32(cam, orbit_offset_z) = -(zoom << 1) - q;
    }

    if (CAMERA_M32(cam, focus_x) < CAMERA_M32(cam, stage_origin_x))
        CAMERA_M32(cam, target_x) = CAMERA_M32(cam, focus_x) + CAMERA_M32(cam, orbit_offset_x);
    else
        CAMERA_M32(cam, target_x) = CAMERA_M32(cam, focus_x) - CAMERA_M32(cam, orbit_offset_x);

    if ((M16(cam, 0xAE) + M16(cam, 0xB0)) & 1)
        CAMERA_M32(cam, target_y) = CAMERA_M32(cam, focus_y) + CAMERA_M32(cam, orbit_offset_y);
    else
        CAMERA_M32(cam, target_y) = CAMERA_M32(cam, focus_y) - CAMERA_M32(cam, orbit_offset_y);

    CAMERA_M32(cam, target_z) = CAMERA_M32(cam, focus_z) + CAMERA_M32(cam, orbit_offset_z);

    camera_compute_lookat_angles(CAMERA_M32(cam, target_x), CAMERA_M32(cam, target_y), CAMERA_M32(cam, target_z),
                  CAMERA_M32(cam, focus_x), CAMERA_M32(cam, focus_y), CAMERA_M32(cam, focus_z),
                  (s16 *)(cam + 0xC4), (s16 *)(cam + 0xC6));

    CAMERA_M16(cam, target_angle_x) = ((M16(cam, 0xAE) + M16(cam, 0xB0)) & 1) ? (s16)-0x800
                                                             : (s16)0x800;
    if (zoom > 0x18000)
        CAMERA_M16(cam, target_angle_x) = (s16)(CAMERA_M16(cam, target_angle_x) >> 1);
    CAMERA_M16(cam, target_angle_z) = 0;

    camera_ease_to_target((void *)cam);

    {
        s32 d = CAMERA_M32(cam, stage_origin_x) - CAMERA_M32(cam, position_x);
        if (d > CAMERA_M32(cam, horizontal_range) + 0x8000)
            CAMERA_M32(cam, position_x) = CAMERA_M32(cam, stage_origin_x) - CAMERA_M32(cam, horizontal_range) - 0x8000;
        else if (d < -CAMERA_M32(cam, horizontal_range) - 0x8000)
            CAMERA_M32(cam, position_x) = CAMERA_M32(cam, stage_origin_x) + CAMERA_M32(cam, horizontal_range) + 0x8000;
    }

    camera_tick_shake(task);
    mtx_build_fwd_inv_entry((void *)cam);
}
