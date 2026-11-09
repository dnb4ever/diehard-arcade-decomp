/* player_clear_saved_action [0x0223dcc2]: Clears the selected player's sixteen-byte saved action record. */
#include "dh.h"
/* func_0223dcc2(idx): clear the 16-byte (4-long) record at g_060E0E38[idx-1].
 * Tail-jumps to the longword zero-fill helper func_0221327c(dst, nlongs = 4).
 * fidelity: HIGH - 0x10 stride and the tail-jump to the zero-fill helper verified;
 *   the dead stack-save of idx (never read before the tail-jump) is omitted. */
extern void zero_longs_raw_entry(void *dst, s32 nlongs);   /* zero-fill N longwords */
extern u8   g_060E0E38[];

void player_clear_saved_action(s32 idx) {
    zero_longs_raw_entry(&g_060E0E38[(idx - 1) * 16], 4);
}
