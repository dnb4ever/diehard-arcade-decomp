/* display_commit_channel_accumulator [0x022386c6]: Copies the pending display triple into the VDP2 shadow then clears the accumulator. */
#include "dh.h"
/* 0x022386c6 - flush pending scratch triple at 0x0609861e (if flag at
 * 0x0609861c is nonzero) into the record pointed to by 0x06098f08 at
 * offset 0x114 (+0x114/+0x116/+0x118), then zero the scratch triple.
 * fidelity: HIGH - three s16 copy stores then three zeroing stores. */
void display_commit_channel_accumulator(void) {
    if ((s8)M8(0x0609861c, 0) != 0) {
        u32 dst = MU32(DH_VDP2_REGISTER_SHADOW_ADDR, 0) + 0x114;
        M16(dst, 0) = M16(0x0609861e, 0);
        M16(dst, 2) = M16(0x0609861e, 2);
        M16(dst, 4) = M16(0x0609861e, 4);
        M16(0x0609861e, 0) = 0;
        M16(0x0609861e, 2) = 0;
        M16(0x0609861e, 4) = 0;
    }
}
