/* effect_pool_make_room [0x0223e4a0]: Finds an active recyclable effect when full, releases its slot and starts its retirement countdown. */
#include "dh.h"
/* 0x0223e4a0 (callC) - returns s32. base=0x060E0E88.
 * If *(s32)base <= 7 return 1. Scan i=0..7:
 *   if base[0xA4 + 4*i] == 1: entry = base[0xC4 + 4*i]; (entry+16)=5 (byte);
 *     obj = *(void**)(entry+8); call func_0223cd0c(obj); (obj+0xC4)=40 (long); return 1.
 *   if i == 7 (no match): return 0.
 * bsrf cart 0x23cd0c -> RAM 0x06040d0c.
 * fidelity: HIGH - slot table 32-bit (stride 4); (entry+16) 8-bit; (obj+0xC4) 32-bit. */
extern s32 g_060E0E88;
extern void effect_pool_release(void *);
s32 effect_pool_make_room(void) {
    void *base = &g_060E0E88;
    s32 i;
    if (M32(base, 0) <= 7)
        return 1;
    for (i = 0; i < 8; i++) {
        if (M32(base, 0xA4 + i * 4) == 1) {
            void *entry = MP(base, 0xC4 + i * 4);
            void *obj;
            M8(entry, 16) = 5;
            obj = MP(entry, 8);
            effect_pool_release(obj);
            M32(obj, 0xC4) = 40;
            return 1;
        }
        if (i == 7)
            return 0;
    }
    return 1;
}
