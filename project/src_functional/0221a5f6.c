/* hud_select_player_sources [0x0221a5f6]: Selects the two prompt/input source pointers according to the ready-player mask. */
/* 0x0221a5f6  menu_set_text_ptrs: by the selector @0x06098568, store a pair of
 * string/table pointers into obj[0x64] / obj[0x68] (used for on-screen prompts).
 *   1: obj[0x64]=0x06098738, obj[0x68]=0x060543dc
 *   2: obj[0x64]=0x060543dc, obj[0x68]=0x06098750, flag @0x0609856c |= 2
 *   3: obj[0x64]=0x06098738, obj[0x68]=0x06098750
 * fidelity: faithful - cmp/eq 1/2/3 if-chain (no jump table); other values no-op. */
#include "dh.h"

void hud_select_player_sources(void *obj) {
    switch (M32(0x06098568, 0)) {
    case 1:
        M32(obj, 0x64) = 0x06098738;
        M32(obj, 0x68) = 0x060543dc;
        break;
    case 2:
        M32(obj, 0x64) = 0x060543dc;
        M32(obj, 0x68) = 0x06098750;
        M32(0x0609856c, 0) |= 2;
        break;
    case 3:
        M32(obj, 0x64) = 0x06098738;
        M32(obj, 0x68) = 0x06098750;
        break;
    }
}
