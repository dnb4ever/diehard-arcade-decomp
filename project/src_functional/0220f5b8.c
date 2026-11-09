/* debug_free_fly_camera [0x0220f5b8]: Maps direction/action input to local translation or view angles and calls debug service two. */
#include "dh.h"
#include "dh_work.h"
/* 0x0220f5b8 - debug / free-fly camera input handler for object `obj`. Reads the controller
 * word g_0609874C and, depending on which face button (bit0/bit1/none) is held, maps the
 * d-pad (0x80 up, 0x40 down, 0x200 right, 0x100 left) to either a local translation vector
 * {vx, vy, vz} or to the camera angles obj->{24,26,28}:
 *   no face button : 0x80/0x40 -> vz = +/-0x1000; 0x200/0x100 -> pitch obj->26 +/-0x100
 *   button 0 (bit0): 0x80/0x40 -> obj->C (height) +/-0x1000; 0x200/0x100 -> vx = -/+0x1000
 *   button 1 (bit1): 0x80/0x40 -> obj->24 +/-0x100; 0x200/0x100 -> obj->28 +/-0x100
 * It then rotates the translation vector by the camera angles (obj+0x24) through
 * mtx_push_rotate_apply (-> g_06098B1C), advances the camera X/Z position
 * obj->8 += 2*g_06098B1C[0], obj->10 += 2*g_06098B1C[2], and tail-calls func_0220ad58(2).
 * Clears obj->unk68 unless it is already 1. Real entry 0x20f5b8 (member 0x20f5b2 is
 * trailing-pool-contaminated). fidelity: HIGH - m2c decoded the input logic cleanly; the two
 * PIC calls (0x21080a, 0x20ad58) are resolved from asm; constants verified from the pool. */
void mtx_push_rotate_apply(s32 *vec, u16 *angles);      /* 0x0221080a */
void debug_draw_camera_pose(s32 arg0);                           /* 0x0220ad58 */

void debug_free_fly_camera(void *obj) {
    s32 vec[3];
    s16 inp;
    void *mat = (void *)DH_TRANSFORMED_VECTOR_ADDR;

    if (M32(obj, 0x68) != 1)
        M32(obj, 0x68) = 0;

    vec[0] = 0;
    vec[1] = 0;
    vec[2] = 0;
    inp = M16((void *)0x0609874C, 0);

    if (!((u16)inp & DH_PAD_ACTION1) && !((u16)inp & DH_PAD_ACTION2) && !((u16)inp & DH_PAD_ACTION3)) {
        if ((u16)inp & DH_PAD_Z_POS)  vec[2] = 0x1000;
        if ((u16)inp & DH_PAD_Z_NEG)  vec[2] = -0x1000;
        if ((u16)inp & DH_PAD_X_POS) WORLD_POSE(obj)->angle_y += 0x100;
        if ((u16)inp & DH_PAD_X_NEG) WORLD_POSE(obj)->angle_y -= 0x100;
    }
    if ((u16)inp & DH_PAD_ACTION1) {
        if ((u16)inp & DH_PAD_Z_POS)  WORLD_POSE(obj)->position_y += 0x1000;
        if ((u16)inp & DH_PAD_Z_NEG)  WORLD_POSE(obj)->position_y -= 0x1000;
        if ((u16)inp & DH_PAD_X_POS) vec[0] = -0x1000;
        if ((u16)inp & DH_PAD_X_NEG) vec[0] = 0x1000;
    }
    if ((u16)inp & DH_PAD_ACTION2) {
        if ((u16)inp & DH_PAD_Z_POS)  WORLD_POSE(obj)->angle_x += 0x100;
        if ((u16)inp & DH_PAD_Z_NEG)  WORLD_POSE(obj)->angle_x -= 0x100;
        if ((u16)inp & DH_PAD_X_POS) WORLD_POSE(obj)->angle_z += 0x100;
        if ((u16)inp & DH_PAD_X_NEG) WORLD_POSE(obj)->angle_z -= 0x100;
    }

    mtx_push_rotate_apply(vec, (u16 *)((char *)obj + 0x24));
    WORLD_POSE(obj)->position_x += M32(mat, 0) * 2;
    WORLD_POSE(obj)->position_z += M32(mat, 8) * 2;
    debug_draw_camera_pose(2);
}
