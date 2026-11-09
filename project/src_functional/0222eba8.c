/* animation_cell_tick [0x0222eba8]: Frame/dwell counters advance 8-byte cell table then upload selected frame. */
#include "dh.h"
/* 0x0222eba8 - advance an animation/timer cell on control block g_06096c94 against
 * the 8-byte cell table g_06085854 (indexed by cur index unk6), then tail-call
 * func_0222ec3c. Fields on g_06096c94: unk0 byte restart-flag; unk2 s16 counter;
 * unk4 s16 status flags; unk6 s16 current index; unk8 s16 reset index.
 *   unk4 = 0;
 *   if (unk0 & 1): unk0 = 0; unk2 = 1; unk6 = unk8; unk4 |= 1;  else: ++unk2;
 *   entry = &g_06085854[unk6*8];
 *   if ((u32)unk2 > (u32)entry[0]) { unk4 |= 1; unk2 = 1; unk6 = unk8; entry = ...; }
 *   if (unk2 == entry[0]) unk4 |= 2;
 *   func_0222ec3c();
 * fidelity: HIGH - m2c over-swallowed past this function; bounded to 0x22ec3c
 * (next real entry), which is reached by a tail bra (modeled as a call). cmp/hi is
 * an unsigned compare of the sign-extended s16 fields. */
extern u8   g_06085854[];
extern void animation_cell_upload_frame(void);

void animation_cell_tick(void) {
    void *c = (void *)(uintptr_t)0x06096c94;
    u8 *entry;

    M16(c, 4) = 0;
    if (M8(c, 0) & 1) {
        M8(c, 0) = 0;
        M16(c, 2) = 1;
        M16(c, 6) = M16(c, 8);
        M16(c, 4) |= 1;
    } else {
        M16(c, 2) += 1;
    }
    entry = &g_06085854[M16(c, 6) << 3];
    if ((u32)M16(c, 2) > (u32)M16(entry, 0)) {
        M16(c, 4) |= 1;
        M16(c, 2) = 1;
        M16(c, 6) = M16(c, 8);
        entry = &g_06085854[M16(c, 6) << 3];
    }
    if (M16(c, 2) == M16(entry, 0)) {
        M16(c, 4) |= 2;
    }
    animation_cell_upload_frame();
}
