/* actor_load_transition_pose [0x0223d0c6]: Selects a flag-dependent pose/duration record and advances the transition byte. */
#include "dh.h"
/* 0x0223d0c6 (leafC) - pick 8-byte entry [sel] from table 0x0608CE9C, sel = bit24 of (arg0+0xE0)?1:0.
 * Copy entry.w@0 -> (arg0+92); zero-extend entry.w@4 -> 32-bit (arg0+0x90);
 * set (arg0+65)=1; (arg0+106)++.
 * fidelity: HIGH - entry words 16-bit, stored 32-bit via extu.w; table stride 8. */
extern s8 g_0608CE9C;
void actor_load_transition_pose(void *arg0) {
    s32 sel = (M32(arg0, 0xE0) & 0x1000000) ? 1 : 0;
    void *e = &g_0608CE9C + sel * 8;
    M16(arg0, 92) = M16(e, 0);
    M32(arg0, 0x90) = (u16)M16(e, 4);
    M8(arg0, 65) = 1;
    M8(arg0, 106) = (s8)(M8(arg0, 106) + 1);
}
