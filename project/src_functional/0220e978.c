/* camera_attach_to_source [0x0220e978]: Copies or transforms position/angles relative to the selected active source. */
#include "dh.h"
/* 0x0220e978 (attach object to parent/camera coordinate space) - selects a parent
 * transform by a mode value read from the global camera block at 0x06099590
 *   (mode = *(s32*)0x06099594 + *(s32*)0x06099598), then either initialises the
 * object (obj->unk68 == 1) by copying the source position (src) and angle triple
 * (ang) into it and latching the parent Z offset, or (steady state) just refreshes
 * obj->unk10 (Z) from the latched value obj->unkCC plus the parent's current Z
 * (parent->unk10).
 *   mode 0: nothing to do (return early).
 *   mode 1: parent = (*(0x06099584+0x30))->unk8
 *   mode 2: parent = (*(0x06099584+0x34))->unk8
 *   mode 3: parent = (*(0x060995B0+0x08))->unk8
 *   default: parent = 0x06099584 (base block itself)
 * fidelity: HIGH - all position fields 32-bit (16.16), angle fields 16-bit; a dead
 * scratch read of (*(0x060995B0+4)+8) in mode 3 and dead stack spills are dropped
 * (no side effects). Verified vs m2c --asm. */

void camera_attach_to_source(void *obj, void *src, void *ang) {
    void *parent = (void *)DH_ACTOR_SLOT_REGISTRY_ADDR;
    s32 mode = M32((void *)DH_ACTOR_SLOT_IDS_ADDR, 4) + M32((void *)DH_ACTOR_SLOT_IDS_ADDR, 8);

    switch (mode) {
    case 0:
        return;
    case 1:
        parent = MP(MP((void *)DH_ACTOR_SLOT_REGISTRY_ADDR, 0x30), 8);
        break;
    case 2:
        parent = MP(MP((void *)DH_ACTOR_SLOT_REGISTRY_ADDR, 0x34), 8);
        break;
    case 3:
        parent = MP(MP((void *)DH_ACTOR_SLOT_TASKS_ADDR, 8), 8);
        break;
    default:
        break;
    }

    if (M32(obj, 0x68) == 1) {
        M32(obj, 0x8)  = M32(src, 0x0);
        M32(obj, 0xC)  = M32(src, 0x4);
        M32(obj, 0x10) = M32(src, 0x8) + M32(parent, 0x10);
        M32(obj, 0xCC) = M32(src, 0x8);
        M16(obj, 0x24) = M16(ang, 0x0);
        M16(obj, 0x26) = M16(ang, 0x2);
        M16(obj, 0x28) = M16(ang, 0x4);
        M32(obj, 0x68) = 0;
    } else {
        M32(obj, 0x10) = M32(obj, 0xCC) + M32(parent, 0x10);
    }
}
