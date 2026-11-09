/* effect_arm_follow_without_copy [0x022319b2]: Owner flag/lifetime gates then installs follow handler without position-copy block. */
#include "dh.h"
/* 0x022319b2 (leafC) - task step (variant of 0x0223194a w/o the field copies).
 * obj=*(arg0+8), src=*( *(arg0+20)+8 ). (obj+104) |= 0x0604EBE4[ *(s8)(obj+52) ].
 * If (obj+60)&1 && !((src+60)&0x20): *arg0=0.
 * Else if (obj+60)&2 and --(obj.w@88) had orig<=0: *arg0=0.
 * Else: *arg0 = resume ptr 0x06035A1C (mova 0x2319FC+0x20).
 * fidelity: MED - counter 16-bit, flags 32-bit, table stride 4. */
extern s32 g_actor_slot_masks_legacy;
void effect_arm_follow_without_copy(void *arg0) {
    void *o = MP(arg0, 0x8);
    void *src = MP(MP(arg0, 20), 8);
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
