/* sprite_clear_character_tables [0x022151d8]: Clears both sprite character/attribute table heads by sixteen-byte strides. */
/* 0x022151d8 (clear_sprite_tables) - clear the two sprite character/attribute
 * tables by strided zero-fill: func_022148ac(0x060cf0cc, 0x3800) then a tail-call
 * func_022148ac(0x060d28cc, 0x3400).
 * NOTE: fn.extent (0x1ae) over-runs this entry far past its real end - the braf is
 * a tail-branch, not a terminator, and a large separate texture-upload routine and
 * two literal pools sit between here and 0x215386. This TU models only 0x2151d8's
 * own body, which ends at that braf.
 * fidelity: faithful - two literal (base,len) pairs, bsrf + braf tail-call resolved
 *   to func_022148ac. */
#include "dh_band21.h"

extern void cache_clear_line_heads(u32 base, u32 len);

void sprite_clear_character_tables(void) {
    cache_clear_line_heads(0x060cf0cc, 0x3800);
    cache_clear_line_heads(0x060d28cc, 0x3400);
}
