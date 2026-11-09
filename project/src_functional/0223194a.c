/* effect_follow_parent_task [0x0223194a]: Copies the parent pose and slot mask, enforces lifetime gates and installs its next handler. */
#include "dh.h"
/* 0x0223194a (leafC) - task step. obj=*(arg0+8), src=*( *(arg0+20)+8 ).
 * Copy src[8/12/16]->obj[8/12/16] (long), src.b@52->obj.b@52, src[56]->obj[56] (long);
 * (obj+104) |= table 0x0604EBE4[ *(s8)(obj+52) ].
 * If (obj+60)&1 && !((src+60)&0x20): *arg0=0.
 * Else if (obj+60)&2 and --(obj.w@88) had orig<=0: *arg0=0.
 * Else: *arg0 = resume ptr 0x06035A1C (mova 0x2319A8+0x74).
 * fidelity: MED - control flow faithful; copies 32-bit, field52 8-bit, counter88 16-bit. */
extern s32 g_actor_slot_masks_legacy;
void effect_follow_parent_task(void *arg0) {
    void *o = MP(arg0, 0x8);
    void *src = MP(MP(arg0, 20), 8);
    M32(o, 0x8)  = M32(src, 0x8);
    M32(o, 0xC)  = M32(src, 0xC);
    M32(o, 0x10) = M32(src, 0x10);
    M8(o, 52) = M8(src, 52);
    M32(o, 56) = M32(src, 56);
    M32(o, 104) |= (&g_actor_slot_masks_legacy)[ M8(o, 52) ];
    if ((M32(o, 60) & 1) && !(M32(src, 60) & 0x20)) {
        M32(arg0, 0) = 0;
        return;
    }
    if (M32(o, 60) & 2) {
        s16 orig = M16(o, 88);
        M16(o, 88) = (s16)(orig - 1);
        if (orig <= 0) {
            M32(arg0, 0) = 0;
            return;
        }
    }
    M32(arg0, 0) = (s32)0x06035A1C;
}
