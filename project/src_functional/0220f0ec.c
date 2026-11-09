/* camera_copy_source_part_pose [0x0220f0ec]: Selects an active source, copies its part position and angles and clears initialization. */
#include "dh.h"
/* 0x0220f0ec - refresh an object's sprite-attribute block from the active stage source
 * record. Source `src` defaults to the global stage record g_06099584; the selector
 * (g->unk10 + g->unk14) chooses 1/3 -> *(g->unk30)->unk8, 2 -> *(g->unk34)->unk8, and
 * 0 -> early-out (after clearing obj->unk68). It copies a 0x100-stride entry from the
 * attribute table g_06097d68 (index = src->unk34 byte) into obj->{8,C,10} and 3 shorts
 * src->{24,26,28} -> obj->{24,26,28}. Clears obj->unk68 if it was 1 (done before the
 * switch). fn.units member 0x20f0ee is this body minus its leading pool load -> phantom.
 * fidelity: HIGH - direct transcription of m2c asm; no computed branches. */
#define G_SRC  ((void *)DH_ACTOR_SLOT_REGISTRY_ADDR)
#define G_ATTR ((u8 *)0x06097d68)

void camera_copy_source_part_pose(void *obj) {
    void *src = G_SRC;
    s32 sel = M32(G_SRC, 0x10) + M32(G_SRC, 0x14);
    u8 *ent;

    if (M32(obj, 0x68) == 1)
        M32(obj, 0x68) = 0;

    switch (sel) {
    case 0:
        return;
    case 1:
    case 3:
        src = MP(MP(G_SRC, 0x30), 8);
        break;
    case 2:
        src = MP(MP(G_SRC, 0x34), 8);
        break;
    default:
        break;
    }

    ent = G_ATTR + (M8(src, 0x34) << 8);
    M32(obj, 0x08) = M32(ent, 0);
    M32(obj, 0x0C) = M32(ent, 4);
    M32(obj, 0x10) = M32(ent, 8);
    M16(obj, 0x24) = M16(src, 0x24);
    M16(obj, 0x26) = M16(src, 0x26);
    M16(obj, 0x28) = M16(src, 0x28);
}
