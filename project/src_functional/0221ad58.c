/* scene_spawn_matching_behaviors [0x0221ad58]: Spawns behaviors from scene-ID/value matches or the fallback handler when none match. */
/* 0x0221ad58  spawn_matched_entities: scan a {id, value} table (stride 8,
 * terminated by id < 0) chosen by *0x06098670 (1 -> 0x06054670, else 0x060546e0).
 * For every entry whose id equals the current id *0x06098668, spawn a behaviour
 * object via func_0220fece(obj, 0x0601edd4) and set spawned->unk8->unk64 = value.
 * If at least one matched, clear *0x060986b8. Otherwise, unless the id is 0x43,
 * spawn a fallback behaviour via func_0220fece(obj, 0x0603bea6).
 * fidelity: faithful - PIC pointers (mova+add) resolved to func_0220fece; both
 *   behaviour arguments are runtime HWRAM addresses (cart 0x200000 <-> 0x06004000:
 *   0x0601ed90 + 0x44 = 0x0601edd4 for the matched case). */
#include "dh.h"

extern void *task_spawn_current_work6c(s32 obj, u32 behaviour);

void scene_spawn_matching_behaviors(s32 obj) {
    u32 tbl = (M32(DH_PLAY_SESSION_MODE_ADDR, 0) == 1) ? 0x06054670 : 0x060546e0;
    s32 found = 0;

    while (M32(tbl, 0) >= 0) {
        if (M32(tbl, 0) == M32(DH_SCENE_INDEX_ADDR, 0)) {
            void *e = task_spawn_current_work6c(obj, 0x0601edd4);
            M32(MP(e, 8), 0x64) = M32(tbl, 4);
            found = 1;
        }
        tbl += 8;
    }

    if (found) {
        M32(0x060986b8, 0) = 0;
        return;
    }
    if (M32(DH_SCENE_INDEX_ADDR, 0) != 0x43) {
        task_spawn_current_work6c(obj, 0x0603bea6);
    }
}
