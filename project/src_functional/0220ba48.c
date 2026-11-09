/* camera_approach_medium_task [0x0220ba48]: Eases medium-mode framing and switches to the steady medium handler. */
#include "dh_work.h"
#include "dh.h"
/* 0x0220ba48 (camera "approach" handler, medium mode) - partner of steady func_0220b8d4;
 * same skeleton (see func_0220b8d4). view_bounds_from_actors(cam, &box[3], &box[0]) fills the box
 * (box[0]=max_x, box[2]=max_z, box[3]=min_x, box[5]=min_z). Drives zoom (two optional
 * reductions, the second gated by a flag set when the depth term is positive), the eye
 * offset, smooths, X-clamps, ticks shake, and hands off to steady func_0220b8d4 when the
 * framed span becomes tight.
 *   target: unk90=(min_x+max_x)>>1 ; unk94=0x10000 ; unk98=(min_z*3+max_z)>>2
 *   zoom=0x10000; if (unk98-unk8C)>0 { zoom-=that>>3; flag=0 } ; w=|unk90-unk84|;
 *     if w>0x20000 zoom-=(w-0x20000)>>3 ;
 *     if flag { dw=span_x; dz=span_z; m=dz-(dz>>2); v=max(dw,m); if v<0x90000 zoom-=(0x90000-v)>>4 }
 *     clamp zoom>=0x8000
 *   offset: unk9C=0 ; unkA0=zoom*6-(zoom>>2) ; unkA4=-(zoom*7)
 *   eye: unkB8/BC/C0=target+offset ; unkC4=-0x2000 ; unkC6=C8=0
 *   X clamp |unk84-unk8|<=0x20000; handoff if span_x<0x60000 && span_z<0x14000.
 * fidelity: HIGH - coords 32-bit, unkC4 16-bit; shifts arithmetic; computed branches and
 * next-handler pointer resolved; box[1]/box[4] untouched, unused. Verified vs asm. */
extern void view_bounds_from_actors(void *cam, s32 *hi, s32 *lo);
extern void camera_ease_to_target(void *cam);
extern void camera_tick_shake(void *task);
extern void mtx_build_fwd_inv(void *cam);
extern void camera_frame_medium_task(void *task);

void camera_approach_medium_task(void *task) {
    void *cam = TASK_PTR(task, work);
    s32 box[6];
    s32 zoom = 0x10000;
    s32 flag = 1;
    s32 t, w;

    view_bounds_from_actors(cam, &box[3], &box[0]);

    CAMERA_M32(cam, focus_x) = (box[3] + box[0]) >> 1;
    CAMERA_M32(cam, focus_y) = 0x10000;
    CAMERA_M32(cam, focus_z) = (box[5] * 3 + box[2]) >> 2;

    t = CAMERA_M32(cam, focus_z) - CAMERA_M32(cam, stage_origin_z);
    if (t > 0) {
        zoom = 0x10000 - (t >> 3);
        flag = 0;
    }

    w = CAMERA_M32(cam, focus_x) - CAMERA_M32(cam, stage_origin_x);
    if (w < 0)
        w = -w;
    if (w > 0x20000)
        zoom -= (w + (s32)0xFFFE0000) >> 3;

    if (flag != 0) {
        s32 dw = box[0] - box[3];
        s32 dz = box[2] - box[5];
        s32 m = dz - (dz >> 2);
        s32 v = (dw < m) ? m : dw;
        if (v < 0x90000)
            zoom -= (0x90000 - v) >> 4;
    }

    if (zoom < 0x8000)
        zoom = 0x8000;

    CAMERA_M32(cam, orbit_offset_x) = 0;
    CAMERA_M32(cam, orbit_offset_y) = zoom * 6 - (zoom >> 2);
    CAMERA_M32(cam, orbit_offset_z) = -(zoom * 7);
    CAMERA_M32(cam, target_x) = CAMERA_M32(cam, focus_x) + CAMERA_M32(cam, orbit_offset_x);
    CAMERA_M32(cam, target_y) = CAMERA_M32(cam, focus_y) + CAMERA_M32(cam, orbit_offset_y);
    CAMERA_M32(cam, target_z) = CAMERA_M32(cam, focus_z) + CAMERA_M32(cam, orbit_offset_z);
    CAMERA_M16(cam, target_angle_x) = -0x2000;
    CAMERA_M16(cam, target_angle_y) = 0;
    CAMERA_M16(cam, target_angle_z) = 0;

    camera_ease_to_target(cam);

    t = CAMERA_M32(cam, stage_origin_x) - CAMERA_M32(cam, position_x);
    if (t > 0x20000)
        CAMERA_M32(cam, position_x) = CAMERA_M32(cam, stage_origin_x) + (s32)0xFFFE0000;
    else if (t < (s32)0xFFFE0000)
        CAMERA_M32(cam, position_x) = CAMERA_M32(cam, stage_origin_x) + 0x20000;

    camera_tick_shake(task);

    if ((box[0] - box[3]) < 0x60000 && (box[2] - box[5]) < 0x14000)
        CAMERA_MP(cam, update_handler) = (void *)camera_frame_medium_task;

    mtx_build_fwd_inv(cam);
}
