#include "dh_work.h"
/* camera_ease_to_target [0x0220e0aa]: Eases eye coordinates and view angles toward camera target fields with capped steps. */
#include "dh.h"
/* 0x0220e0aa (camera smoothing/lerp, leaf) - eases the camera's current eye position
 * (unk8/C/10) toward its target (unkB8/BC/C0) and its current view angles (unk24/26)
 * toward the target angles (unkC4/C6), snapping the roll angle (unk28 = unkC8).
 *   position axes: d = target - cur; if d > 0x18000 step +0x3000; else if d < -0x18000
 *     step -0x3000; else cur += d>>3.
 *   angles: d = (s16)((u16)target - (u16)cur) (wrapping delta); if d > 0x900 step +0x120;
 *     else if d < -0x900 step -0x120; else cur += d>>3.
 * fidelity: HIGH - position 32-bit (16.16), angles 16-bit with unsigned-wrap delta; all
 * shifts arithmetic; a dead stack spill of the first angle delta is dropped. Verified vs asm. */

void camera_ease_to_target(void *cam) {
    s32 d;
    s16 a;

    d = CAMERA_M32(cam, target_x) - CAMERA_M32(cam, position_x);
    if (d > 0x18000)              CAMERA_M32(cam, position_x) += 0x3000;
    else if (d < (s32)0xFFFE8000) CAMERA_M32(cam, position_x) -= 0x3000;
    else                          CAMERA_M32(cam, position_x) += d >> 3;

    d = CAMERA_M32(cam, target_y) - CAMERA_M32(cam, position_y);
    if (d > 0x18000)              CAMERA_M32(cam, position_y) += 0x3000;
    else if (d < (s32)0xFFFE8000) CAMERA_M32(cam, position_y) -= 0x3000;
    else                          CAMERA_M32(cam, position_y) += d >> 3;

    d = CAMERA_M32(cam, target_z) - CAMERA_M32(cam, position_z);
    if (d > 0x18000)              CAMERA_M32(cam, position_z) += 0x3000;
    else if (d < (s32)0xFFFE8000) CAMERA_M32(cam, position_z) -= 0x3000;
    else                          CAMERA_M32(cam, position_z) += d >> 3;

    a = (s16)((u16)CAMERA_M16(cam, target_angle_x) - (u16)CAMERA_M16(cam, angle_x));
    if (a > 0x900)       CAMERA_M16(cam, angle_x) += 0x120;
    else if (a < -0x900) CAMERA_M16(cam, angle_x) -= 0x120;
    else                 CAMERA_M16(cam, angle_x) += a >> 3;

    a = (s16)((u16)CAMERA_M16(cam, target_angle_y) - (u16)CAMERA_M16(cam, angle_y));
    if (a > 0x900)       CAMERA_M16(cam, angle_y) += 0x120;
    else if (a < -0x900) CAMERA_M16(cam, angle_y) -= 0x120;
    else                 CAMERA_M16(cam, angle_y) += a >> 3;

    CAMERA_M16(cam, angle_z) = CAMERA_M16(cam, target_angle_z);
}
