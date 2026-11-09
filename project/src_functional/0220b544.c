/* camera_approach_near_task [0x0220b544]: Eases near-mode framing and hands off to the steady near camera when the span permits. */
#include "dh_work.h"
#include "dh.h"
/* 0x0220b544 (camera "approach" handler, near/tight mode) - partner of the steady handler
 * func_0220b3cc. Same skeleton as func_0220b8d4 (see it): view_bounds_from_actors(cam, &box[3],
 * &box[0]) fills the framing box (box[0]=max_x, box[2]=max_z, box[3]=min_x, box[5]=min_z),
 * then it drives a zoom factor and the eye offset, smooths (func_0220e0aa), clamps X to
 * unk84, ticks shake (func_0220d98e) and, once the framed span is tight enough, hands off
 * to the steady handler func_0220b3cc; finally mtx_build_fwd_inv(cam).
 *   target: unk90=(min_x+max_x)>>1 ; unk94=0x14000 ; unk98=(min_z+max_z)>>1
 *   zoom=0x10000; if (unk98-unk8C+0x10000)>0 zoom-= that>>3; w=|unk90-unk84|;
 *     if w>0x20000 zoom-=(w-0x20000)>>3; clamp zoom>=0x8000
 *   offset: unk9C=0 ; unkA0=zoom*4 ; unkA4=-(zoom>>1)-zoom*4
 *   eye: unkB8/BC/C0=target+offset ; unkC4=-0x2000 ; unkC6=C8=0
 *   X clamp |unk84-unk8|<=0x10000; handoff if span_x<0x58000 && span_z<0x14000.
 * fidelity: HIGH - coords 32-bit, unkC4 16-bit; shifts arithmetic; computed branches and
 * next-handler pointer resolved; box[1]/box[4] untouched by callee, unused. Verified vs asm. */
extern void view_bounds_from_actors(void *cam, s32 *hi, s32 *lo);
extern void camera_ease_to_target(void *cam);
extern void camera_tick_shake(void *task);
extern void mtx_build_fwd_inv(void *cam);
extern void camera_frame_near_task(void *task);

void camera_approach_near_task(void *task) {
    void *cam = TASK_PTR(task, work);
    s32 box[6];
    s32 zoom = 0x10000;
    s32 t, w, a0;

    view_bounds_from_actors(cam, &box[3], &box[0]);

    CAMERA_M32(cam, focus_x) = (box[3] + box[0]) >> 1;
    CAMERA_M32(cam, focus_y) = 0x14000;
    CAMERA_M32(cam, focus_z) = (box[5] + box[2]) >> 1;

    t = (CAMERA_M32(cam, focus_z) - CAMERA_M32(cam, stage_origin_z)) + 0x10000;
    if (t > 0)
        zoom = 0x10000 - (t >> 3);

    w = CAMERA_M32(cam, focus_x) - CAMERA_M32(cam, stage_origin_x);
    if (w < 0)
        w = -w;
    if (w > 0x20000)
        zoom -= (w + (s32)0xFFFE0000) >> 3;

    if (zoom < 0x8000)
        zoom = 0x8000;

    a0 = zoom * 4;
    CAMERA_M32(cam, orbit_offset_x) = 0;
    CAMERA_M32(cam, orbit_offset_y) = a0;
    CAMERA_M32(cam, orbit_offset_z) = -(zoom >> 1) - a0;
    CAMERA_M32(cam, target_x) = CAMERA_M32(cam, focus_x) + CAMERA_M32(cam, orbit_offset_x);
    CAMERA_M32(cam, target_y) = CAMERA_M32(cam, focus_y) + CAMERA_M32(cam, orbit_offset_y);
    CAMERA_M32(cam, target_z) = CAMERA_M32(cam, focus_z) + CAMERA_M32(cam, orbit_offset_z);
    CAMERA_M16(cam, target_angle_x) = -0x2000;
    CAMERA_M16(cam, target_angle_y) = 0;
    CAMERA_M16(cam, target_angle_z) = 0;

    camera_ease_to_target(cam);

    t = CAMERA_M32(cam, stage_origin_x) - CAMERA_M32(cam, position_x);
    if (t > 0x10000)
        CAMERA_M32(cam, position_x) = CAMERA_M32(cam, stage_origin_x) + (s32)0xFFFF0000;
    else if (t < (s32)0xFFFF0000)
        CAMERA_M32(cam, position_x) = CAMERA_M32(cam, stage_origin_x) + 0x10000;

    camera_tick_shake(task);

    if ((box[0] - box[3]) < 0x58000 && (box[2] - box[5]) < 0x14000)
        CAMERA_MP(cam, update_handler) = (void *)camera_frame_near_task;

    mtx_build_fwd_inv(cam);
}
