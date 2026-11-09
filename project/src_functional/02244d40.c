/* slot_record_reset [0x02244d40]: Existing map corroborated by source: (member) idx=(s8)(actor+0x34); if idx<8: g_06098838[idx] {[0]=0,[1]=0x12c} (8-byte stride) (promoted from nonmatching draft) */
#include "dh.h"
/* 0x02244d40  set slot field[0x34] of the 8-byte g_06098838[] table: {0, 0x12C}.
 * idx = field[0x34]; if idx < 8: elem.word0 = 0; elem.word1 = 0x12C.
 * fidelity: HIGH - exact (shll2+shll = *8 stride; idx is signed byte). */
extern s32 g_06098838[];  /* array of {s32 a; s32 b;} (8-byte elems) */
void slot_record_reset(void *p) {
    s8 idx = M8(p, 0x34);
    if (idx >= 8)
        return;
    g_06098838[idx * 2]     = 0;
    g_06098838[idx * 2 + 1] = 0x12C;
}
