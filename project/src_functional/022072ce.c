/* background_set_view_projection [0x022072ce]: View-task work X+Z and inverted half Y feed display block 060b8b90. */
/* func_022072ce (cart 0x2072ce) - leaf.
 * Fill a target struct (ptr @0x060B8B90) from a source struct reached via
 * (*0x0609882C)->[8]: pos = src[8]+src[0x10]; a = 0x01000000 - (src[0xC]>>1);
 * two slots = 0x16000.
 * Confidence: HIGH. No calls. Faithful to disasm.
 */
#include "decomp.h"

void background_set_view_projection(void) {
    void *a = *(void **)0x060B8B90;
    void *b = FPTR(*(void **)DH_VIEW_TASK_ADDR, 8);

    FS32(a, 0x00) = FS32(b, 0x08) + FS32(b, 0x10);
    FS32(a, 0x04) = 0x01000000 - (FS32(b, 0x0C) >> 1);
    FS32(a, 0x14) = 0x16000;
    FS32(a, 0x10) = 0x16000;
}
