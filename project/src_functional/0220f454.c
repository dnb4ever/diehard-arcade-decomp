/* camera_update_translated_source [0x0220f454]: Updates the selected source-relative position without the rotated-source path. */
#include "dh.h"
/* 0x0220f454 - per-frame position update for a 3D object `obj` (non-rotated sibling of
 * func_0220f28e). Source record is selected off the global stage block g_06099584 with the
 * same selector (sel = g->unk10 + g->unk14; 0 -> no-op; 1/2 -> *(g->unk30/0x34)->unk8,
 * flag 1; 3 -> *(g->unk30)->unk8, flag 2).
 *
 * First init (obj->unk68 == 1): copy base position obj->{B8,BC,C0} = arg1->{0,4,8} and
 * velocity obj->{14,18,1C} = arg2->{0,4,8} (direct, no scaling), clear obj->unk68, and if
 * flag != 0 snap target obj->{90,94,98} to src->{8,C,10}.
 *
 * Steady state: ease obj->{90,94,98} an eighth of the way toward src->{8,C,10}
 * (delta/8 via sdiv32); set world position obj->{8,C,10} = obj->{90,94,98} + obj->{B8,BC,C0}
 * (no matrix transform); compute look-at angles obj->{24,26} via func_0221055c from
 * (obj->{8,C,10}) toward (obj->90, obj->94+0x15000, obj->98); and integrate base position
 * obj->{B8,BC,C0} += velocity obj->{14,18,1C}.
 *
 * Real entry 0x20f454 (fn.units member 0x20f44a is trailing-pool-contaminated). m2c aborts
 * on this one (shared pool-adjacent label .L20f488), so it is hand-transcribed from asm.
 * The 3 ease divides resolve to sdiv32 (0x24a06c), the angle call to func_0221055c (0x21055c).
 * fidelity: HIGH - transcribed from the fn.py disassembly; PIC deltas verified. */
s32 sdiv32(s32 dividend, s32 divisor);                  /* 0x0224a06c */
void camera_compute_lookat_angles(s32 x, s32 y, s32 z, s32 bx, s32 by, s32 bz, s16 *a6, s16 *a7);

void camera_update_translated_source(void *obj, void *arg1, void *arg2) {
    void *G = (void *)DH_ACTOR_SLOT_REGISTRY_ADDR;
    s32 sel = M32(G, 0x10) + M32(G, 0x14);
    void *src = 0;
    s32 flag;

    if (sel == 0)
        return;
    if (sel == 1) {
        src = MP(MP(G, 0x30), 8);
        flag = 1;
    } else if (sel == 2) {
        src = MP(MP(G, 0x34), 8);
        flag = 1;
    } else if (sel == 3) {
        src = MP(MP(G, 0x30), 8);
        flag = 2;
    } else {
        flag = sel;
    }

    if (M32(obj, 0x68) == 1) {
        M32(obj, 0xB8) = M32(arg1, 0);
        M32(obj, 0xBC) = M32(arg1, 4);
        M32(obj, 0xC0) = M32(arg1, 8);
        M32(obj, 0x14) = M32(arg2, 0);
        M32(obj, 0x18) = M32(arg2, 4);
        M32(obj, 0x1C) = M32(arg2, 8);
        M32(obj, 0x68) = 0;
        if (flag != 0) {
            M32(obj, 0x90) = M32(src, 8);
            M32(obj, 0x94) = M32(src, 0xC);
            M32(obj, 0x98) = M32(src, 0x10);
        }
    } else {
        M32(obj, 0x90) += sdiv32(M32(src, 8)    - M32(obj, 0x90), 8);
        M32(obj, 0x94) += sdiv32(M32(src, 0xC)  - M32(obj, 0x94), 8);
        M32(obj, 0x98) += sdiv32(M32(src, 0x10) - M32(obj, 0x98), 8);
        M32(obj, 0x08) = M32(obj, 0x90) + M32(obj, 0xB8);
        M32(obj, 0x0C) = M32(obj, 0x94) + M32(obj, 0xBC);
        M32(obj, 0x10) = M32(obj, 0x98) + M32(obj, 0xC0);
        camera_compute_lookat_angles(M32(obj, 8), M32(obj, 0xC), M32(obj, 0x10),
                      M32(obj, 0x90), M32(obj, 0x94) + 0x15000, M32(obj, 0x98),
                      (s16 *)((char *)obj + 0x24), (s16 *)((char *)obj + 0x26));
        M32(obj, 0xB8) += M32(obj, 0x14);
        M32(obj, 0xBC) += M32(obj, 0x18);
        M32(obj, 0xC0) += M32(obj, 0x1C);
    }
}
