#include "dh_work.h"
#include "dh.h"
/* 0x0220bc20 (camera_init_pose) - (re)initialise the camera's pose at scene start.
 * (Real entry; cart 0x20bbfa is a pool-contaminated pre-entry of this one. NOT a HUD
 * builder - it was grouped with them in the triage but calls mtx_build_fwd_inv, not the
 * ui_printf thunk.)
 *
 * ABI: arg0 = task; cam = task->unk8.
 *
 *  - If an override camera is active (*g_06098670 != 0): load the eye position (unkB8/BC/C0,
 *    mirrored into unk8/C/10) from table 0x06051e8c[(*g_0609866C).unk28] (stride 0xC) and the
 *    angles (unkC4/C6/C8, mirrored into unk24/26/28) from 0x06051f94[(*g_0609866C).unk29]
 *    (stride 6). Otherwise load the defaults at 0x06051e78 (pos) / 0x06051e84 (angles).
 *  - Add the current room's world offset (0x06060814[g_060B3D54], stride 0x2C -> cam
 *    unk84/88/8C) into the eye position.
 *  - Reset bookkeeping: g_06098830 = 10, g_06098834 = 0, task->unkD = 0, cam->unk68 =
 *    0x30000, cam->unkA8 = cam->unkAA = 0, and clear the ttl (+4) of all 8 entries in the
 *    recent-position ring at 0x06098838 (the one view_bounds_from_actors @0x20d9ec consumes).
 *  - Install the per-frame handler (task->unk0 = func_0220bdb4), seed cam->unk64 from
 *    0x06051e74, and build the camera matrices via mtx_build_fwd_inv (0x214a7a).
 *  - If the override is mode 1 and g_06098830 >= 3: force g_06098830 = 3, mark the object at
 *    *g_0609882C (unkD = 1) and zero its entity's view indices (unkAE/B0/B2).
 *
 * fidelity: HIGH - hand-traced vs m2c_fn --asm (0x20bc20..0x20bdb2). Sole bsrf -> 0x214a7a.
 * Pose table scalings reproduce the exts.w/extu.b truncations the asm applies to the index. */

extern void mtx_build_fwd_inv(void *cam);   /* 0x214a7a */
extern void camera_run_mode2_handler(void *task);      /* installed per-frame handler */

void camera_init_pose(void *task)
{
    void *cam = TASK_PTR(task, work);
    s32 i;

    if (*(s32 *)(uintptr_t)DH_PLAY_SESSION_MODE_ADDR != 0) {
        void *sel = *(void **)(uintptr_t)DH_SCENE_DESCRIPTOR_ADDR;
        s32 *pr = (s32 *)(uintptr_t)(0x06051e8c + (s16)(SCENE_M16(sel, pose_index) * 0xC));
        s16 *ar = (s16 *)(uintptr_t)(0x06051f94 + (u8)(SCENE_M8(sel, angle_variant) * 6));
        CAMERA_M32(cam, target_x) = pr[0]; CAMERA_M32(cam, target_y) = pr[1]; CAMERA_M32(cam, target_z) = pr[2];
        CAMERA_M32(cam, position_x) = pr[0]; CAMERA_M32(cam, position_y) = pr[1]; CAMERA_M32(cam, position_z) = pr[2];
        CAMERA_M16(cam, target_angle_x) = ar[0]; CAMERA_M16(cam, target_angle_y) = ar[1]; CAMERA_M16(cam, target_angle_z) = ar[2];
        CAMERA_M16(cam, angle_x) = ar[0]; CAMERA_M16(cam, angle_y) = ar[1]; CAMERA_M16(cam, angle_z) = ar[2];
    } else {
        s32 *pr = (s32 *)(uintptr_t)0x06051e78;
        s16 *ar = (s16 *)(uintptr_t)0x06051e84;
        CAMERA_M32(cam, target_x) = pr[0]; CAMERA_M32(cam, target_y) = pr[1]; CAMERA_M32(cam, target_z) = pr[2];
        CAMERA_M32(cam, position_x) = pr[0]; CAMERA_M32(cam, position_y) = pr[1]; CAMERA_M32(cam, position_z) = pr[2];
        CAMERA_M16(cam, target_angle_x) = ar[0]; CAMERA_M16(cam, target_angle_y) = ar[1]; CAMERA_M16(cam, target_angle_z) = ar[2];
        CAMERA_M16(cam, angle_x) = ar[0]; CAMERA_M16(cam, angle_y) = ar[1]; CAMERA_M16(cam, angle_z) = ar[2];
    }

    {
        s32 *room = (s32 *)(uintptr_t)(0x06060814 + *(s32 *)(uintptr_t)0x060b3d54 * 0x2C);
        CAMERA_M32(cam, stage_origin_x) = room[0]; CAMERA_M32(cam, stage_origin_y) = room[1]; CAMERA_M32(cam, stage_origin_z) = room[2];
    }
    CAMERA_M32(cam, position_x) += CAMERA_M32(cam, stage_origin_x);
    CAMERA_M32(cam, position_y) += CAMERA_M32(cam, stage_origin_y);
    CAMERA_M32(cam, position_z) += CAMERA_M32(cam, stage_origin_z);

    *(s32 *)(uintptr_t)DH_VIEW_REQUEST_PRIORITY_ADDR = 10;
    *(s32 *)(uintptr_t)0x06098834 = 0;
    M8(task, 0xD) = 0;
    CAMERA_M32(cam, horizontal_range) = 0x30000;
    CAMERA_M16(cam, request_hold_ticks) = 0;
    CAMERA_M16(cam, shake_ticks) = 0;
    for (i = 0; i < 8; i++)
        M32((void *)(uintptr_t)0x06098838, i * 8 + 4) = 0;

    TASK_S32(task, handler) = (s32)(uintptr_t)camera_run_mode2_handler;
    CAMERA_M32(cam, update_handler) = *(s32 *)(uintptr_t)0x06051e74;
    mtx_build_fwd_inv(cam);

    if (*(s32 *)(uintptr_t)DH_PLAY_SESSION_MODE_ADDR == 1 && *(s32 *)(uintptr_t)DH_VIEW_REQUEST_PRIORITY_ADDR >= 3) {
        void *o = *(void **)(uintptr_t)DH_VIEW_TASK_ADDR;
        void *oe = MP(o, 8);
        *(s32 *)(uintptr_t)DH_VIEW_REQUEST_PRIORITY_ADDR = 3;
        M8(o, 0xD) = 1;
        M16(oe, 0xAE) = 0;
        M16(oe, 0xB0) = 0;
        M16(oe, 0xB2) = 0;
    }
}
