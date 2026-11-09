/* vdp1_reset_sort_arenas [0x02214dae]: Clears draw heads, writes the VDP1 end command and resets both radix-sort arenas. */
#include "dh.h"
/* 0x02214dae (vdp1 cmd-list / sort-arena reset) - run clear_record_heads(), terminate the VDP1 command
 * list by writing the end code 0x8000 to the first command word at VDP1 VRAM 0x25C00000, then reset
 * the two entity radix-sort bucket-arena pointers: g_060BACB8 = 0x060BB8C0 (= 0x060BACC0 + 0xC00),
 * g_060BACBC = 0x060C00C0 (= 0x060BF4C0 + 0xC00). (Arena bases match entity_radix_sort 0x0214e2e.)
 * fidelity: HIGH - VDP1 word write, arena pointers long. Computed call via braf_rewrite.py. */
extern void clear_record_heads(void);
extern volatile s16 reg_25C00000;   /* VDP1 VRAM command table */
extern s32 g_060BACB8;              /* radix-sort arena pointer 1 */
extern s32 g_060BACBC;              /* radix-sort arena pointer 2 */

void vdp1_reset_sort_arenas(void) {
    clear_record_heads();
    reg_25C00000 = (s16)0x8000;
    g_060BACB8 = 0x060BB8C0;
    g_060BACBC = 0x060C00C0;
}
