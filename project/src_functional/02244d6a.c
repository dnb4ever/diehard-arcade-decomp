/* actor_slot_set_mode7 [0x02244d6a]: Slot+34 selects 8-byte record written {7,300}. */
#include "dh.h"
/* 0x02244d6a  set slot field[0x34] of the 8-byte g_06098838[] table: {7, 0x12C}.
 * idx = field[0x34]; if idx < 8: elem.word0 = 7; elem.word1 = 0x12C.
 * fidelity: HIGH - exact (shll2+shll = *8 stride; idx is signed byte). */
extern s32 g_06098838[];  /* array of {s32 a; s32 b;} (8-byte elems) */
void actor_slot_set_mode7(void *p) {
    s8 idx = M8(p, 0x34);
    if (idx >= 8)
        return;
    g_06098838[idx * 2]     = 7;
    g_06098838[idx * 2 + 1] = 0x12C;
}
