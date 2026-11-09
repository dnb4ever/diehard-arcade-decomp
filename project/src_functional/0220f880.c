/* camera_step_current_definition [0x0220f880]: Restarts the current camera definition and sequences its callbacks and countdown. */
#include "dh.h"
/* 0x0220f880 - per-frame scene-object animation stepper (variant B; sibling of func_0220f6f6).
 * Same structure, but the restart case rebinds the object's own current definition rather than
 * the global default. The scene node `scene` owns obj = scene->unk8 running cur = obj->unkD4.
 * Dispatches on scene->unk10:
 *   0: (re)start - clear obj->{68,3C}, rebind d = obj->unkD4 via func_0220f816, then run its
 *      step callback d->unk0(obj, d->unk4, d->unk8).
 *   1: countdown - if (--obj->unkAE == 0) raise g_06098664 = 1.
 *   2: run cur->unk0(obj, cur->unk4, cur->unk8); if obj->unk6C != 0 tick the countdown;
 *      if obj->unk3C != 0, clear it and advance to cur+0x10 via func_0220f816.
 *   3: like 2, but when obj->unk6C == 0 the countdown advances to cur+0x10 at 0.
 * Finally tail-calls mtx_build_fwd_inv(obj) (PIC braf -> 0x214a7a). fidelity: HIGH -
 * hand-transcribed from asm; PIC deltas verified; step callbacks indirect through unk0. */
void camera_bind_scene_definition(void *scene, void *def);
void mtx_build_fwd_inv(void *obj);                      /* 0x0214a7a */

void camera_step_current_definition(void *scene) {
    void *obj = MP(scene, 8);
    void *cur = MP(obj, 0xD4);
    void *d;

    switch (M8(scene, 0x10)) {
    case 0:
        M32(obj, 0x68) = 0;
        M32(obj, 0x3C) = 0;
        d = MP(obj, 0xD4);
        camera_bind_scene_definition(scene, d);
        ((void (*)(void *, s32, s32))MP(d, 0))(obj, M32(d, 4), M32(d, 8));
        break;
    case 1:
        M16(obj, 0xAE) -= 1;
        if (M16(obj, 0xAE) == 0)
            M32((void *)0x06098664, 0) = 1;
        break;
    case 2:
        ((void (*)(void *, s32, s32))MP(cur, 0))(obj, M32(cur, 4), M32(cur, 8));
        if (M32(obj, 0x6C) != 0) {
            M16(obj, 0xAE) -= 1;
            if (M16(obj, 0xAE) == 0)
                M32((void *)0x06098664, 0) = 1;
        }
        if (M32(obj, 0x3C) != 0) {
            M32(obj, 0x3C) = 0;
            camera_bind_scene_definition(scene, (char *)cur + 0x10);
        }
        break;
    case 3:
        ((void (*)(void *, s32, s32))MP(cur, 0))(obj, M32(cur, 4), M32(cur, 8));
        if (M32(obj, 0x6C) != 0) {
            M16(obj, 0xAE) -= 1;
            if (M16(obj, 0xAE) == 0)
                M32((void *)0x06098664, 0) = 1;
        } else {
            M16(obj, 0xAE) -= 1;
            if (M16(obj, 0xAE) == 0)
                camera_bind_scene_definition(scene, (char *)cur + 0x10);
        }
        break;
    default:
        break;
    }

    mtx_build_fwd_inv(obj);   /* tail call */
}
