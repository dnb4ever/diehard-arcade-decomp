/* camera_bind_scene_definition [0x0220f816]: Binds a sixteen-byte camera definition and initializes its mode and countdown. */
#include "dh.h"
/* 0x0220f816 - (anim)state helper, shared by the scene steppers func_0220f6f6 / func_0220f880.
 * Binds a new animation/behaviour definition `def` to the object owned by the scene node
 * `scene` (obj = scene->unk8):
 *   obj->unkD4 = def;                         (current definition)
 *   scene->unk10 = (u8)def->unkC;             (low byte = initial sub-state)
 *   obj->unk6C = (s16)def->unkC >> 8;         (high byte, sign-extended)
 *   obj->unk68 = 1;                           (mark "just (re)initialised")
 * then, by the freshly-set sub-state scene->unk10:
 *   0 : obj->unkAE = 1;                 scene->unk10 = 1;
 *   1 : obj->unk68 = 0; obj->unkAE = def->unkE; scene->unk10 = 1;
 *   2,3: obj->unkAE = def->unkE;
 * fidelity: HIGH - transcribed from the fn.py disassembly; leaf (no calls). */
void camera_bind_scene_definition(void *scene, void *def) {
    void *obj = MP(scene, 8);

    MP(obj, 0xD4) = def;
    M8(scene, 0x10) = (s8)M16(def, 0xC);
    M32(obj, 0x6C) = M16(def, 0xC) >> 8;
    M32(obj, 0x68) = 1;

    switch (M8(scene, 0x10)) {
    case 0:
        M16(obj, 0xAE) = 1;
        M8(scene, 0x10) = 1;
        break;
    case 1:
        M32(obj, 0x68) = 0;
        M16(obj, 0xAE) = M16(def, 0xE);
        M8(scene, 0x10) = 1;
        break;
    case 2:
    case 3:
        M16(obj, 0xAE) = M16(def, 0xE);
        break;
    default:
        break;
    }
}
