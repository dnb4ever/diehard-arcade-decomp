#include "dh_work.h"
#include "dh.h"
/* 0x0220c17c (camera_track_single) - per-frame camera update that frames a single focus
 * target (slot cam->unkAE) and pulls the eye back far enough to also keep nearby active
 * actors in shot. Task handler: arg0 = task; cam = task->unk8.
 *
 * The sibling 0x0220c3e4 (camera_track_pair, NOT this entry) frames two targets
 * (unkAE + unkB0); this one is the single-target variant and ends by tail-calling
 * mtx_build_fwd_inv, exactly as 0x20c3e4 does.
 *
 * Focus point (cam->unk90/94/98): the target actor's coord (objPtr[idx]->unk8 .unk8/C/10)
 * biased in Y by g_06096C58[idx] and clamped to [0x8000, 0x24000]; or the camera's own
 * home pos (cam->unk84/8C) with Y pinned at 0x10000 when the slot is empty.
 *
 * Zoom (var zoom, starts 0x10000): for every OTHER active slot i (i != idx, and either
 * flagged or enabled by g_0604EBE4[i-1] & *g_0609977C), add (0xA0000 - dist[idx][i])>>4
 * when positive, plus 0x3000 if the actor's state (coord->unk38) has bits 0x1E0 else
 * 0x6000 if bits 0xFE00; capped at 0x26000. Then:
 *   cam->unk9C = zoom>>1;  cam->unkA0 = cam->unkA8<<10;  cam->unkA4 = (zoom>>2)-(zoom*2);
 * and the eye (unkB8/BC/C0) = focus +/- those offsets (X offset sign follows unk90<unk84).
 *
 * dist table: g_06099584 + 0x4C, 8x8 longs (stride 0x20), [focusSlot][otherSlot].
 *
 * Calls (all resolved via braf_resolve, m2c dropped them as bsrf):
 *   lookat_angles_xz (0x21055c, 8 args, 4 on stack) -> cam->unkC4/C6 (pitch/yaw);
 *   camera_smooth_lerp (0x20e0aa, cam); camera_shake_decay (0x20d98e, task);
 *   mtx_build_fwd_inv (0x214a7a, cam) as a tail call.
 *
 * fidelity: HIGH - hand-traced against m2c_fn --asm (0x20c17c..0x20c3dc braf). The
 * 0xF800/0x8000 pool shorts are loaded with mov.w (sign-extended) = -0x800 / -0x8000. */

extern u8  g_actor_slot_registry[];   /* camera object table */
extern s32 g_06096C58[];   /* per-slot focus Y bias */
extern s32 g_actor_slot_masks_legacy[];   /* per-slot enable-mask table */
extern s32 g_0609977C;     /* active-slot mask bits */

extern void lookat_angles_xz(s32 x, s32 y, s32 z, s32 bx, s32 by, s32 bz,
                             s16 *outPitch, s16 *outYaw);   /* 0x21055c */
extern void camera_smooth_lerp(void *cam);                 /* 0x20e0aa */
extern void camera_shake_decay(void *task);                /* 0x20d98e */
extern void mtx_build_fwd_inv(void *cam);                  /* 0x214a7a */

void camera_track_single(void *task)
{
    void *cam = TASK_PTR(task, work);
    void *coord;
    s32 zoom = 0x10000;
    s32 idx, i, t, flags;

    idx = M16(cam, 0xAE) & 7;
    M16(cam, 0xAE) = (s16)idx;

    if (M32(g_actor_slot_registry, 0xC + idx * 4) != 0) {
        coord = MP(MP(g_actor_slot_registry, 0x2C + idx * 4), 8);
        CAMERA_M32(cam, focus_x) = WORLD_POSE(coord)->position_x;
        CAMERA_M32(cam, focus_y) = WORLD_POSE(coord)->position_y;
        CAMERA_M32(cam, focus_z) = WORLD_POSE(coord)->position_z;
        CAMERA_M32(cam, focus_y) += g_06096C58[idx];
        if (CAMERA_M32(cam, focus_y) > 0x24000)      CAMERA_M32(cam, focus_y) = 0x24000;
        else if (CAMERA_M32(cam, focus_y) < 0x8000)  CAMERA_M32(cam, focus_y) = 0x8000;
    } else {
        CAMERA_M16(cam, request_hold_ticks) = 1;
        CAMERA_M32(cam, focus_x) = CAMERA_M32(cam, stage_origin_x);
        CAMERA_M32(cam, focus_z) = CAMERA_M32(cam, stage_origin_z);
        CAMERA_M32(cam, focus_y) = 0x10000;
    }

    for (i = 0; i < 8; i++) {
        if (i == idx)
            continue;
        if (M32(g_actor_slot_registry, 0xC + i * 4) == 0 &&
            !(g_actor_slot_masks_legacy[i - 1] & g_0609977C))
            continue;
        t = 0xA0000 - M32(g_actor_slot_registry, 0x4C + idx * 0x20 + i * 4);
        if (t <= 0)
            continue;
        zoom += t >> 4;
        flags = M32(MP(MP(g_actor_slot_registry, 0x2C + i * 4), 8), 0x38);
        if (flags & 0x1E0)
            zoom += 0x3000;
        else if (flags & 0xFE00)
            zoom += 0x6000;
    }
    if (zoom > 0x26000)
        zoom = 0x26000;

    CAMERA_M32(cam, orbit_offset_x) = zoom >> 1;
    CAMERA_M32(cam, orbit_offset_y) = CAMERA_M16(cam, request_hold_ticks) << 10;
    CAMERA_M32(cam, orbit_offset_z) = (zoom >> 2) - (zoom * 2);

    if (CAMERA_M32(cam, focus_x) < CAMERA_M32(cam, stage_origin_x))
        CAMERA_M32(cam, target_x) = CAMERA_M32(cam, focus_x) + CAMERA_M32(cam, orbit_offset_x);
    else
        CAMERA_M32(cam, target_x) = CAMERA_M32(cam, focus_x) - CAMERA_M32(cam, orbit_offset_x);
    CAMERA_M32(cam, target_y) = CAMERA_M32(cam, focus_y) + CAMERA_M32(cam, orbit_offset_y);
    CAMERA_M32(cam, target_z) = CAMERA_M32(cam, focus_z) + CAMERA_M32(cam, orbit_offset_z);

    lookat_angles_xz(CAMERA_M32(cam, target_x), CAMERA_M32(cam, target_y), CAMERA_M32(cam, target_z),
                     CAMERA_M32(cam, focus_x), CAMERA_M32(cam, focus_y), CAMERA_M32(cam, focus_z),
                     &CAMERA_M16(cam, target_angle_x), &CAMERA_M16(cam, target_angle_y));

    CAMERA_M16(cam, target_angle_x) = -0x800;
    CAMERA_M16(cam, target_angle_z) = 0;
    camera_smooth_lerp(cam);

    {
        s32 d = CAMERA_M32(cam, stage_origin_x) - WORLD_POSE(cam)->position_x;
        if (d > CAMERA_M32(cam, horizontal_range) + 0x8000)
            WORLD_POSE(cam)->position_x = CAMERA_M32(cam, stage_origin_x) - CAMERA_M32(cam, horizontal_range) - 0x8000;
        else if (d < -CAMERA_M32(cam, horizontal_range) - 0x8000)
            WORLD_POSE(cam)->position_x = CAMERA_M32(cam, stage_origin_x) + CAMERA_M32(cam, horizontal_range) + 0x8000;
    }

    camera_shake_decay(task);
    mtx_build_fwd_inv(cam);   /* tail call */
}
