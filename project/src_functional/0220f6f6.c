/* camera_step_scene_definition [0x0220f6f6]: Runs the current scene camera definition, its countdown and step callback, then rebuilds view matrices. */
#include "dh.h"
/* 0x0220f6f6 - per-frame scene-object animation stepper (variant A). The scene node `scene`
 * owns an object obj = scene->unk8 running the definition cur = obj->unkD4. Dispatches on the
 * sub-state byte scene->unk10:
 *   0: (re)start - clear obj->{64,68,6C,3C}, bind the default definition
 *      (*g_0609866C)->unk10 via func_0220f816, then run its step callback def->unk0(obj,
 *      def->unk4, def->unk8).
 *   1: countdown - if (--obj->unkAE == 0) raise the scene-advance flag g_06098664 = 1.
 *   2: run cur->unk0(obj, cur->unk4, cur->unk8); if obj->unk6C != 0 tick the countdown
 *      (raising g_06098664 at 0); if obj->unk3C != 0, clear it and advance to the next
 *      definition cur+0x10 via func_0220f816.
 *   3: like 2, but when obj->unk6C == 0 the countdown instead advances to cur+0x10 at 0.
 * Finally tail-calls mtx_build_fwd_inv(obj) (PIC braf -> 0x214a7a) to rebuild the object's
 * model matrices. fidelity: HIGH - hand-transcribed from asm (m2c aborted on the embedded
 * helper/braf). PIC deltas verified; the step callbacks are indirect through def->unk0. */
void camera_bind_scene_definition(void *scene, void *def);
void mtx_build_fwd_inv(void *obj);                      /* 0x0214a7a */

void camera_step_scene_definition(void *scene) {
    void *obj = MP(scene, 8);
    void *cur = MP(obj, 0xD4);
    void *d;

    switch (M8(scene, 0x10)) {
    case 0:
        M32(obj, 0x64) = 0;
        M32(obj, 0x68) = 0;
        M32(obj, 0x6C) = 0;
        M32(obj, 0x3C) = 0;
        d = MP(MP((void *)DH_SCENE_DESCRIPTOR_ADDR, 0), 0x10);
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
