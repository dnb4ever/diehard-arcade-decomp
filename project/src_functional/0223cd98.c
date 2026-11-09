/* effect_retire_finished_slots [0x0223cd98]: Sweeps eight slots; completion signal sets teardown phase/timer and decrements count. */
#include "dh.h"
/* 0x0223cd98 - sweep the 8 active-effect slots and retire any that finished.
 * For slot i in 0..7: if g_060e0f0c[i] != 0 and g_060e0f2c[i] == 1, the effect
 * object g_060e0f4c[i] is torn down: obj->unk10 = 5; obj->unk8->unkC4 = 20;
 * subtract g_0608cf18[i] from the g_060e1214 accumulator; decrement the active
 * count g_060e0e88; and clear the three slot arrays (g_060e0f0c/2c/4c[i]).
 * (Source is 4x loop-unrolled.) fidelity: HIGH. */
extern s32   g_060e0f0c[];
extern s32   g_060e0f2c[];
extern void *g_060e0f4c[];
extern s16   g_0608cf18[];
extern s16   g_060e1214;
extern s32   g_060e0e88;

void effect_retire_finished_slots(void) {
    s32 i;
    void *obj;

    for (i = 0; i < 8; i++) {
        if (g_060e0f0c[i] != 0 && g_060e0f2c[i] == 1) {
            obj = g_060e0f4c[i];
            M8(obj, 0x10) = 5;
            M32(MP(obj, 8), 0xC4) = 20;
            g_060e1214 -= g_0608cf18[i];
            g_060e0e88 -= 1;
            g_060e0f0c[i] = 0;
            g_060e0f2c[i] = 0;
            g_060e0f4c[i] = 0;
        }
    }
}
