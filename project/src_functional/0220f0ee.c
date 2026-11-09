/* camera_copy_selected_part_pose [0x0220f0ee]: Resolves the supplied source selector and copies its part position and angles. */
/* func_0220f0ee (cart 0x20f0ee) - leaf.
 * Resolve a source node p from arg1 by a small selector
 * (p[0x10]+p[0x14]): follow one of several pointer chains. If arg0->[0x68]==1
 * clear it. Then copy a position triple from the sprite table
 * (0x06097D68 + (p[0x34]<<8)) into arg0->[8/C/10] and three angle words
 * [0x24/26/28] from p. Selector 0 returns early.
 * Confidence: MED. No calls. Faithful to disasm/m2c.
 */
#include "decomp.h"

void camera_copy_selected_part_pose(void *arg0, void *arg1) {
    void *p = arg1;
    s32 sel = FS32(p, 0x10) + FS32(p, 0x14);
    void *e;

    if (FS32(arg0, 0x68) == 1)
        FS32(arg0, 0x68) = 0;

    switch (sel) {
    case 0:
        return;
    case 2:
        p = FPTR(FPTR(p, 0x34), 8);
        break;
    case 3:
        p = FPTR(FPTR(p, 0x30), 8);
        break;
    case 1:
        p = FPTR(FPTR(p, 0x30), 8);
        /* fallthrough */
    default:
        break;
    }

    e = (char *)0x06097D68 + (FS8(p, 0x34) << 8);
    FS32(arg0, 0x08) = FS32(e, 0);
    FS32(arg0, 0x0C) = FS32(e, 4);
    FS32(arg0, 0x10) = FS32(e, 8);
    FS16(arg0, 0x24) = FS16(p, 0x24);
    FS16(arg0, 0x26) = FS16(p, 0x26);
    FS16(arg0, 0x28) = FS16(p, 0x28);
}
