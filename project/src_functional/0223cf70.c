/* actor_attach_effect_slot [0x0223cf70]: Spawn result selects actor byte pair +6a/+6b or +70/+71 and interaction flag. */
#include "dh.h"
/* 0x0223cf70 (callC) - returns s32. r = func_0223cbb0(arg1, 0) (bsr cart 0x23cbb0).
 * If r == 0 return -1. obj = *(void**)(r+8).
 * If arg1 == 1: (arg0+106)=1, (arg0+107)=*(s8)(obj+52) (bytes); (arg0+0x8C) |= 0x2000000.
 * else:         (arg0+112)=1, (arg0+113)=*(s8)(obj+52) (bytes); (arg0+0x8C) |= 0x1000000.
 * return 0.  fidelity: HIGH - fields 8-bit; flag word 32-bit at 0x8C. */
extern void *effect_pool_spawn(s32, s32);
s32 actor_attach_effect_slot(void *arg0, s32 arg1) {
    void *r = effect_pool_spawn(arg1, 0);
    void *obj;
    if (r == 0)
        return -1;
    obj = MP(r, 8);
    if (arg1 == 1) {
        M8(arg0, 106) = 1;
        M8(arg0, 107) = M8(obj, 52);
        M32(arg0, 0x8C) |= 0x2000000;
    } else {
        M8(arg0, 112) = 1;
        M8(arg0, 113) = M8(obj, 52);
        M32(arg0, 0x8C) |= 0x1000000;
    }
    return 0;
}
