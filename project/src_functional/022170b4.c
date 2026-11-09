/* 0x022170b4  release_slot_entry: idx = (s8)obj[+0x34]; if idx < 32, ent = 0x060b6d9c + idx*72;
 * if *(s32*)ent != 0, call func_02217d22(ent) and clear ent[+4].
 * fidelity: faithful - signed byte load @(0x34,r4), cmp/ge #32 gate, idx*72 (shll2/shll twice =
 *   *8, +idx = *9, *8 = *72; exts.w), null check, bsr 0x217d22, mov.l #0 to ent[+4]. */
#include "dh_band21.h"

extern void grid_remove_node(s8 *ent);   /* 0x02217d22 */

void release_slot_entry(s8 *obj) {
    s32 idx = *(s8 *)(obj + 0x34);
    if (idx >= 32) return;

    {
        s8 *ent = PTR(s8, DH_ACTOR_MOVEMENT_RECORDS_ADDR) + idx * 72;
        if (*(s32 *)ent != 0) {
            grid_remove_node(ent);
        }
        *(s32 *)(ent + 4) = 0;
    }
}
