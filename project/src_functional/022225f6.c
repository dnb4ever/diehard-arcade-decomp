#include "dh_work.h"
/* view_build_display_transform [0x022225f6]: Uses view task position/angles to fill display transform with scaled offsets and sine/cosine rotation. */
#include "dh.h"
/* 0x022225f6  build the view transform for display object `mode` (0 or 1). Source object is
 * (*0x0609882C)->unk8; its world position (0x8/0xC/0x10) is taken relative to the active camera
 * (0x060755D4, or 0x060755E0 when the object's control flags bit1 is set) and scaled by 16 or 32,
 * storing screen offsets into the matrix at 0x060B8B90/0x060B8B94 (+0x44/+0x48/+0x60). The 3x3
 * rotation block (+0x1C..+0x30, +0x68, +0x6C) is built from sin/cos (sin_lookup/cos_lookup)
 * of the object's two Euler angles (src+0x24, src+0x26) via sum/difference identities, then
 * func_02222c08 finalises the matrix. No-op unless control flags bit0 (0x060BACAC/0x060BACB0) is set.
 * fidelity: HIGH - all fields, shifts and the sum/difference trig verified against the ROM. */
extern void *g_view_task_legacy;
extern void *g_060B8B90;
extern void *g_060B8B94;
extern s32 g_060BAC98;
extern s32 g_060BAC9C;
extern s32 g_060BACA0;
extern s32 g_060BACA4;
extern s32 g_060BACAC;
extern s32 g_060BACB0;
extern s8  g_060755D4[];                    /* camera position vectors: A at +0, B at +0xC */
extern s32 sin_lookup(s32 angle);        /* fixed-point sine */
extern s32 cos_lookup(s32 angle);        /* fixed-point cosine */
extern void perspective_build_edge_samples(void *m, s32 arg1, s32 arg2, s32 flag);
void view_build_display_transform(s32 mode) {
    void *src = TASK_PTR(g_view_task_legacy, work);
    void *dst;
    s32 ofs, arg1, flags, shift;
    s8 *cam;
    s32 dx, dy, dz, dyo, a, b;
    s32 sin_ab, sin_amb, cos_ab, cos_amb;

    if (mode == 1) {
        dst = g_060B8B94;
        arg1 = g_060BAC9C;
        ofs = g_060BACA4;
        flags = g_060BACB0;
    } else {
        dst = g_060B8B90;
        arg1 = g_060BAC98;
        ofs = g_060BACA0;
        flags = g_060BACAC;
    }
    if (!(flags & 1)) {
        return;
    }
    if (flags & 2) {
        cam = g_060755D4 + 0xC;
        shift = 5;
    } else {
        cam = g_060755D4;
        shift = 4;
    }
    dx = WORLD_POSE(src)->position_x  - M32(cam, 0x0);
    dy = WORLD_POSE(src)->position_y  - M32(cam, 0x4);
    dz = WORLD_POSE(src)->position_z - M32(cam, 0x8);
    dyo = dy - ofs;
    M32(dst, 0x44) = dx << shift;
    M32(dst, 0x48) = -(dz << shift);
    M32(dst, 0x60) = dyo << 8;

    a = WORLD_POSE(src)->angle_x;
    b = WORLD_POSE(src)->angle_y;
    sin_ab  = sin_lookup(a + b);
    sin_amb = sin_lookup(a - b);
    cos_ab  = cos_lookup(a + b);
    cos_amb = cos_lookup(a - b);
    M32(dst, 0x1C) = cos_lookup(b);
    M32(dst, 0x20) = (cos_ab - cos_amb) >> 1;
    M32(dst, 0x24) = (sin_ab - sin_amb) >> 1;
    M32(dst, 0x28) = -sin_lookup(b);
    M32(dst, 0x2C) = (-sin_ab - sin_amb) >> 1;
    M32(dst, 0x30) = (cos_ab + cos_amb) >> 1;
    M32(dst, 0x64) = 0;
    M32(dst, 0x68) = -cos_lookup(a);
    M32(dst, 0x6C) = -sin_lookup(a);
    perspective_build_edge_samples(dst, arg1, 0xF0, flags & 2);
}
