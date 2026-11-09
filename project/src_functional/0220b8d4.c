/* camera_frame_medium_task [0x0220b8d4]: Frames active actor bounds with medium-mode zoom and offset tuning. */
#include "dh_work.h"
#include "dh.h"
/* 0x0220b8d4 (camera-framing task handler, "medium" tuning variant) - task->unk8 = entity
 * (the camera). Calls view_bounds_from_actors(cam, &box[3], &box[0]) which fills two 3-long vectors
 * on the stack (box[0..2] and box[3..5]); from them it computes the look-at/target point,
 * a distance-driven zoom factor, the camera offset vector, the final eye position and a
 * pitch angle, then runs the follow-up helpers and, when the framed span is too wide,
 * swaps in the next camera handler func_0220ba48.
 *   target:  unk90 = (box0+box3)>>1 ; unk94 = 0x10000 ; unk98 = (box5*3+box2)>>2
 *   zoom:    0x10000, reduced when (box0-box3) < 0x60000 (clamped to >=0x28000)
 *   offset:  unk9C=0 ; unkA0=(zoom>>1)+zoom ; unkA4=-(zoom*7)
 *   eye:     unkB8/BC/C0 = target + offset ; unkC4 = ((unk98-unk8C)>>8)+0xF600 ; unkC6=C8=0
 *   then func_0220e0aa(cam); clamp |unk84-unk8| to <=0x20000 (pull unk8 toward unk84);
 *   func_0220d98e(task); if span wide set unk64=func_0220ba48; mtx_build_fwd_inv(cam).
 * fidelity: HIGH - all coords 32-bit (16.16), angle unkC4 16-bit; shifts arithmetic;
 * computed branches resolved (braf_resolve), next-handler pointer resolved (ptr_resolve).
 * box[1]/box[4] are neither written by the callee nor read here. Verified vs asm. */
extern void view_bounds_from_actors(void *cam, s32 *hi, s32 *lo);
extern void camera_ease_to_target(void *cam);
extern void camera_tick_shake(void *task);
extern void mtx_build_fwd_inv(void *cam);
extern void camera_approach_medium_task(void *task);

void camera_frame_medium_task(void *task) {
    void *cam = TASK_PTR(task, work);
    s32 box[6];
    s32 zoom = 0x10000;
    s32 d;

    view_bounds_from_actors(cam, &box[3], &box[0]);

    CAMERA_M32(cam, focus_x) = (box[0] + box[3]) >> 1;
    CAMERA_M32(cam, focus_y) = 0x10000;
    CAMERA_M32(cam, focus_z) = (box[5] * 3 + box[2]) >> 2;

    d = box[0] - box[3];
    if (d < 0x60000) {
        if (d < 0x28000)
            d = 0x28000;
        zoom = 0x10000 - ((0x60000 - d) >> 3);
    }
    CAMERA_M32(cam, orbit_offset_x) = 0;
    CAMERA_M32(cam, orbit_offset_y) = (zoom >> 1) + zoom;
    CAMERA_M32(cam, orbit_offset_z) = -(zoom * 7);
    CAMERA_M32(cam, target_x) = CAMERA_M32(cam, focus_x) + CAMERA_M32(cam, orbit_offset_x);
    CAMERA_M32(cam, target_y) = CAMERA_M32(cam, focus_y) + CAMERA_M32(cam, orbit_offset_y);
    CAMERA_M32(cam, target_z) = CAMERA_M32(cam, focus_z) + CAMERA_M32(cam, orbit_offset_z);
    CAMERA_M16(cam, target_angle_x) = (s16)(((CAMERA_M32(cam, focus_z) - CAMERA_M32(cam, stage_origin_z)) >> 8) + 0xF600);
    CAMERA_M16(cam, target_angle_y) = 0;
    CAMERA_M16(cam, target_angle_z) = 0;

    camera_ease_to_target(cam);

    d = CAMERA_M32(cam, stage_origin_x) - CAMERA_M32(cam, position_x);
    if (d > 0x20000)
        CAMERA_M32(cam, position_x) = CAMERA_M32(cam, stage_origin_x) + (s32)0xFFFE0000;
    else if (d < (s32)0xFFFE0000)
        CAMERA_M32(cam, position_x) = CAMERA_M32(cam, stage_origin_x) + 0x20000;

    camera_tick_shake(task);

    if ((box[0] - box[3]) > 0x80000 || (box[2] - box[5]) > 0x1C000)
        CAMERA_MP(cam, update_handler) = (void *)camera_approach_medium_task;

    mtx_build_fwd_inv(cam);
}
