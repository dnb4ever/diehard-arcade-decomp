/* animation_bind_channel [0x02210858]: Installs a nonzero keyframe list in the slot's channel and clears its countdown. */
#include "dh.h"
/* 0x02210858 (leafC) - if val != 0, register it: g_060E1218[obj->field34] = val and
 * clear g_060E1298[obj->field34]. The index is the signed byte at obj+0x34, stride 4.
 * fidelity: HIGH - exact (mov.b index, shll2 stride, 32-bit stores). Leaf. */
extern s32 g_060E1218[];
extern s32 g_060E1298[];
void animation_bind_channel(void *obj, s32 val) {
    if (val != 0) {
        s8 i = M8(obj, 0x34);
        g_060E1218[i] = val;
        g_060E1298[i] = 0;
    }
}
